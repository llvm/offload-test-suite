//===- DX/Device.cpp - DirectX Device API ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include <wrl/client.h>

#include <cstdint>
#include <d3d12.h>
#include <d3dx12.h>
#include <dxcore.h>
#include <dxgiformat.h>
#include <dxguids.h>

#ifndef _WIN32
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wsl/winadapter.h>

#endif

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/Capabilities.h"
#include "API/Device.h"
#include "API/FormatConversion.h"
#include "DXFeatures.h"
#include "Support/Pipeline.h"
#include "Support/WinError.h"

#include "DXResources.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Object/DXContainer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Signals.h"

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>

using namespace offloadtest;
using Microsoft::WRL::ComPtr;

template <> char CapabilityValueEnum<directx::ShaderModel>::ID = 0;
template <> char CapabilityValueEnum<directx::RootSignature>::ID = 0;

static std::mutex SignalHandlerMutex;
static llvm::SmallVector<ID3D12Device *> SignalHandlerDevices;

static void dumpD3DInfoQueues(void *) {
  const std::lock_guard<std::mutex> Lock(SignalHandlerMutex);
  for (ID3D12Device *Device : SignalHandlerDevices) {
    ComPtr<ID3D12InfoQueue> InfoQueue;
    HRESULT HR = Device->QueryInterface(InfoQueue.GetAddressOf());
    if (FAILED(HR)) {
      llvm::errs() << "Failed to query D3D info queue\n";
      continue;
    }
    for (int I = 0, E = InfoQueue->GetNumStoredMessages(); I < E; ++I) {
      SIZE_T Len = 0;
      HR = InfoQueue->GetMessage(I, NULL, &Len);
      if (FAILED(HR)) {
        llvm::errs() << "Failed to get message " << I
                     << " from D3D info queue\n";
      } else {
        D3D12_MESSAGE *Msg = (D3D12_MESSAGE *)malloc(Len);
        HR = InfoQueue->GetMessage(I, Msg, &Len);
        llvm::errs() << "D3D: " << Msg->pDescription << "\n";
        free(Msg);
      }
    }
  }
}

#define DXFormats(FMT)                                                         \
  if (Channels == 1)                                                           \
    return DXGI_FORMAT_R32_##FMT;                                              \
  if (Channels == 2)                                                           \
    return DXGI_FORMAT_R32G32_##FMT;                                           \
  if (Channels == 3)                                                           \
    return DXGI_FORMAT_R32G32B32_##FMT;                                        \
  if (Channels == 4)                                                           \
    return DXGI_FORMAT_R32G32B32A32_##FMT;

static DXGI_FORMAT getDXFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int32:
    DXFormats(SINT) break;
  case DataFormat::Float32:
    DXFormats(FLOAT) break;
  case DataFormat::UInt64:
  case DataFormat::Int64:
    if (Channels == 1)
      return DXGI_FORMAT_R32G32_UINT;
    if (Channels == 2)
      return DXGI_FORMAT_R32G32B32A32_UINT;
    llvm_unreachable("Unsupported channel count for 64-bit format");
  case DataFormat::Depth32:
    llvm_unreachable(
        "Depth32 format is not yet supported in the DirectX backend.");
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT getRawDXFormat(const Resource &R) {
  if (!R.isByteAddressBuffer())
    return DXGI_FORMAT_UNKNOWN;

  switch (R.BufferPtr->Format) {
  case DataFormat::Hex16:
  case DataFormat::UInt16:
  case DataFormat::Int16:
  case DataFormat::Float16:
  case DataFormat::Hex32:
  case DataFormat::UInt32:
  case DataFormat::Int32:
  case DataFormat::Float32:
  case DataFormat::Hex64:
  case DataFormat::UInt64:
  case DataFormat::Int64:
  case DataFormat::Float64:
    return DXGI_FORMAT_R32_TYPELESS;
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return DXGI_FORMAT_UNKNOWN;
}

static uint32_t getUAVBufferSize(const Resource &R) {
  return R.HasCounter
             ? llvm::alignTo(R.size(), D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT) +
                   sizeof(uint32_t)
             : R.size();
}

static uint32_t getUAVBufferCounterOffset(const Resource &R) {
  return R.HasCounter
             ? llvm::alignTo(R.size(), D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT)
             : 0;
}

static D3D12_RESOURCE_DIMENSION getDXDimension(ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::ConstantBuffer:
    return D3D12_RESOURCE_DIMENSION_BUFFER;
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
    return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  case ResourceKind::Sampler:
    return D3D12_RESOURCE_DIMENSION_UNKNOWN;
  case ResourceKind::SampledTexture2D:
    llvm_unreachable("SampledTextures aren't supported in DirectX!");
  }
  llvm_unreachable("All cases handled");
}

enum DXResourceKind { UAV, SRV, CBV, SAMPLER };

static DXResourceKind getDXKind(offloadtest::ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::Texture2D:
    return SRV;

  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::RWTexture2D:
    return UAV;

  case ResourceKind::ConstantBuffer:
    return CBV;

  case ResourceKind::Sampler:
    return SAMPLER;
  case ResourceKind::SampledTexture2D:
    llvm_unreachable("Sampled textures aren't supported in DirectX!");
  }
  llvm_unreachable("All cases handled");
}

static llvm::Expected<D3D12_RESOURCE_DESC>
getResourceDescription(const Resource &R) {
  const D3D12_RESOURCE_DIMENSION Dimension = getDXDimension(R.Kind);
  const offloadtest::CPUBuffer &B = *R.BufferPtr;

  if (B.OutputProps.MipLevels != 1)
    return llvm::createStringError(std::errc::not_supported,
                                   "Multiple mip levels are not yet supported "
                                   "for DirectX textures.");

  const DXGI_FORMAT Format =
      R.isTexture() ? getDXFormat(B.Format, B.Channels) : DXGI_FORMAT_UNKNOWN;
  const uint32_t Width =
      R.isTexture() ? B.OutputProps.Width : getUAVBufferSize(R);
  const uint32_t Height = R.isTexture() ? B.OutputProps.Height : 1;
  D3D12_TEXTURE_LAYOUT Layout;

  if (R.isTexture())
    Layout =
        R.IsReserved && (getDXKind(R.Kind) == SRV || getDXKind(R.Kind) == UAV)
            ? D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE
            : D3D12_TEXTURE_LAYOUT_UNKNOWN;
  else
    Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

  const D3D12_RESOURCE_FLAGS Flags =
      R.isReadWrite() ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                      : D3D12_RESOURCE_FLAG_NONE;
  const D3D12_RESOURCE_DESC ResDesc = {Dimension, 0,      Width,  Height, 1, 1,
                                       Format,    {1, 0}, Layout, Flags};
  return ResDesc;
}

static D3D12_SHADER_RESOURCE_VIEW_DESC getSRVDescription(const Resource &R) {
  const uint32_t EltSize = R.getElementSize();
  const uint32_t NumElts = R.size() / EltSize;

  llvm::outs() << "    EltSize = " << EltSize << " NumElts = " << NumElts
               << "\n";
  D3D12_SHADER_RESOURCE_VIEW_DESC Desc = {};
  Desc.Format = R.isRaw()
                    ? getRawDXFormat(R)
                    : getDXFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
  Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  switch (R.Kind) {
  case ResourceKind::Buffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::ByteAddressBuffer:

    Desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    Desc.Buffer =
        D3D12_BUFFER_SRV{0, NumElts, R.isStructuredBuffer() ? EltSize : 0,
                         R.isByteAddressBuffer() ? D3D12_BUFFER_SRV_FLAG_RAW
                                                 : D3D12_BUFFER_SRV_FLAG_NONE};
    break;
  case ResourceKind::Texture2D:
    Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    Desc.Texture2D = D3D12_TEX2D_SRV{0, 1, 0, 0};
    break;
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::RWTexture2D:
  case ResourceKind::ConstantBuffer:
  case ResourceKind::Sampler:
    llvm_unreachable("Not an SRV type!");
  case ResourceKind::SampledTexture2D:
    llvm_unreachable("Sampled textures aren't supported in DirectX!");
  }
  return Desc;
}

static D3D12_UNORDERED_ACCESS_VIEW_DESC getUAVDescription(const Resource &R) {
  const uint32_t EltSize = R.getElementSize();
  const uint32_t NumElts = R.size() / EltSize;
  const uint32_t CounterOffset = getUAVBufferCounterOffset(R);

  llvm::outs() << "    EltSize = " << EltSize << " NumElts = " << NumElts
               << "\n";
  D3D12_UNORDERED_ACCESS_VIEW_DESC Desc = {};
  Desc.Format = R.isRaw()
                    ? getRawDXFormat(R)
                    : getDXFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
  switch (R.Kind) {
  case ResourceKind::RWBuffer:
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::RWByteAddressBuffer:

    Desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    Desc.Buffer = D3D12_BUFFER_UAV{
        0, NumElts, R.isStructuredBuffer() ? EltSize : 0, CounterOffset,
        R.isByteAddressBuffer() ? D3D12_BUFFER_UAV_FLAG_RAW
                                : D3D12_BUFFER_UAV_FLAG_NONE};
    break;
  case ResourceKind::RWTexture2D:
    Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    Desc.Texture2D = D3D12_TEX2D_UAV{0, 0};
    break;
  case ResourceKind::StructuredBuffer:
  case ResourceKind::Buffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::Texture2D:
  case ResourceKind::ConstantBuffer:
  case ResourceKind::Sampler:
    llvm_unreachable("Not a UAV type!");
  case ResourceKind::SampledTexture2D:
    llvm_unreachable("Sampled textures aren't supported in DirectX!");
  }
  return Desc;
}

namespace {

class DXBuffer : public offloadtest::Buffer {
public:
  ComPtr<ID3D12Resource> Buffer;
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;

  DXBuffer(ComPtr<ID3D12Resource> Buffer, llvm::StringRef Name,
           BufferCreateDesc Desc, size_t SizeInBytes)
      : offloadtest::Buffer(GPUAPI::DirectX), Buffer(Buffer), Name(Name),
        Desc(Desc), SizeInBytes(SizeInBytes) {}

  static bool classof(const offloadtest::Buffer *B) {
    return B->getAPI() == GPUAPI::DirectX;
  }
};

class DXTexture : public offloadtest::Texture {
public:
  ComPtr<ID3D12Resource> Resource;

  // TODO:
  // Ideally SRVs/UAVs would also live here, but they currently require a
  // shared CBV_SRV_UAV heap whose indices are determined at pipeline bind time.
  // Moving them here would require a descriptor heap allocator, which is not
  // yet implemented.
  // Either an RTV or DSV descriptor, depending on Desc.Usage.
  // A zero ptr means no descriptor was created for that view type.
  D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};

  std::string Name;
  TextureCreateDesc Desc;

  DXTexture(ComPtr<ID3D12Resource> Resource, llvm::StringRef Name,
            TextureCreateDesc Desc)
      : offloadtest::Texture(GPUAPI::DirectX), Resource(Resource), Name(Name),
        Desc(Desc) {}

  static bool classof(const offloadtest::Texture *T) {
    return T->getAPI() == GPUAPI::DirectX;
  }
};

class DXPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  ComPtr<ID3D12RootSignature> RootSig;
  ComPtr<ID3D12PipelineState> PSO;

  DXPipelineState(llvm::StringRef Name, ComPtr<ID3D12RootSignature> RootSig,
                  ComPtr<ID3D12PipelineState> PSO)
      : Name(Name), RootSig(RootSig), PSO(PSO) {}
};

class DXFence : public offloadtest::Fence {
public:
#ifdef _WIN32
  DXFence(ComPtr<ID3D12Fence> Fence, HANDLE Event, llvm::StringRef Name)
#else // WSL
  DXFence(ComPtr<ID3D12Fence> Fence, int Event, llvm::StringRef Name)
#endif
      : Name(Name), Fence(Fence), Event(Event) {
  }

  std::string Name;
  ComPtr<ID3D12Fence> Fence;
#ifdef _WIN32
  HANDLE Event;
#else // WSL
  int Event;
#endif

  static llvm::Expected<std::unique_ptr<DXFence>> create(ID3D12Device *Device,
                                                         llvm::StringRef Name) {
    ComPtr<ID3D12Fence> Fence;
    if (auto Err = HR::toError(
            Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)),
            "Failed to create Fence."))
      return Err;

#ifdef _WIN32
    HANDLE Event = CreateEventA(nullptr, false, false, nullptr);
    if (!Event)
#else // WSL
    int Event = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (Event == -1)
#endif
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create event.");

    return std::make_unique<DXFence>(Fence, Event, Name);
  }

  ~DXFence() {
#ifdef _WIN32
    CloseHandle(Event);
#else // WSL
    close(Event);
#endif
  }

  uint64_t getFenceValue() override { return Fence->GetCompletedValue(); }

  llvm::Error waitForCompletion(uint64_t SignalValue) override {
    if (Fence->GetCompletedValue() >= SignalValue)
      return llvm::Error::success();

#ifdef _WIN32
    if (auto Err = HR::toError(Fence->SetEventOnCompletion(SignalValue, Event),
                               "Failed to register end event."))
      return Err;
    WaitForSingleObject(Event, INFINITE);
#else // WSL
    if (auto Err =
            HR::toError(Fence->SetEventOnCompletion(
                            SignalValue, reinterpret_cast<HANDLE>(Event)),
                        "Failed to register end event."))
      return Err;
    pollfd PollEvent;
    PollEvent.fd = Event;
    PollEvent.events = POLLIN;
    PollEvent.revents = 0;
    if (poll(&PollEvent, 1, -1) == -1)
      return llvm::createStringError(
          std::error_code(errno, std::system_category()), strerror(errno));
#endif
    return llvm::Error::success();
  }
};

class DXQueue : public offloadtest::Queue {
public:
  using Queue::submit;

  ComPtr<ID3D12CommandQueue> Queue;
  std::unique_ptr<DXFence> SubmitFence;
  uint64_t FenceCounter = 0;
  // Batches of command buffers submitted to the GPU that may still be
  // in-flight.  The ID3D12CommandAllocator owns the backing memory for
  // recorded commands, so it must outlive GPU execution.  Each batch
  // records the fence value it signals so we can non-blockingly query
  // progress and release completed batches.
  struct InFlightBatch {
    uint64_t FenceValue;
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs;
  };
  llvm::SmallVector<InFlightBatch> InFlightBatches;

  DXQueue(ComPtr<ID3D12CommandQueue> Queue,
          std::unique_ptr<DXFence> SubmitFence)
      : Queue(Queue), SubmitFence(std::move(SubmitFence)) {}
  DXQueue(DXQueue &&) = default;
  ~DXQueue() override {}

  static llvm::Expected<DXQueue>
  createGraphicsQueue(ComPtr<ID3D12Device> Device) {
    const D3D12_COMMAND_QUEUE_DESC Desc = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
                                           D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    ComPtr<ID3D12CommandQueue> CmdQueue;
    if (auto Err = HR::toError(
            Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(&CmdQueue)),
            "Failed to create command queue."))
      return Err;
    auto FenceOrErr = DXFence::create(Device.Get(), "QueueSubmitFence");
    if (!FenceOrErr)
      return FenceOrErr.takeError();
    return DXQueue(CmdQueue, std::move(*FenceOrErr));
  }

  llvm::Expected<offloadtest::SubmitResult>
  submit(llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs)
      override;
};

class DXCommandBuffer : public offloadtest::CommandBuffer {
public:
  ComPtr<ID3D12CommandAllocator> Allocator;
  ComPtr<ID3D12GraphicsCommandList> CmdList;

  static llvm::Expected<std::unique_ptr<DXCommandBuffer>>
  create(ComPtr<ID3D12Device> Device) {
    auto CB = std::unique_ptr<DXCommandBuffer>(new DXCommandBuffer());
    if (auto Err = HR::toError(
            Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           IID_PPV_ARGS(&CB->Allocator)),
            "Failed to create command allocator."))
      return Err;
    if (auto Err = HR::toError(
            Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      CB->Allocator.Get(), nullptr,
                                      IID_PPV_ARGS(&CB->CmdList)),
            "Failed to create command list."))
      return Err;
    return CB;
  }

  ~DXCommandBuffer() override = default;

  static bool classof(const CommandBuffer *CB) {
    return CB->getKind() == GPUAPI::DirectX;
  }

private:
  DXCommandBuffer() : CommandBuffer(GPUAPI::DirectX) {}
};

struct DescriptorAllocator {
  ComPtr<ID3D12DescriptorHeap> Heap;
  std::atomic<uint32_t> NextIndex{0};
  uint32_t DescIncSize;
  uint32_t Capacity;

  static llvm::Expected<DescriptorAllocator>
  create(ID3D12Device *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type,
         uint32_t Capacity) {
    ComPtr<ID3D12DescriptorHeap> Heap;
    const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {
        Type, Capacity, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 0};
    if (auto Err = HR::toError(
            Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap)),
            "Failed to create descriptor heap for DescriptorAllocator."))
      return Err;
    const uint32_t DescIncSize = Device->GetDescriptorHandleIncrementSize(Type);
    return DescriptorAllocator(Heap, DescIncSize, Capacity);
  }

  llvm::Expected<D3D12_CPU_DESCRIPTOR_HANDLE> allocate() {
    // TODO(manon): Use a better allocator that can also free descriptors.
    const uint32_t Index = NextIndex.fetch_add(1, std::memory_order_relaxed);
    if (Index >= Capacity)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Descriptor heap allocator exhausted.");
    D3D12_CPU_DESCRIPTOR_HANDLE Handle =
        Heap->GetCPUDescriptorHandleForHeapStart();
    Handle.ptr += Index * DescIncSize;
    return Handle;
  }

  DescriptorAllocator(DescriptorAllocator &&Other)
      : Heap(std::move(Other.Heap)),
        NextIndex(Other.NextIndex.load(std::memory_order_relaxed)),
        DescIncSize(Other.DescIncSize), Capacity(Other.Capacity) {}
  DescriptorAllocator &operator=(DescriptorAllocator &&) = delete;
  DescriptorAllocator(const DescriptorAllocator &) = delete;
  DescriptorAllocator &operator=(const DescriptorAllocator &) = delete;

  DescriptorAllocator(ComPtr<ID3D12DescriptorHeap> Heap, uint32_t DescIncSize,
                      uint32_t Capacity)
      : Heap(Heap), DescIncSize(DescIncSize), Capacity(Capacity) {}
};

class DXDevice : public offloadtest::Device {
private:
  ComPtr<IDXCoreAdapter> Adapter;
  ComPtr<ID3D12Device> Device;
  DXQueue GraphicsQueue;
  Capabilities Caps;
  DescriptorAllocator RTVAllocator;
  DescriptorAllocator DSVAllocator;

  struct ResourceSet {
    ComPtr<ID3D12Resource> Upload;
    ComPtr<ID3D12Resource> Buffer;
    ComPtr<ID3D12Resource> Readback;
    ComPtr<ID3D12Heap> Heap;
    ResourceSet(ComPtr<ID3D12Resource> Upload, ComPtr<ID3D12Resource> Buffer,
                ComPtr<ID3D12Resource> Readback,
                ComPtr<ID3D12Heap> Heap = nullptr)
        : Upload(Upload), Buffer(Buffer), Readback(Readback), Heap(Heap) {}
  };

  // ResourceBundle will contain one ResourceSet for a singular resource
  // or multiple ResourceSets for resource array.
  using ResourceBundle = llvm::SmallVector<ResourceSet>;
  using ResourcePair = std::pair<offloadtest::Resource *, ResourceBundle>;

  struct DescriptorTable {
    llvm::SmallVector<ResourcePair> Resources;
  };

  struct InvocationState {
    ComPtr<ID3D12DescriptorHeap> DescHeap;
    std::unique_ptr<DXCommandBuffer> CB;

    std::shared_ptr<PipelineState> Pipeline;

    // Resources for graphics pipelines.
    std::unique_ptr<offloadtest::Texture> RT;
    std::unique_ptr<offloadtest::Buffer> RTReadback;
    std::unique_ptr<offloadtest::Texture> DS;
    ComPtr<ID3D12Resource> VB;

    llvm::SmallVector<DescriptorTable> DescTables;
    llvm::SmallVector<ResourcePair> RootResources;
  };

public:
  DXDevice(ComPtr<IDXCoreAdapter> A, ComPtr<ID3D12Device> D, DXQueue Q,
           DescriptorAllocator RTVAllocator, DescriptorAllocator DSVAllocator,
           std::string Desc)
      : Adapter(A), Device(D), GraphicsQueue(std::move(Q)),
        RTVAllocator(std::move(RTVAllocator)),
        DSVAllocator(std::move(DSVAllocator)) {
    Description = Desc;
  }
  DXDevice(const DXDevice &) = delete;
  DXDevice &operator=(const DXDevice &) = delete;

  ~DXDevice() override {
    const std::lock_guard<std::mutex> Lock(SignalHandlerMutex);
    llvm::erase(SignalHandlerDevices, Device.Get());
  }

  llvm::StringRef getAPIName() const override { return "DirectX"; }
  GPUAPI getAPI() const override { return GPUAPI::DirectX; }

  Queue &getGraphicsQueue() override { return GraphicsQueue; }

  llvm::Error
  createRootSignatureFromShader(llvm::StringRef, const ShaderContainer &Shader,
                                ComPtr<ID3D12RootSignature> &OutRootSignature) {
    // Try pulling a root signature from the DXIL first
    auto ExContainer =
        llvm::object::DXContainer::create(Shader.Shader->getMemBufferRef());
    // If this fails we really have a problem...
    if (!ExContainer)
      return ExContainer.takeError();

    bool HasRootSigPart = false;
    for (const auto &Part : *ExContainer)
      if (memcmp(Part.Part.Name, "RTS0", 4) == 0)
        HasRootSigPart = true;

    if (HasRootSigPart) {
      const llvm::StringRef Binary = Shader.Shader->getBuffer();
      if (auto Err = HR::toError(
              Device->CreateRootSignature(0, Binary.data(), Binary.size(),
                                          IID_PPV_ARGS(&OutRootSignature)),
              "Failed to create root signature."))
        return Err;
    }

    return llvm::Error::success();
  }

  llvm::Error createRootSignatureFromBindingsDesc(
      llvm::StringRef, const BindingsDesc &BindingsDesc, bool IsGraphics,
      ComPtr<ID3D12RootSignature> &OutRootSignature) {
    uint32_t DescriptorCount = 0;
    for (auto &D : BindingsDesc.DescriptorSetDescs)
      DescriptorCount += D.ResourceBindings.size();

    std::vector<D3D12_ROOT_PARAMETER> RootParams;
    const std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]> Ranges =
        std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]>(
            new D3D12_DESCRIPTOR_RANGE[DescriptorCount]);
    uint32_t RangeIdx = 0;
    for (const auto &D : BindingsDesc.DescriptorSetDescs) {
      uint32_t DescriptorIdx = 0;
      const uint32_t StartRangeIdx = RangeIdx;
      for (const auto &R : D.ResourceBindings) {
        switch (getDXKind(R.Kind)) {
        case SRV:
          Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
          break;
        case UAV:
          Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
          break;
        case CBV:
          Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
          break;
        case SAMPLER:
          llvm_unreachable("Not implemented yet."); // Requires a separate heap
        }
        Ranges.get()[RangeIdx].NumDescriptors = R.DescriptorCount;
        Ranges.get()[RangeIdx].BaseShaderRegister = R.DXBinding.Register;
        Ranges.get()[RangeIdx].RegisterSpace = R.DXBinding.Space;
        Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart =
            DescriptorIdx;
        RangeIdx++;
        DescriptorIdx += R.DescriptorCount;
      }
      RootParams.push_back(D3D12_ROOT_PARAMETER{
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
          {D3D12_ROOT_DESCRIPTOR_TABLE{
              static_cast<uint32_t>(D.ResourceBindings.size()),
              &Ranges.get()[StartRangeIdx]}},
          D3D12_SHADER_VISIBILITY_ALL});
    }

    CD3DX12_ROOT_SIGNATURE_DESC Desc;
    Desc.Init(static_cast<uint32_t>(RootParams.size()), RootParams.data(), 0,
              nullptr,
              IsGraphics
                  ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
                  : D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> Signature;
    ComPtr<ID3DBlob> Error;
    if (auto Err = HR::toError(
            D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1,
                                        &Signature, &Error),
            "Failed to seialize root signature.")) {
      const std::string Msg =
          std::string(reinterpret_cast<char *>(Error->GetBufferPointer()),
                      Error->GetBufferSize() / sizeof(char));
      return joinErrors(
          std::move(Err),
          llvm::createStringError(std::errc::protocol_error, Msg.c_str()));
    }

    if (auto Err = HR::toError(
            Device->CreateRootSignature(0, Signature->GetBufferPointer(),
                                        Signature->GetBufferSize(),
                                        IID_PPV_ARGS(&OutRootSignature)),
            "Failed to create root signature."))
      return Err;

    return llvm::Error::success();
  }

  llvm::Error
  createRootSignature(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                      const ShaderContainer &Shader, bool IsGraphics,
                      ComPtr<ID3D12RootSignature> &OutRootSignature) {
    assert(OutRootSignature.Get() == nullptr);

    if (auto Err =
            createRootSignatureFromShader(Name, Shader, OutRootSignature))
      return Err;

    if (OutRootSignature.Get() != nullptr)
      return llvm::Error::success();

    return createRootSignatureFromBindingsDesc(Name, BindingsDesc, IsGraphics,
                                               OutRootSignature);
  }

  llvm::Expected<std::shared_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                   ShaderContainer CS) override {
    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BindingsDesc, CS,
                                       /*IsGraphics=*/false, RootSig))
      return Err;

    auto DXIL = CS.Shader->getBuffer();
    const D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {
        RootSig.Get(),
        {DXIL.data(), DXIL.size()},
        0,
        {
            nullptr,
            0,
        },
        D3D12_PIPELINE_STATE_FLAG_NONE};

    ComPtr<ID3D12PipelineState> PSO;
    if (auto Err = HR::toError(
            Device->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&PSO)),
            "Failed to create PSO."))
      return Err;

    return std::make_shared<DXPipelineState>(Name, RootSig, PSO);
  }

  llvm::Expected<std::shared_ptr<PipelineState>>
  createPipelineVsPs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                     llvm::ArrayRef<InputLayoutDesc> InputLayout,
                     llvm::ArrayRef<Format> RTFormats,
                     std::optional<Format> DSFormat, ShaderContainer VS,
                     ShaderContainer PS) override {
    assert(RTFormats.size() <= 8);

    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BindingsDesc, VS,
                                       /*IsGraphics=*/true, RootSig))
      return Err;

    std::vector<D3D12_INPUT_ELEMENT_DESC> DXInputLayout;
    DXInputLayout.reserve(InputLayout.size());
    for (const InputLayoutDesc &Elem : InputLayout) {
      D3D12_INPUT_ELEMENT_DESC ElementDesc = {};
      ElementDesc.SemanticName = Elem.Name.c_str();
      ElementDesc.SemanticIndex = 0;
      ElementDesc.Format = getDXGIFormat(Elem.Format);
      ElementDesc.InputSlot = 0;
      ElementDesc.AlignedByteOffset = Elem.OffsetInBytes;
      ElementDesc.InputSlotClass =
          Elem.InstanceStepRate ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
                                : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
      ElementDesc.InstanceDataStepRate = Elem.InstanceStepRate.value_or(0);
      DXInputLayout.push_back(ElementDesc);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.InputLayout = {DXInputLayout.data(), (UINT)DXInputLayout.size()};
    PSODesc.pRootSignature = RootSig.Get();
    PSODesc.VS = {VS.Shader->getBuffer().data(), VS.Shader->getBuffer().size()};
    PSODesc.PS = {PS.Shader->getBuffer().data(), PS.Shader->getBuffer().size()};
    if (PSODesc.VS.BytecodeLength == 0 || PSODesc.PS.BytecodeLength == 0)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Graphics pipeline requires both a vertex "
                                     "shader and a pixel shader.");

    PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    PSODesc.DepthStencilState.DepthEnable = false;
    PSODesc.DepthStencilState.StencilEnable = false;
    PSODesc.SampleMask = UINT_MAX;
    PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    PSODesc.NumRenderTargets = static_cast<UINT>(RTFormats.size());
    if (DSFormat)
      PSODesc.DSVFormat = getDXGIFormat(*DSFormat);
    for (size_t I = 0; I < RTFormats.size(); ++I)
      PSODesc.RTVFormats[I] = getDXGIFormat(RTFormats[I]);
    PSODesc.SampleDesc.Count = 1;

    ComPtr<ID3D12PipelineState> PSO;
    if (auto Err = HR::toError(
            Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)),
            "Failed to create graphics PSO."))
      return Err;

    return std::make_shared<DXPipelineState>(Name, RootSig, PSO);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override {
    return DXFence::create(Device.Get(), Name);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, BufferCreateDesc &Desc,
               size_t SizeInBytes) override {
    const D3D12_HEAP_TYPE HeapType = getDXHeapType(Desc.Location);

    const D3D12_RESOURCE_FLAGS Flags =
        HeapType == D3D12_HEAP_TYPE_READBACK
            ? D3D12_RESOURCE_FLAG_NONE
            : D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(HeapType);
    const D3D12_RESOURCE_DESC BufferDesc =
        CD3DX12_RESOURCE_DESC::Buffer(SizeInBytes, Flags);

    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
      InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (HeapType == D3D12_HEAP_TYPE_READBACK)
      // As per the readback heap docs
      // > Resources in this heap must be created with
      // > D3D12_RESOURCE_STATE_COPY_DEST, and cannot be changed away from this.
      InitialState = D3D12_RESOURCE_STATE_COPY_DEST;

    ComPtr<ID3D12Resource> DeviceBuffer;
    if (auto Err =
            HR::toError(Device->CreateCommittedResource(
                            &HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
                            InitialState, nullptr, IID_PPV_ARGS(&DeviceBuffer)),
                        "Failed to create buffer."))
      return Err;

    return std::make_unique<DXBuffer>(DeviceBuffer, Name, Desc, SizeInBytes);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Texture>>
  createTexture(std::string Name, TextureCreateDesc &Desc) override {
    if (auto Err = validateTextureCreateDesc(Desc))
      return Err;

    const D3D12_HEAP_PROPERTIES HeapProps =
        CD3DX12_HEAP_PROPERTIES(getDXHeapType(Desc.Location));

    D3D12_RESOURCE_DESC TexDesc = {};
    TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    TexDesc.Width = Desc.Width;
    TexDesc.Height = Desc.Height;
    TexDesc.DepthOrArraySize = 1;
    TexDesc.MipLevels = static_cast<UINT16>(Desc.MipLevels);
    TexDesc.Format = getDXGIFormat(Desc.Fmt);
    TexDesc.SampleDesc.Count = 1;
    TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    TexDesc.Flags = getDXResourceFlags(Desc.Usage);

    const D3D12_CLEAR_VALUE *ClearValuePtr = nullptr;
    D3D12_CLEAR_VALUE ClearValue = {};
    if (Desc.OptimizedClearValue) {
      ClearValue.Format = TexDesc.Format;
      std::visit(
          [&ClearValue](auto &&V) {
            using T = std::decay_t<decltype(V)>;
            if constexpr (std::is_same_v<T, ClearColor>) {
              ClearValue.Color[0] = V.R;
              ClearValue.Color[1] = V.G;
              ClearValue.Color[2] = V.B;
              ClearValue.Color[3] = V.A;
            } else {
              ClearValue.DepthStencil.Depth = V.Depth;
              ClearValue.DepthStencil.Stencil = V.Stencil;
            }
          },
          *Desc.OptimizedClearValue);
      ClearValuePtr = &ClearValue;
    }

    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    if ((Desc.Usage & TextureUsage::RenderTarget) != 0)
      InitialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    else if ((Desc.Usage & TextureUsage::DepthStencil) != 0)
      InitialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    ComPtr<ID3D12Resource> DeviceTexture;
    if (auto Err = HR::toError(Device->CreateCommittedResource(
                                   &HeapProps, D3D12_HEAP_FLAG_NONE, &TexDesc,
                                   InitialState, ClearValuePtr,
                                   IID_PPV_ARGS(&DeviceTexture)),
                               "Failed to create texture."))
      return Err;

    auto Tex = std::make_unique<DXTexture>(DeviceTexture, Name, Desc);

    const bool IsRT = (Desc.Usage & TextureUsage::RenderTarget) != 0;
    const bool IsDS = (Desc.Usage & TextureUsage::DepthStencil) != 0;
    if (IsRT) {
      auto HandleOrErr = RTVAllocator.allocate();
      if (!HandleOrErr)
        return HandleOrErr.takeError();
      Tex->RTVHandle = *HandleOrErr;
      Device->CreateRenderTargetView(DeviceTexture.Get(), nullptr,
                                     Tex->RTVHandle);
    }
    if (IsDS) {
      auto HandleOrErr = DSVAllocator.allocate();
      if (!HandleOrErr)
        return HandleOrErr.takeError();
      Tex->DSVHandle = *HandleOrErr;
      Device->CreateDepthStencilView(DeviceTexture.Get(), nullptr,
                                     Tex->DSVHandle);
    }

    return Tex;
  }

  static llvm::Expected<std::unique_ptr<offloadtest::Device>>
  create(ComPtr<IDXCoreAdapter> Adapter, const DeviceConfig &Config) {
    ComPtr<ID3D12Device> Device;
    if (auto Err =
            HR::toError(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                          IID_PPV_ARGS(&Device)),
                        "Failed to create D3D device"))
      return Err;

    static std::once_flag SignalHandlerRegistered;
    std::call_once(SignalHandlerRegistered, [] {
      llvm::sys::AddSignalHandler(dumpD3DInfoQueues, nullptr);
    });
    {
      const std::lock_guard<std::mutex> Lock(SignalHandlerMutex);
      SignalHandlerDevices.push_back(Device.Get());
    }

    assert(
        Adapter->IsPropertySupported(DXCoreAdapterProperty::DriverDescription));
    size_t BufferSize;
    Adapter->GetPropertySize(DXCoreAdapterProperty::DriverDescription,
                             &BufferSize);
    std::vector<char> DescVec(BufferSize);
    Adapter->GetProperty(DXCoreAdapterProperty::DriverDescription, BufferSize,
                         (void *)DescVec.data());
    if (Config.EnableDebugLayer || Config.EnableValidationLayer)
      if (auto Err = configureInfoQueue(Device.Get()))
        return Err;

    auto GraphicsQueueOrErr = DXQueue::createGraphicsQueue(Device);
    if (!GraphicsQueueOrErr)
      return GraphicsQueueOrErr.takeError();

    auto RTVHeapOrErr = DescriptorAllocator::create(
        Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
    if (!RTVHeapOrErr)
      return RTVHeapOrErr.takeError();

    auto DSVHeapOrErr = DescriptorAllocator::create(
        Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
    if (!DSVHeapOrErr)
      return DSVHeapOrErr.takeError();

    return std::make_unique<DXDevice>(
        Adapter, Device, std::move(*GraphicsQueueOrErr),
        std::move(*RTVHeapOrErr), std::move(*DSVHeapOrErr),
        std::string(DescVec.data()));
  }

  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      queryCapabilities();
    return Caps;
  }

  void queryCapabilities() {
    CD3DX12FeatureSupport Features;
    Features.Init(Device.Get());

#define D3D_FEATURE_BOOL(Name)                                                 \
  Caps.insert(                                                                 \
      std::make_pair(#Name, makeCapability<bool>(#Name, Features.Name())));

#define D3D_FEATURE_UINT(Name)                                                 \
  Caps.insert(std::make_pair(                                                  \
      #Name, makeCapability<uint32_t>(#Name, Features.Name())));

#define D3D_FEATURE_ENUM(NewEnum, Name)                                        \
  Caps.insert(std::make_pair(                                                  \
      #Name,                                                                   \
      makeCapability<NewEnum>(#Name, static_cast<NewEnum>(Features.Name()))));

#include "DXFeatures.def"
  }

  static llvm::Error configureInfoQueue(ID3D12Device *Device) {
#ifdef _WIN32
    ComPtr<ID3D12InfoQueue> InfoQueue;
    if (auto Err = HR::toError(Device->QueryInterface(InfoQueue.GetAddressOf()),
                               "Error initializing info queue"))
      return Err;
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
#endif
    return llvm::Error::success();
  }

  llvm::Error createDescriptorHeap(Pipeline &P, InvocationState &State) {
    if (P.getDescriptorCount() == 0)
      return llvm::Error::success();
    const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        P.getDescriptorCountWithFlattenedArrays(),
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    if (auto Err = HR::toError(Device->CreateDescriptorHeap(
                                   &HeapDesc, IID_PPV_ARGS(&State.DescHeap)),
                               "Failed to create descriptor heap."))
      return Err;
    return llvm::Error::success();
  }

  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override {
    return DXCommandBuffer::create(Device);
  }

  void addResourceUploadCommands(Resource &R, InvocationState &IS,
                                 ComPtr<ID3D12Resource> Destination,
                                 ComPtr<ID3D12Resource> Source) {
    addUploadBeginBarrier(IS, Destination);
    if (R.isTexture()) {
      const offloadtest::CPUBuffer &B = *R.BufferPtr;
      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
          0, CD3DX12_SUBRESOURCE_FOOTPRINT(
                 getDXFormat(B.Format, B.Channels), B.OutputProps.Width,
                 B.OutputProps.Height, 1,
                 B.OutputProps.Width * B.getElementSize())};
      const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(Destination.Get(), 0);
      const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(Source.Get(), Footprint);

      IS.CB->CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);
    } else
      IS.CB->CmdList->CopyBufferRegion(Destination.Get(), 0, Source.Get(), 0,
                                       R.size());
    addUploadEndBarrier(IS, Destination, R.isReadWrite());
  }

  static UINT getNumTiles(std::optional<uint32_t> NumTiles, uint32_t Width) {
    UINT Ret;
    if (NumTiles.has_value())
      Ret = static_cast<UINT>(*NumTiles);
    else {
      // Map the entire buffer by computing how many 64KB tiles cover it
      Ret = static_cast<UINT>(
          (Width + D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES - 1) /
          D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
      // check for overflow
      assert(Width < std::numeric_limits<UINT>::max() -
                         D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES - 1);
    }
    return Ret;
  }

  llvm::Error setupReservedResource(Resource &R,
                                    const D3D12_RESOURCE_DESC ResDesc,
                                    ComPtr<ID3D12Heap> &Heap,
                                    ComPtr<ID3D12Resource> &Buffer) {
    // Tile mapping setup (only skipped when TilesMapped is set to 0)
    const UINT NumTiles = getNumTiles(R.TilesMapped, ResDesc.Width);

    if (NumTiles == 0)
      return llvm::Error::success();

    // Create a Heap large enough for the mapped tiles
    D3D12_HEAP_DESC HeapDesc = {};
    HeapDesc.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    HeapDesc.SizeInBytes = static_cast<UINT64>(NumTiles) *
                           D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    HeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

    if (auto Err =
            HR::toError(Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(&Heap)),
                        "Failed to create heap for tiled SRV resource."))
      return Err;

    // Define one contiguous mapping region
    const D3D12_TILED_RESOURCE_COORDINATE StartCoord = {0, 0, 0, 0};
    D3D12_TILE_REGION_SIZE RegionSize = {};
    RegionSize.NumTiles = NumTiles;
    RegionSize.UseBox = FALSE;

    const D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
    const UINT HeapRangeStartOffset = 0;
    const UINT RangeTileCount = NumTiles;

    ID3D12CommandQueue *CommandQueue = GraphicsQueue.Queue.Get();
    CommandQueue->UpdateTileMappings(
        Buffer.Get(), 1, &StartCoord, &RegionSize, Heap.Get(), 1, &RangeFlag,
        &HeapRangeStartOffset, &RangeTileCount, D3D12_TILE_MAPPING_FLAG_NONE);

    // Synchronize after UpdateTileMappings, which is a queue operation (not
    // recorded into a command list).
    const uint64_t CurrentCounter = ++GraphicsQueue.FenceCounter;
    if (auto Err = HR::toError(
            CommandQueue->Signal(GraphicsQueue.SubmitFence->Fence.Get(),
                                 CurrentCounter),
            "Failed to add signal."))
      return Err;

    return GraphicsQueue.SubmitFence->waitForCompletion(CurrentCounter);
  }

  llvm::Expected<ResourceBundle> createSRV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;

    auto ResDescOrErr = getResourceDescription(R);
    if (!ResDescOrErr)
      return ResDescOrErr.takeError();
    const D3D12_RESOURCE_DESC ResDesc = *ResDescOrErr;
    const D3D12_HEAP_PROPERTIES UploadHeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC UploadResDesc =
        CD3DX12_RESOURCE_DESC::Buffer(R.size());

    uint32_t RegOffset = 0;

    for (const auto &ResData : R.BufferPtr->Data) {
      llvm::outs() << "Creating SRV: { Size = " << R.size() << ", Register = t"
                   << R.DXBinding.Register + RegOffset
                   << ", Space = " << R.DXBinding.Space;

      if (R.TilesMapped)
        llvm::outs() << ", TilesMapped = " << *R.TilesMapped;
      llvm::outs() << " }\n";

      ComPtr<ID3D12Resource> Buffer;
      if (R.IsReserved) {
        if (auto Err =
                HR::toError(Device->CreateReservedResource(
                                &ResDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                IID_PPV_ARGS(&Buffer)),
                            "Failed to create reserved resource (buffer)."))
          return Err;
      } else {
        // for committed resources
        const D3D12_HEAP_PROPERTIES CommittedResourceHeapProp =
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        if (auto Err = HR::toError(
                Device->CreateCommittedResource(&CommittedResourceHeapProp,
                                                D3D12_HEAP_FLAG_NONE, &ResDesc,
                                                D3D12_RESOURCE_STATE_COMMON,
                                                nullptr, IID_PPV_ARGS(&Buffer)),
                "Failed to create committed resource (buffer)."))
          return Err;
      }

      ComPtr<ID3D12Resource> UploadBuffer;
      if (auto Err = HR::toError(
              Device->CreateCommittedResource(
                  &UploadHeapProp, D3D12_HEAP_FLAG_NONE, &UploadResDesc,
                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                  IID_PPV_ARGS(&UploadBuffer)),
              "Failed to create committed resource (upload buffer)."))
        return Err;

      ComPtr<ID3D12Heap> Heap; // optional, only created if NumTiles > 0
      if (R.IsReserved)
        if (auto Err = setupReservedResource(R, ResDesc, Heap, Buffer))
          return Err;

      // Upload data initialization
      void *ResDataPtr = nullptr;
      if (SUCCEEDED(UploadBuffer->Map(0, NULL, &ResDataPtr))) {
        memcpy(ResDataPtr, ResData.get(), R.size());
        UploadBuffer->Unmap(0, nullptr);
      } else {
        return llvm::createStringError(std::errc::io_error,
                                       "Failed to map SRV upload buffer.");
      }

      addResourceUploadCommands(R, IS, Buffer, UploadBuffer);

      Bundle.emplace_back(UploadBuffer, Buffer, nullptr, Heap);
      RegOffset++;
    }
    return Bundle;
  }

  // returns the next available HeapIdx
  uint32_t bindSRV(Resource &R, InvocationState &IS, uint32_t HeapIdx,
                   ResourceBundle ResBundle) {
    const uint32_t EltSize = R.getElementSize();
    const uint32_t NumElts = R.size() / EltSize;
    const D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = getSRVDescription(R);
    const uint32_t DescHandleIncSize = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const D3D12_CPU_DESCRIPTOR_HANDLE SRVHandleHeapStart =
        IS.DescHeap->GetCPUDescriptorHandleForHeapStart();

    for (const ResourceSet &RS : ResBundle) {
      llvm::outs() << "SRV: HeapIdx = " << HeapIdx << " EltSize = " << EltSize
                   << " NumElts = " << NumElts << "\n";
      D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = SRVHandleHeapStart;
      SRVHandle.ptr += HeapIdx * DescHandleIncSize;
      Device->CreateShaderResourceView(RS.Buffer.Get(), &SRVDesc, SRVHandle);
      HeapIdx++;
    }
    return HeapIdx;
  }

  llvm::Expected<ResourceBundle> createUAV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;
    const uint32_t BufferSize = getUAVBufferSize(R);

    auto ResDescOrErr = getResourceDescription(R);
    if (!ResDescOrErr)
      return ResDescOrErr.takeError();
    const D3D12_RESOURCE_DESC ResDesc = *ResDescOrErr;

    const D3D12_HEAP_PROPERTIES ReadBackHeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    const D3D12_RESOURCE_DESC ReadBackResDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        BufferSize,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE};

    const D3D12_HEAP_PROPERTIES UploadHeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC UploadResDesc =
        CD3DX12_RESOURCE_DESC::Buffer(BufferSize);

    uint32_t RegOffset = 0;

    for (const auto &ResData : R.BufferPtr->Data) {
      llvm::outs() << "Creating UAV: { Size = " << BufferSize
                   << ", Register = u" << R.DXBinding.Register + RegOffset
                   << ", Space = " << R.DXBinding.Space
                   << ", HasCounter = " << R.HasCounter;

      if (R.TilesMapped)
        llvm::outs() << ", TilesMapped = " << *R.TilesMapped;
      llvm::outs() << " }\n";

      ComPtr<ID3D12Resource> Buffer;
      if (R.IsReserved) {
        if (auto Err =
                HR::toError(Device->CreateReservedResource(
                                &ResDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                IID_PPV_ARGS(&Buffer)),
                            "Failed to create reserved resource (buffer)."))
          return Err;
      } else {
        // for committed resources
        const D3D12_HEAP_PROPERTIES CommittedResourceHeapProp =
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        if (auto Err = HR::toError(
                Device->CreateCommittedResource(&CommittedResourceHeapProp,
                                                D3D12_HEAP_FLAG_NONE, &ResDesc,
                                                D3D12_RESOURCE_STATE_COMMON,
                                                nullptr, IID_PPV_ARGS(&Buffer)),
                "Failed to create committed resource (buffer)."))
          return Err;
      }

      ComPtr<ID3D12Resource> UploadBuffer;
      if (auto Err = HR::toError(
              Device->CreateCommittedResource(
                  &UploadHeapProp, D3D12_HEAP_FLAG_NONE, &UploadResDesc,
                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                  IID_PPV_ARGS(&UploadBuffer)),
              "Failed to create committed resource (upload buffer)."))
        return Err;

      // Committed readback buffer
      ComPtr<ID3D12Resource> ReadBackBuffer;
      if (auto Err = HR::toError(
              Device->CreateCommittedResource(
                  &ReadBackHeapProp, D3D12_HEAP_FLAG_NONE, &ReadBackResDesc,
                  D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                  IID_PPV_ARGS(&ReadBackBuffer)),
              "Failed to create committed resource (readback buffer)."))
        return Err;

      ComPtr<ID3D12Heap> Heap; // optional, only created if NumTiles > 0
      if (R.IsReserved)
        if (auto Err = setupReservedResource(R, ResDesc, Heap, Buffer))
          return Err;

      // Upload data initialization
      void *ResDataPtr = nullptr;
      if (SUCCEEDED(UploadBuffer->Map(0, NULL, &ResDataPtr))) {
        memcpy(ResDataPtr, ResData.get(), R.size());
        UploadBuffer->Unmap(0, nullptr);
      } else {
        return llvm::createStringError(std::errc::io_error,
                                       "Failed to map UAV upload buffer.");
      }

      addResourceUploadCommands(R, IS, Buffer, UploadBuffer);

      Bundle.emplace_back(UploadBuffer, Buffer, ReadBackBuffer, Heap);
      RegOffset++;
    }
    return Bundle;
  }

  // returns the next available HeapIdx
  uint32_t bindUAV(Resource &R, InvocationState &IS, uint32_t HeapIdx,
                   ResourceBundle ResBundle) {
    const uint32_t EltSize = R.getElementSize();
    const uint32_t NumElts = R.size() / EltSize;
    const D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = getUAVDescription(R);
    const uint32_t DescHandleIncSize = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const D3D12_CPU_DESCRIPTOR_HANDLE UAVHandleHeapStart =
        IS.DescHeap->GetCPUDescriptorHandleForHeapStart();

    for (const ResourceSet &RS : ResBundle) {
      llvm::outs() << "UAV: HeapIdx = " << HeapIdx << " EltSize = " << EltSize
                   << " NumElts = " << NumElts
                   << " HasCounter = " << R.HasCounter << "\n";

      D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle = UAVHandleHeapStart;
      UAVHandle.ptr += HeapIdx * DescHandleIncSize;
      ID3D12Resource *CounterBuffer = R.HasCounter ? RS.Buffer.Get() : nullptr;
      Device->CreateUnorderedAccessView(RS.Buffer.Get(), CounterBuffer,
                                        &UAVDesc, UAVHandle);
      HeapIdx++;
    }
    return HeapIdx;
  }

  static size_t getCBVSize(size_t Sz) {
    return (Sz + 255u) & 0xFFFFFFFFFFFFFF00;
  }

  llvm::Expected<ResourceBundle> createCBV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;

    const size_t CBVSize = getCBVSize(R.size());
    const D3D12_HEAP_PROPERTIES HeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_RESOURCE_DESC ResDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        CBVSize,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        {1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS};

    const D3D12_HEAP_PROPERTIES UploadHeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC UploadResDesc =
        CD3DX12_RESOURCE_DESC::Buffer(CBVSize);

    uint32_t RegOffset = 0;
    for (const auto &ResData : R.BufferPtr->Data) {
      llvm::outs() << "Creating CBV: { Size = " << CBVSize << ", Register = b"
                   << R.DXBinding.Register + RegOffset
                   << ", Space = " << R.DXBinding.Space << " }\n";

      ComPtr<ID3D12Resource> Buffer;
      if (auto Err = HR::toError(
              Device->CreateCommittedResource(
                  &HeapProp, D3D12_HEAP_FLAG_NONE, &ResDesc,
                  D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&Buffer)),
              "Failed to create committed resource (buffer)."))
        return Err;

      ComPtr<ID3D12Resource> UploadBuffer;
      if (auto Err = HR::toError(
              Device->CreateCommittedResource(
                  &UploadHeapProp, D3D12_HEAP_FLAG_NONE, &UploadResDesc,
                  D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                  IID_PPV_ARGS(&UploadBuffer)),
              "Failed to create committed resource (upload buffer)."))
        return Err;

      // Initialize the CBV data
      void *ResDataPtr = nullptr;
      if (auto Err = HR::toError(UploadBuffer->Map(0, nullptr, &ResDataPtr),
                                 "Failed to acquire UAV data pointer."))
        return Err;
      memset(ResDataPtr, 0, CBVSize);
      memcpy(ResDataPtr, ResData.get(), R.size());

      UploadBuffer->Unmap(0, nullptr);

      addResourceUploadCommands(R, IS, Buffer, UploadBuffer);

      Bundle.emplace_back(UploadBuffer, Buffer, nullptr);
      RegOffset++;
    }
    return Bundle;
  }

  // returns the next available HeapIdx
  uint32_t bindCBV(Resource &R, InvocationState &IS, uint32_t HeapIdx,
                   ResourceBundle ResBundle) {
    const size_t CBVSize = getCBVSize(R.size());
    const uint32_t DescHandleIncSize = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const D3D12_CPU_DESCRIPTOR_HANDLE CVBHandleHeapStart =
        IS.DescHeap->GetCPUDescriptorHandleForHeapStart();

    for (const ResourceSet &RS : ResBundle) {
      llvm::outs() << "CBV: HeapIdx = " << HeapIdx << " Size = " << CBVSize
                   << "\n";
      const D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {
          RS.Buffer->GetGPUVirtualAddress(), static_cast<uint32_t>(CBVSize)};
      D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle = CVBHandleHeapStart;
      CBVHandle.ptr += HeapIdx * DescHandleIncSize;
      Device->CreateConstantBufferView(&CBVDesc, CBVHandle);
      HeapIdx++;
    }
    return HeapIdx;
  }

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    auto CreateBuffer =
        [&IS,
         this](Resource &R,
               llvm::SmallVectorImpl<ResourcePair> &Resources) -> llvm::Error {
      switch (getDXKind(R.Kind)) {
      case SRV: {
        auto ExRes = createSRV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.push_back(std::make_pair(&R, *ExRes));
        break;
      }
      case UAV: {
        auto ExRes = createUAV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.push_back(std::make_pair(&R, *ExRes));
        break;
      }
      case CBV: {
        auto ExRes = createCBV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.push_back(std::make_pair(&R, *ExRes));
        break;
      }
      case SAMPLER:
        return llvm::createStringError(
            std::errc::not_supported,
            "Samplers are not yet implemented for DirectX.");
      }
      return llvm::Error::success();
    };

    for (auto &D : P.Sets) {
      IS.DescTables.emplace_back(DescriptorTable());
      DescriptorTable &Table = IS.DescTables.back();
      for (auto &R : D.Resources)
        if (auto Err = CreateBuffer(R, Table.Resources))
          return Err;
    }

    // Bind descriptors in descriptor tables.
    uint32_t HeapIndex = 0;
    for (auto &T : IS.DescTables) {
      for (auto &R : T.Resources) {
        switch (getDXKind(R.first->Kind)) {
        case SRV:
          HeapIndex = bindSRV(*(R.first), IS, HeapIndex, R.second);
          break;
        case UAV:
          HeapIndex = bindUAV(*(R.first), IS, HeapIndex, R.second);
          break;
        case CBV:
          HeapIndex = bindCBV(*(R.first), IS, HeapIndex, R.second);
          break;
        case SAMPLER:
          llvm_unreachable("Not implemented yet.");
        }
      }
    }

    // Setup root descriptors
    for (auto &R : P.Settings.DX.RootParams) {
      if (R.Kind != dx::RootParamKind::RootDescriptor)
        continue;
      auto &Resource = std::get<dx::RootResource>(R.Data);
      if (!Resource.IsReserved && Resource.TilesMapped.has_value()) {
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Error: Cannot define tiles mapped without declaring resource as "
            "reserved.");
      }
      if (auto Err = CreateBuffer(Resource, IS.RootResources))
        return Err;
    }
    return llvm::Error::success();
  }

  void addUploadBeginBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R) {
    const D3D12_RESOURCE_BARRIER Barrier = {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        D3D12_RESOURCE_BARRIER_FLAG_NONE,
        {D3D12_RESOURCE_TRANSITION_BARRIER{
            R.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST}}};
    IS.CB->CmdList->ResourceBarrier(1, &Barrier);
  }

  void addUploadEndBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R,
                           bool IsUAV) {
    const D3D12_RESOURCE_BARRIER Barrier = {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        D3D12_RESOURCE_BARRIER_FLAG_NONE,
        {D3D12_RESOURCE_TRANSITION_BARRIER{
            R.Get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_STATE_COPY_DEST,
            IsUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
                  : D3D12_RESOURCE_STATE_GENERIC_READ}}};
    IS.CB->CmdList->ResourceBarrier(1, &Barrier);
  }

  void addReadbackBeginBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R) {
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        R.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    IS.CB->CmdList->ResourceBarrier(1, &Barrier);
  }

  void addReadbackEndBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R) {
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        R.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    IS.CB->CmdList->ResourceBarrier(1, &Barrier);
  }

  llvm::Expected<offloadtest::SubmitResult>
  executeCommandList(InvocationState &IS) {
    return GraphicsQueue.submit(std::move(IS.CB));
  }

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    CD3DX12_GPU_DESCRIPTOR_HANDLE Handle;
    if (IS.DescHeap) {
      ID3D12DescriptorHeap *const Heaps[] = {IS.DescHeap.Get()};
      IS.CB->CmdList->SetDescriptorHeaps(1, Heaps);
      Handle = IS.DescHeap->GetGPUDescriptorHandleForHeapStart();
    }
    const DXPipelineState *DXPipeline =
        static_cast<const DXPipelineState *>(IS.Pipeline.get());
    IS.CB->CmdList->SetComputeRootSignature(DXPipeline->RootSig.Get());
    IS.CB->CmdList->SetPipelineState(DXPipeline->PSO.Get());

    const uint32_t Inc = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    if (P.Settings.DX.RootParams.size() > 0) {
      uint32_t ConstantOffset = 0u;
      uint32_t RootParamIndex = 0u;
      uint32_t DescriptorTableIndex = 0u;
      auto *RootDescIt = IS.RootResources.begin();
      for (const auto &Param : P.Settings.DX.RootParams) {
        switch (Param.Kind) {
        case dx::RootParamKind::Constant: {
          auto &Constant = std::get<dx::RootConstant>(Param.Data);
          if (Constant.BufferPtr->ArraySize != 1)
            return llvm::createStringError(
                std::errc::value_too_large,
                "Root constant cannot refer to resource arrays.");
          const uint32_t NumValues =
              Constant.BufferPtr->size() / sizeof(uint32_t);
          IS.CB->CmdList->SetComputeRoot32BitConstants(
              RootParamIndex++, NumValues,
              Constant.BufferPtr->Data.back().get(), ConstantOffset);
          ConstantOffset += NumValues;
          break;
        }
        case dx::RootParamKind::DescriptorTable:
          IS.CB->CmdList->SetComputeRootDescriptorTable(RootParamIndex++,
                                                        Handle);
          Handle.Offset(P.Sets[DescriptorTableIndex++].Resources.size(), Inc);
          break;
        case dx::RootParamKind::RootDescriptor:
          assert(RootDescIt != IS.RootResources.end());
          if (RootDescIt->first->getArraySize() != 1)
            return llvm::createStringError(
                std::errc::value_too_large,
                "Root descriptor cannot refer to resource arrays.");
          switch (getDXKind(RootDescIt->first->Kind)) {
          case SRV:
            IS.CB->CmdList->SetComputeRootShaderResourceView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case UAV:
            IS.CB->CmdList->SetComputeRootUnorderedAccessView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case CBV:
            IS.CB->CmdList->SetComputeRootConstantBufferView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case SAMPLER:
            llvm_unreachable("Not implemented yet.");
          }
          ++RootDescIt;
          break;
        }
      }
    } else {
      // If no explicit root parameters are provided, fall back to using the
      // descriptor set layout. This is to make it easier to write tests that
      // don't need complicated root signatures.
      for (uint32_t Idx = 0u; Idx < P.Sets.size(); ++Idx) {
        IS.CB->CmdList->SetComputeRootDescriptorTable(Idx, Handle);
        Handle.Offset(P.Sets[Idx].Resources.size(), Inc);
      }
    }

    const llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);

    IS.CB->CmdList->Dispatch(DispatchSize[0], DispatchSize[1], DispatchSize[2]);

    auto CopyBackResource = [&IS, this](ResourcePair &R) {
      if (R.first->isTexture()) {
        const offloadtest::CPUBuffer &B = *R.first->BufferPtr;
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
            0, CD3DX12_SUBRESOURCE_FOOTPRINT(
                   getDXFormat(B.Format, B.Channels), B.OutputProps.Width,
                   B.OutputProps.Height, 1,
                   B.OutputProps.Width * B.getElementSize())};
        for (const ResourceSet &RS : R.second) {
          if (RS.Readback == nullptr)
            continue;
          addReadbackBeginBarrier(IS, RS.Buffer);
          const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(RS.Readback.Get(),
                                                     Footprint);
          const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(RS.Buffer.Get(), 0);
          IS.CB->CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);
          addReadbackEndBarrier(IS, RS.Buffer);
        }
        return;
      }
      for (const ResourceSet &RS : R.second) {
        if (RS.Readback == nullptr)
          continue;
        addReadbackBeginBarrier(IS, RS.Buffer);
        IS.CB->CmdList->CopyResource(RS.Readback.Get(), RS.Buffer.Get());
        addReadbackEndBarrier(IS, RS.Buffer);
      }
    };

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        CopyBackResource(R);

    for (auto &R : IS.RootResources)
      CopyBackResource(R);

    return llvm::Error::success();
  }

  llvm::Error readBack(Pipeline &P, InvocationState &IS) {
    auto MemCpyBack = [](ResourcePair &R) -> llvm::Error {
      if (!R.first->isReadWrite())
        return llvm::Error::success();

      auto *RSIt = R.second.begin();
      auto *DataIt = R.first->BufferPtr->Data.begin();
      for (; RSIt != R.second.end() && DataIt != R.first->BufferPtr->Data.end();
           ++RSIt, ++DataIt) {
        void *DataPtr;
        if (auto Err = HR::toError(RSIt->Readback->Map(0, nullptr, &DataPtr),
                                   "Failed to map result."))
          return Err;
        memcpy(DataIt->get(), DataPtr, R.first->size());

        if (R.first->HasCounter) {
          uint32_t Counter;
          memcpy(&Counter,
                 static_cast<char *>(DataPtr) +
                     getUAVBufferCounterOffset(*R.first),
                 sizeof(uint32_t));
          R.first->BufferPtr->Counters.push_back(Counter);
        }
        RSIt->Readback->Unmap(0, nullptr);
      }

      return llvm::Error::success();
    };

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        if (auto Err = MemCpyBack(R))
          return Err;

    for (auto &R : IS.RootResources)
      if (auto Err = MemCpyBack(R))
        return Err;

    // If there is no render target, return early.
    if (!IS.RTReadback)
      return llvm::Error::success();

    // Map readback and copy into host buffer, accounting for row pitch and
    // flipping vertical orientation. DirectX render target origin is top-left,
    // while our image writer expects bottom-left.
    const CPUBuffer &B = *P.Bindings.RTargetBufferPtr;
    void *Mapped = nullptr;
    auto &Readback = llvm::cast<DXBuffer>(*IS.RTReadback);
    if (auto Err = HR::toError(Readback.Buffer->Map(0, nullptr, &Mapped),
                               "Failed to map render target readback"))
      return Err;

    // Query the copy footprint to get the actual padded row pitch used by the
    // copy operation.
    auto &RT = llvm::cast<DXTexture>(*IS.RT);
    const D3D12_RESOURCE_DESC RTDesc = RT.Resource->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Placed = {};
    uint32_t NumRows = 0;
    uint64_t RowSizeInBytes = 0;
    uint64_t TotalBytes = 0;
    Device->GetCopyableFootprints(&RTDesc, 0u, 1u, 0u, &Placed, &NumRows,
                                  &RowSizeInBytes, &TotalBytes);

    const uint32_t RowPitch = Placed.Footprint.RowPitch;
    const uint32_t RowBytes =
        static_cast<uint32_t>(B.getElementSize() * B.OutputProps.Width);
    const uint32_t Height = static_cast<uint32_t>(B.OutputProps.Height);

    const uint8_t *SrcBase = reinterpret_cast<uint8_t *>(Mapped);
    uint8_t *DstBase =
        reinterpret_cast<uint8_t *>(P.Bindings.RTargetBufferPtr->Data[0].get());

    // Copy rows in reverse order.
    for (uint32_t Y = 0; Y < Height; ++Y) {
      const uint8_t *SrcRow = SrcBase + static_cast<size_t>(Y) * RowPitch;
      uint8_t *DstRow =
          DstBase + static_cast<size_t>(Height - 1 - Y) * RowBytes;
      memcpy(DstRow, SrcRow, RowBytes);
    }

    Readback.Buffer->Unmap(0, nullptr);
    return llvm::Error::success();
  }

  llvm::Error createRenderTarget(Pipeline &P, InvocationState &IS) {
    if (!P.Bindings.RTargetBufferPtr)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "No render target bound for graphics pipeline.");
    const CPUBuffer &OutBuf = *P.Bindings.RTargetBufferPtr;

    auto TexOrErr = offloadtest::createRenderTargetFromCPUBuffer(*this, OutBuf);
    if (!TexOrErr)
      return TexOrErr.takeError();

    IS.RT = std::move(*TexOrErr);

    // Create readback buffer sized for the pixel data (raw bytes).
    BufferCreateDesc BufDesc = {};
    BufDesc.Location = MemoryLocation::GpuToCpu;
    auto BufOrErr = createBuffer("RTReadback", BufDesc, OutBuf.size());
    if (!BufOrErr)
      return BufOrErr.takeError();
    IS.RTReadback = std::move(*BufOrErr);

    return llvm::Error::success();
  }

  llvm::Error createDepthStencil(Pipeline &P, InvocationState &IS) {
    auto TexOrErr = offloadtest::createDefaultDepthStencilTarget(
        *this, P.Bindings.RTargetBufferPtr->OutputProps.Width,
        P.Bindings.RTargetBufferPtr->OutputProps.Height);
    if (!TexOrErr)
      return TexOrErr.takeError();
    IS.DS = std::move(*TexOrErr);
    return llvm::Error::success();
  }

  llvm::Error createVertexBuffer(Pipeline &P, InvocationState &IS) {
    if (!P.Bindings.VertexBufferPtr)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "No vertex buffer bound for graphics pipeline.");
    const CPUBuffer &VB = *P.Bindings.VertexBufferPtr;
    const uint64_t VBSize = VB.size();
    D3D12_RESOURCE_DESC const Desc = CD3DX12_RESOURCE_DESC::Buffer(VBSize);
    CD3DX12_HEAP_PROPERTIES HeapProps =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    if (auto Err = HR::toError(Device->CreateCommittedResource(
                                   &HeapProps, D3D12_HEAP_FLAG_NONE, &Desc,
                                   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                   IID_PPV_ARGS(&IS.VB)),
                               "Failed to create vertex buffer"))
      return Err;

    void *Ptr = nullptr;
    if (auto Err = HR::toError(IS.VB->Map(0, nullptr, &Ptr),
                               "Failed to map vertex buffer"))
      return Err;
    memcpy(Ptr, VB.Data[0].get(), VBSize);
    IS.VB->Unmap(0, nullptr);

    D3D12_VERTEX_BUFFER_VIEW VBView = {};
    VBView.BufferLocation = IS.VB->GetGPUVirtualAddress();
    VBView.SizeInBytes = static_cast<UINT>(VBSize);
    VBView.StrideInBytes = P.Bindings.getVertexStride();

    IS.CB->CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    IS.CB->CmdList->IASetVertexBuffers(0, 1, &VBView);

    return llvm::Error::success();
  }

  llvm::Error createGraphicsCommands(Pipeline &P, InvocationState &IS) {
    auto &RT = llvm::cast<DXTexture>(*IS.RT);
    auto &DS = llvm::cast<DXTexture>(*IS.DS);
    auto &RTReadback = llvm::cast<DXBuffer>(*IS.RTReadback);

    const DXPipelineState *DXPipeline =
        static_cast<const DXPipelineState *>(IS.Pipeline.get());
    IS.CB->CmdList->SetGraphicsRootSignature(DXPipeline->RootSig.Get());
    if (IS.DescHeap) {
      ID3D12DescriptorHeap *const Heaps[] = {IS.DescHeap.Get()};
      IS.CB->CmdList->SetDescriptorHeaps(1, Heaps);
      IS.CB->CmdList->SetGraphicsRootDescriptorTable(
          0, IS.DescHeap->GetGPUDescriptorHandleForHeapStart());
    }
    IS.CB->CmdList->SetPipelineState(DXPipeline->PSO.Get());

    IS.CB->CmdList->OMSetRenderTargets(1, &RT.RTVHandle, false, &DS.DSVHandle);

    const auto *DepthCV =
        std::get_if<ClearDepthStencil>(&*DS.Desc.OptimizedClearValue);
    if (!DepthCV)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Depth/stencil clear value must be a ClearDepthStencil.");
    IS.CB->CmdList->ClearDepthStencilView(
        DS.DSVHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        DepthCV->Depth, DepthCV->Stencil, 0, nullptr);

    D3D12_VIEWPORT VP = {};
    VP.Width =
        static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Width);
    VP.Height =
        static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Height);
    VP.MinDepth = 0.0f;
    VP.MaxDepth = 1.0f;
    VP.TopLeftX = 0.0f;
    VP.TopLeftY = 0.0f;
    IS.CB->CmdList->RSSetViewports(1, &VP);
    const D3D12_RECT Scissor = {0, 0, static_cast<LONG>(VP.Width),
                                static_cast<LONG>(VP.Height)};
    IS.CB->CmdList->RSSetScissorRects(1, &Scissor);

    IS.CB->CmdList->DrawInstanced(P.Bindings.getVertexCount(), 1, 0, 0);

    // Transition the render target to copy source and copy to the readback
    // buffer.
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        RT.Resource.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    IS.CB->CmdList->ResourceBarrier(1, &Barrier);

    const CPUBuffer &B = *P.Bindings.RTargetBufferPtr;
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
        0,
        CD3DX12_SUBRESOURCE_FOOTPRINT(
            getDXFormat(B.Format, B.Channels), B.OutputProps.Width,
            B.OutputProps.Height, 1, B.OutputProps.Width * B.getElementSize())};
    const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(RTReadback.Buffer.Get(),
                                               Footprint);
    const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(RT.Resource.Get(), 0);

    IS.CB->CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);

    auto CopyBackResource = [&IS, this](ResourcePair &R) {
      if (R.first->isTexture()) {
        const offloadtest::CPUBuffer &B = *R.first->BufferPtr;
        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
            0, CD3DX12_SUBRESOURCE_FOOTPRINT(
                   getDXFormat(B.Format, B.Channels), B.OutputProps.Width,
                   B.OutputProps.Height, 1,
                   B.OutputProps.Width * B.getElementSize())};
        for (const ResourceSet &RS : R.second) {
          if (RS.Readback == nullptr)
            continue;
          addReadbackBeginBarrier(IS, RS.Buffer);
          const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(RS.Readback.Get(),
                                                     Footprint);
          const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(RS.Buffer.Get(), 0);
          IS.CB->CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);
          addReadbackEndBarrier(IS, RS.Buffer);
        }
        return;
      }
      for (const ResourceSet &RS : R.second) {
        if (RS.Readback == nullptr)
          continue;
        addReadbackBeginBarrier(IS, RS.Buffer);
        IS.CB->CmdList->CopyResource(RS.Readback.Get(), RS.Buffer.Get());
        addReadbackEndBarrier(IS, RS.Buffer);
      }
    };

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        CopyBackResource(R);

    for (auto &R : IS.RootResources)
      CopyBackResource(R);

    return llvm::Error::success();
  }

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState State;
    llvm::outs() << "Configuring execution on device: " << Description << "\n";
    if (auto Err = createDescriptorHeap(P, State))
      return Err;
    llvm::outs() << "Descriptor heap created.\n";

    auto CBOrErr = DXCommandBuffer::create(Device);
    if (!CBOrErr)
      return CBOrErr.takeError();
    State.CB = std::move(*CBOrErr);
    llvm::outs() << "Command buffer created.\n";

    if (auto Err = createBuffers(P, State))
      return Err;
    llvm::outs() << "Buffers created.\n";

    BindingsDesc BindingsDesc = {};
    for (auto &S : P.Sets) {
      DescriptorSetLayoutDesc Layout;
      for (auto &R : S.Resources) {
        ResourceBindingDesc ResourceBinding = {};
        ResourceBinding.Kind = R.Kind;
        ResourceBinding.DXBinding.Register = R.DXBinding.Register;
        ResourceBinding.DXBinding.Space = R.DXBinding.Space;
        ResourceBinding.DescriptorCount = R.getArraySize();

        Layout.ResourceBindings.push_back(ResourceBinding);
      }

      BindingsDesc.DescriptorSetDescs.push_back(Layout);
    }

    if (P.isCompute()) {
      // This is an arbitrary distinction that we could alter in the future.
      if (P.Shaders.size() != 1 || P.Shaders[0].Stage != Stages::Compute)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Compute pipeline must have exactly one compute shader.");

      ShaderContainer CS = {};
      CS.EntryPoint = P.Shaders[0].Entry;
      CS.Shader = P.Shaders[0].Shader.get();

      auto PipelineStateOrErr =
          createPipelineCs("Compute Pipeline State", BindingsDesc, CS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = *PipelineStateOrErr;
      llvm::outs() << "Compute Pipeline created.\n";
      if (auto Err = createComputeCommands(P, State))
        return Err;
      llvm::outs() << "Compute command list created.\n";

    } else {
      // Create render target, depth/stencil, readback and vertex buffer and
      // PSO.
      if (auto Err = createRenderTarget(P, State))
        return Err;
      llvm::outs() << "Render target created.\n";
      // TODO: Always created for graphics pipelines. Consider making this
      // conditional on the pipeline definition.
      if (auto Err = createDepthStencil(P, State))
        return Err;
      llvm::outs() << "Depth stencil created.\n";
      if (auto Err = createVertexBuffer(P, State))
        return Err;
      llvm::outs() << "Vertex buffer created.\n";

      ShaderContainer VS = {};
      ShaderContainer PS = {};
      for (auto &Shader : P.Shaders) {
        if (Shader.Stage == Stages::Vertex) {
          VS.EntryPoint = Shader.Entry;
          VS.Shader = Shader.Shader.get();
        } else if (Shader.Stage == Stages::Pixel) {
          PS.EntryPoint = Shader.Entry;
          PS.Shader = Shader.Shader.get();
        }
      }

      // Create the input layout based on the vertex attributes.
      llvm::SmallVector<InputLayoutDesc> InputLayout;
      for (auto &Attr : P.Bindings.VertexAttributes) {
        auto FormatOrErr = toFormat(Attr.Format, Attr.Channels);
        if (!FormatOrErr)
          return FormatOrErr.takeError();

        InputLayoutDesc Desc = {};
        Desc.Name = Attr.Name;
        Desc.Format = *FormatOrErr;
        Desc.OffsetInBytes = Attr.Offset;
        InputLayout.push_back(Desc);
      }

      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();

      llvm::SmallVector<Format> RTFormats;
      RTFormats.push_back(*FormatOrErr);

      auto PipelineStateOrErr = createPipelineVsPs(
          "Graphics Pipeline State", BindingsDesc, InputLayout, RTFormats,
          Format::D32FloatS8Uint, VS, PS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = *PipelineStateOrErr;
      llvm::outs() << "Graphics Pipeline created.\n";
      if (auto Err = createGraphicsCommands(P, State))
        return Err;
      llvm::outs() << "Graphics command list created complete.\n";
    }

    auto SubmitResult = executeCommandList(State);
    if (!SubmitResult)
      return SubmitResult.takeError();
    llvm::outs() << "Compute commands executed.\n";
    if (auto Err = SubmitResult->waitForCompletion())
      return Err;
    if (auto Err = readBack(P, State))
      return Err;
    llvm::outs() << "Read data back.\n";

    return llvm::Error::success();
  }
};
} // namespace

llvm::Expected<offloadtest::SubmitResult> DXQueue::submit(
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs) {
  // Non-blocking: query how far the GPU has progressed and release
  // command buffers from completed submissions.
  {
    const uint64_t Completed = SubmitFence->getFenceValue();
    llvm::erase_if(InFlightBatches, [Completed](const InFlightBatch &B) {
      return B.FenceValue <= Completed;
    });
  }

  llvm::SmallVector<ID3D12CommandList *> CmdLists;
  CmdLists.reserve(CBs.size());

  // GPU-side wait so that back-to-back submits don't overlap on the GPU.
  // Skip on first submit since Wait(fence, 0) triggers a D3D12 validation
  // warning.
  if (FenceCounter > 0)
    if (auto Err =
            HR::toError(Queue->Wait(SubmitFence->Fence.Get(), FenceCounter),
                        "Failed to wait on previous submit."))
      return Err;

  for (auto &CB : CBs) {
    auto &DCB = *llvm::cast<DXCommandBuffer>(CB.get());
    if (auto Err =
            HR::toError(DCB.CmdList->Close(), "Failed to close command list."))
      return Err;
    CmdLists.push_back(DCB.CmdList.Get());
  }

  Queue->ExecuteCommandLists(CmdLists.size(), CmdLists.data());

  const uint64_t CurrentCounter = ++FenceCounter;
  if (auto Err =
          HR::toError(Queue->Signal(SubmitFence->Fence.Get(), CurrentCounter),
                      "Failed to add signal."))
    return Err;

  // Keep submitted command buffers alive until the GPU is done with them.
  InFlightBatches.push_back({CurrentCounter, std::move(CBs)});

  return offloadtest::SubmitResult{SubmitFence.get(), CurrentCounter};
}

llvm::Error offloadtest::initializeDX12Devices(
    const DeviceConfig Config,
    llvm::SmallVectorImpl<std::unique_ptr<Device>> &Devices) {
#ifdef _WIN32
  if (Config.EnableDebugLayer || Config.EnableValidationLayer) {
    ComPtr<ID3D12Debug1> Debug1;

    if (auto Err = HR::toError(D3D12GetDebugInterface(IID_PPV_ARGS(&Debug1)),
                               "failed to create D3D12 Debug Interface"))
      return Err;

    Debug1->EnableDebugLayer();
    Debug1->SetEnableGPUBasedValidation(Config.EnableValidationLayer);
  }
#endif

  ComPtr<IDXCoreAdapterFactory> Factory;
  if (auto Err = HR::toError(DXCoreCreateAdapterFactory(Factory.GetAddressOf()),
                             "Failed to create DXCore Adapter Factory")) {
    return Err;
  }

  ComPtr<IDXCoreAdapterList> AdapterList;
  GUID AdapterAttributes[]{DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE,
                           DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS};
  if (auto Err = HR::toError(Factory->CreateAdapterList(
                                 _countof(AdapterAttributes), AdapterAttributes,
                                 AdapterList.GetAddressOf()),
                             "Failed to acquire a list of adapters")) {
    return Err;
  }

  for (unsigned AdapterIndex = 0; AdapterIndex < AdapterList->GetAdapterCount();
       ++AdapterIndex) {
    ComPtr<IDXCoreAdapter> Adapter;
    if (auto Err = HR::toError(
            AdapterList->GetAdapter(AdapterIndex, Adapter.GetAddressOf()),
            "Failed to acquire adapter")) {
      return Err;
    }
    auto ExDevice = DXDevice::create(Adapter, Config);
    if (!ExDevice)
      return ExDevice.takeError();
    Devices.push_back(std::move(*ExDevice));
  }
  return llvm::Error::success();
}
