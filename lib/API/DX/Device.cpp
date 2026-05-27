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
#include "API/Encoder.h"
#include "API/FormatConversion.h"
#include "DXFeatures.h"
#include "Support/Pipeline.h"
#include "Support/WinError.h"

#include "DXResources.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Object/DXContainer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Signals.h"

#include "../Util.h"

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>

using namespace offloadtest;
using Microsoft::WRL::ComPtr;

using ID3D12DeviceX = ID3D12Device2;
using ID3D12GraphicsCommandListX = ID3D12GraphicsCommandList6;

template <> char CapabilityValueEnum<directx::ShaderModel>::ID = 0;
template <> char CapabilityValueEnum<directx::RootSignature>::ID = 0;
template <> char CapabilityValueEnum<directx::MeshShaderTier>::ID = 0;

static std::mutex SignalHandlerMutex;
static llvm::SmallVector<ID3D12DeviceX *> SignalHandlerDevices;

static void dumpD3DInfoQueues(void *) {
  const std::lock_guard<std::mutex> Lock(SignalHandlerMutex);
  for (ID3D12DeviceX *Device : SignalHandlerDevices) {
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

// D3D12 requires the RowPitch in a placed subresource footprint (used for
// texture <-> buffer copies via CopyTextureRegion) to be a multiple of
// D3D12_TEXTURE_DATA_PITCH_ALIGNMENT (256 bytes). For textures whose natural
// row size (Width * elementSize) is already a multiple of 256, this is a
// no-op; for smaller rows it pads up.
static uint32_t getAlignedTexturePitch(uint32_t Width, uint32_t ElementSize) {
  return llvm::alignTo(Width * ElementSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE
getDXPrimitiveTopologyType(PrimitiveTopology Topology) {
  switch (Topology) {
  case PrimitiveTopology::TriangleList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  case PrimitiveTopology::PointList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  }
  llvm_unreachable("All PrimitiveTopology cases handled");
}

static D3D_PRIMITIVE_TOPOLOGY
getDXPrimitiveTopology(PrimitiveTopology Topology) {
  switch (Topology) {
  case PrimitiveTopology::TriangleList:
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  case PrimitiveTopology::PointList:
    return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  }
  llvm_unreachable("All PrimitiveTopology cases handled");
}

static uint64_t getAlignedTextureBufferSize(const CPUBuffer &B) {
  const uint64_t AlignedPitch =
      getAlignedTexturePitch(B.OutputProps.Width, B.getElementSize());
  const uint64_t LastRowSize =
      uint64_t(B.OutputProps.Width) * B.getElementSize();
  return uint64_t(B.OutputProps.Height - 1) * AlignedPitch + LastRowSize;
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

static D3D12_TEXTURE_ADDRESS_MODE getDXAddressMode(AddressMode Mode) {
  switch (Mode) {
  case AddressMode::Clamp:
    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  case AddressMode::Repeat:
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  case AddressMode::Mirror:
    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
  case AddressMode::Border:
    return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  case AddressMode::MirrorOnce:
    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
  }
  llvm_unreachable("All AddressMode cases handled");
}

static D3D12_COMPARISON_FUNC getDXComparisonFunc(CompareFunction Func) {
  switch (Func) {
  case CompareFunction::Never:
    return D3D12_COMPARISON_FUNC_NEVER;
  case CompareFunction::Less:
    return D3D12_COMPARISON_FUNC_LESS;
  case CompareFunction::Equal:
    return D3D12_COMPARISON_FUNC_EQUAL;
  case CompareFunction::LessEqual:
    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
  case CompareFunction::Greater:
    return D3D12_COMPARISON_FUNC_GREATER;
  case CompareFunction::NotEqual:
    return D3D12_COMPARISON_FUNC_NOT_EQUAL;
  case CompareFunction::GreaterEqual:
    return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
  case CompareFunction::Always:
    return D3D12_COMPARISON_FUNC_ALWAYS;
  }
  llvm_unreachable("All CompareFunction cases handled");
}

// Compose a D3D12_FILTER from the sampler kind and the requested min/mag
// filter modes. Mip filtering is hardcoded to Nearest (matches VK backend) and
// anisotropic filtering is not currently exposed in the YAML schema.
static D3D12_FILTER getDXFilter(SamplerKind Kind, FilterMode MinFilter,
                                FilterMode MagFilter) {
  const bool IsComparison = (Kind == SamplerKind::SamplerComparison);
  const bool MinLinear = (MinFilter == FilterMode::Linear);
  const bool MagLinear = (MagFilter == FilterMode::Linear);
  D3D12_FILTER F;
  if (!MinLinear && !MagLinear)
    F = D3D12_FILTER_MIN_MAG_MIP_POINT;
  else if (!MinLinear && MagLinear)
    F = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
  else if (MinLinear && !MagLinear)
    F = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
  else
    F = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
  if (IsComparison) {
    // Set the comparison reduction bit (high bit of the D3D12_FILTER encoding).
    F = static_cast<D3D12_FILTER>(static_cast<unsigned>(F) |
                                  D3D12_FILTER_REDUCTION_TYPE_COMPARISON
                                      << D3D12_FILTER_REDUCTION_TYPE_SHIFT);
  }
  return F;
}

static D3D12_SAMPLER_DESC getDXSamplerDesc(const Sampler &S) {
  D3D12_SAMPLER_DESC Desc = {};
  Desc.Filter = getDXFilter(S.Kind, S.MinFilter, S.MagFilter);
  Desc.AddressU = getDXAddressMode(S.Address);
  Desc.AddressV = getDXAddressMode(S.Address);
  Desc.AddressW = getDXAddressMode(S.Address);
  Desc.MipLODBias = S.MipLODBias;
  Desc.MaxAnisotropy = 1;
  Desc.ComparisonFunc = (S.Kind == SamplerKind::SamplerComparison)
                            ? getDXComparisonFunc(S.ComparisonOp)
                            : D3D12_COMPARISON_FUNC_NEVER;
  // Transparent black to match VK backend.
  Desc.BorderColor[0] = 0.0f;
  Desc.BorderColor[1] = 0.0f;
  Desc.BorderColor[2] = 0.0f;
  Desc.BorderColor[3] = 0.0f;
  Desc.MinLOD = S.MinLOD;
  Desc.MaxLOD = S.MaxLOD;
  return Desc;
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
        R.IsReserved && (getDescriptorKind(R.Kind) == DescriptorKind::SRV ||
                         getDescriptorKind(R.Kind) == DescriptorKind::UAV)
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

  size_t getSizeInBytes() const override { return SizeInBytes; }

  llvm::Expected<void *> map() override {
    if (Desc.Location == MemoryLocation::GpuOnly)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Cannot map a GpuOnly buffer.");
    void *Ptr = nullptr;
    if (auto Err =
            HR::toError(Buffer->Map(0, nullptr, &Ptr), "Failed to map buffer."))
      return std::move(Err);
    return Ptr;
  }

  void unmap() override { Buffer->Unmap(0, nullptr); }

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

  const TextureCreateDesc &getDesc() const override { return Desc; }

  static bool classof(const offloadtest::Texture *T) {
    return T->getAPI() == GPUAPI::DirectX;
  }
};

class DXPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  ComPtr<ID3D12RootSignature> RootSig;
  ComPtr<ID3D12PipelineState> PSO;
  // Only set for graphics pipelines.
  std::optional<D3D_PRIMITIVE_TOPOLOGY> Topology;

  DXPipelineState(llvm::StringRef Name, ComPtr<ID3D12RootSignature> RootSig,
                  ComPtr<ID3D12PipelineState> PSO,
                  std::optional<D3D_PRIMITIVE_TOPOLOGY> Topology)
      : offloadtest::PipelineState(GPUAPI::DirectX), Name(Name),
        RootSig(RootSig), PSO(PSO), Topology(Topology) {}

  static bool classof(const offloadtest::PipelineState *B) {
    return B->getAPI() == GPUAPI::DirectX;
  }
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

  static llvm::Expected<std::unique_ptr<DXFence>> create(ID3D12DeviceX *Device,
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
  createGraphicsQueue(ComPtr<ID3D12DeviceX> Device) {
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
  ComPtr<ID3D12GraphicsCommandListX> CmdList;
  /// Whether a UAV barrier is pending from a prior compute command.
  bool PendingUAVBarrier = false;

  static llvm::Expected<std::unique_ptr<DXCommandBuffer>>
  create(ComPtr<ID3D12DeviceX> Device) {
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

  void addPendingUAVBarrier() { PendingUAVBarrier = true; }

  void flushBarrier() {
    if (!PendingUAVBarrier)
      return;
    const D3D12_RESOURCE_BARRIER Barrier =
        CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    CmdList->ResourceBarrier(1, &Barrier);
    PendingUAVBarrier = false;
  }

  llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
  createComputeEncoder() override;

  llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
  createRenderEncoder(const offloadtest::RenderPassBeginDesc &Desc) override;

private:
  DXCommandBuffer() : CommandBuffer(GPUAPI::DirectX) {}
};

struct DescriptorAllocator {
  ComPtr<ID3D12DescriptorHeap> Heap;
  std::atomic<uint32_t> NextIndex{0};
  uint32_t DescIncSize;
  uint32_t Capacity;

  static llvm::Expected<DescriptorAllocator>
  create(ID3D12DeviceX *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type,
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

class DXComputeEncoder : public offloadtest::ComputeEncoder {
  DXCommandBuffer &CB;

  void addUAVBarrier() {
    CB.addPendingUAVBarrier();
    CB.flushBarrier();
  }

public:
  DXComputeEncoder(DXCommandBuffer &CB)
      : ComputeEncoder(GPUAPI::DirectX), CB(CB) {}

  ~DXComputeEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::DirectX;
  }

  // D3D12 debug labels require WinPixEventRuntime for the proper event
  // encoding.  Without it, BeginEvent/EndEvent/SetMarker with metadata type 0
  // crash the D3D12 debug layer, so leave these as no-ops for now.
  void pushDebugGroup(llvm::StringRef Label) override {}
  void popDebugGroup() override {}
  void insertDebugSignpost(llvm::StringRef Label) override {}

  llvm::Error dispatch(uint32_t GroupCountX, uint32_t GroupCountY,
                       uint32_t GroupCountZ) override {
    addUAVBarrier();
    insertDebugSignpost(llvm::formatv("Dispatch [{0},{1},{2}]", GroupCountX,
                                      GroupCountY, GroupCountZ)
                            .str());
    CB.CmdList->Dispatch(GroupCountX, GroupCountY, GroupCountZ);
    return llvm::Error::success();
  }

  llvm::Error copyBufferToBuffer(offloadtest::Buffer &Src, size_t SrcOffset,
                                 offloadtest::Buffer &Dst, size_t DstOffset,
                                 size_t Size) override {
    auto &DXSrc = static_cast<DXBuffer &>(Src);
    auto &DXDst = static_cast<DXBuffer &>(Dst);
    addUAVBarrier();
    insertDebugSignpost(llvm::formatv("CopyBuffer {0}B", Size).str());
    CB.CmdList->CopyBufferRegion(DXDst.Buffer.Get(), DstOffset,
                                 DXSrc.Buffer.Get(), SrcOffset, Size);
    return llvm::Error::success();
  }

  void endEncodingImpl() override { popDebugGroup(); }
};

llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
DXCommandBuffer::createComputeEncoder() {
  auto Enc = std::make_unique<DXComputeEncoder>(*this);
  Enc->pushDebugGroup("ComputeEncoder");
  return Enc;
}

class DXRenderPass final : public offloadtest::RenderPass {
public:
  offloadtest::RenderPassDesc Desc;

  explicit DXRenderPass(offloadtest::RenderPassDesc Desc)
      : RenderPass(GPUAPI::DirectX), Desc(std::move(Desc)) {}

  static bool classof(const offloadtest::RenderPass *RP) {
    return RP->getAPI() == GPUAPI::DirectX;
  }
};

class DXRenderEncoder : public offloadtest::RenderEncoder {
  DXCommandBuffer &CB;

  // Encoder contract: viewport and scissor must both be set before draw().
  bool ViewportSet = false;
  bool ScissorSet = false;

  llvm::Error bindCommonDrawState(const offloadtest::PipelineState &PSO) {
    if (!ViewportSet)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Viewport must be set before drawing.");
    if (!ScissorSet)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Scissor must be set before drawing.");

    const auto &DXPSO = llvm::cast<DXPipelineState>(PSO);
    CB.CmdList->SetGraphicsRootSignature(DXPSO.RootSig.Get());
    CB.CmdList->SetPipelineState(DXPSO.PSO.Get());
    // Mesh-shader pipelines bypass the input assembler and carry no IA
    // topology; only bind one when the pipeline actually has one.
    if (DXPSO.Topology)
      CB.CmdList->IASetPrimitiveTopology(*DXPSO.Topology);
    return llvm::Error::success();
  }

public:
  DXRenderEncoder(DXCommandBuffer &CB)
      : RenderEncoder(GPUAPI::DirectX), CB(CB) {}
  DXRenderEncoder(const DXRenderEncoder &CB) = delete;
  DXRenderEncoder(DXRenderEncoder &&CB) = delete;
  DXRenderEncoder &operator=(DXRenderEncoder &CB) = delete;
  DXRenderEncoder &operator=(const DXRenderEncoder &&CB) = delete;

  ~DXRenderEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::DirectX;
  }

  // See DXComputeEncoder for why these are no-ops.
  void pushDebugGroup(llvm::StringRef Label) override {}
  void popDebugGroup() override {}
  void insertDebugSignpost(llvm::StringRef Label) override {}

  void setViewport(const offloadtest::Viewport &VP) override {
    D3D12_VIEWPORT DXVP = {};
    DXVP.TopLeftX = VP.X;
    DXVP.TopLeftY = VP.Y;
    DXVP.Width = VP.Width;
    DXVP.Height = VP.Height;
    DXVP.MinDepth = VP.MinDepth;
    DXVP.MaxDepth = VP.MaxDepth;
    CB.CmdList->RSSetViewports(1, &DXVP);
    ViewportSet = true;
  }

  void setScissor(const offloadtest::ScissorRect &Rect) override {
    const D3D12_RECT DXRect = {Rect.X, Rect.Y,
                               static_cast<LONG>(Rect.X + Rect.Width),
                               static_cast<LONG>(Rect.Y + Rect.Height)};
    CB.CmdList->RSSetScissorRects(1, &DXRect);
    ScissorSet = true;
  }

  void setVertexBuffer(uint32_t Slot, offloadtest::Buffer *VB, size_t Offset,
                       uint32_t Stride) override {
    assert(Slot < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT &&
           "Vertex buffer slot exceeds D3D12 IA input resource slot count");
    assert(Slot == 0 && "Pipeline input layout only describes slot 0");
    if (VB) {
      auto &DXVB = llvm::cast<DXBuffer>(*VB);
      D3D12_VERTEX_BUFFER_VIEW VBView = {};
      VBView.BufferLocation = DXVB.Buffer->GetGPUVirtualAddress() + Offset;
      VBView.SizeInBytes = static_cast<UINT>(DXVB.getSizeInBytes() - Offset);
      VBView.StrideInBytes = Stride;
      CB.CmdList->IASetVertexBuffers(Slot, 1, &VBView);
    } else {
      CB.CmdList->IASetVertexBuffers(Slot, 1, nullptr);
    }
  }

  llvm::Error drawInstanced(const offloadtest::PipelineState &PSO,
                            uint32_t VertexCount, uint32_t InstanceCount,
                            uint32_t FirstVertex,
                            uint32_t FirstInstance) override {
    if (auto Err = bindCommonDrawState(PSO))
      return Err;
    CB.CmdList->DrawInstanced(VertexCount, InstanceCount, FirstVertex,
                              FirstInstance);
    return llvm::Error::success();
  }

  llvm::Error dispatchMesh(const offloadtest::PipelineState &PSO,
                           uint32_t GroupCountX, uint32_t GroupCountY,
                           uint32_t GroupCountZ) override {
    if (auto Err = bindCommonDrawState(PSO))
      return Err;
    CB.CmdList->DispatchMesh(GroupCountX, GroupCountY, GroupCountZ);
    return llvm::Error::success();
  }

  void endEncodingImpl() override { popDebugGroup(); }
};

llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
DXCommandBuffer::createRenderEncoder(
    const offloadtest::RenderPassBeginDesc &Desc) {
  // The pass carries format / load / store policy; the begin desc supplies
  // the actual textures. Walk both in lockstep.
  if (!Desc.Pass)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc is missing its RenderPass.");
  auto &DXPass = llvm::cast<DXRenderPass>(*Desc.Pass);
  const offloadtest::RenderPassDesc &PassDesc = DXPass.Desc;

  if (Desc.ColorAttachments.size() != PassDesc.ColorAttachments.size())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc color attachment count does not match its "
        "RenderPass.");
  if (PassDesc.DepthStencil.has_value() != (Desc.DepthStencil != nullptr))
    return llvm::createStringError(std::errc::invalid_argument,
                                   "RenderPassBeginDesc depth-stencil "
                                   "presence does not match its RenderPass.");

  if (auto Err = findAndValidateRenderPassTextureSize(Desc, nullptr, nullptr))
    return Err;

  // Validate attachments and gather the RTV / DSV CPU handles. RT and DSV
  // descriptors are owned by the textures themselves; this just collects
  // them for OMSetRenderTargets.
  llvm::SmallVector<DXTexture *, 8> RTTextures;
  llvm::SmallVector<D3D12_CPU_DESCRIPTOR_HANDLE, 8> RTVHandles;
  RTTextures.reserve(Desc.ColorAttachments.size());
  RTVHandles.reserve(Desc.ColorAttachments.size());
  for (offloadtest::Texture *Tex : Desc.ColorAttachments) {
    if (!Tex)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RenderPassBeginDesc has a null color attachment texture.");
    auto &DXTex = llvm::cast<DXTexture>(*Tex);
    if (DXTex.RTVHandle.ptr == 0)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Color attachment texture was not created with RenderTarget usage.");
    RTTextures.push_back(&DXTex);
    RTVHandles.push_back(DXTex.RTVHandle);
  }

  DXTexture *DSTexture = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
  if (Desc.DepthStencil) {
    auto &DXDS = llvm::cast<DXTexture>(*Desc.DepthStencil);
    if (DXDS.DSVHandle.ptr == 0)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Depth-stencil texture was not created with DepthStencil usage.");
    DSTexture = &DXDS;
    DSVHandle = DXDS.DSVHandle;
  }

  CmdList->OMSetRenderTargets(static_cast<UINT>(RTVHandles.size()),
                              RTVHandles.data(),
                              /*RTsSingleHandleToDescriptorRange=*/false,
                              Desc.DepthStencil ? &DSVHandle : nullptr);

  for (size_t I = 0; I < PassDesc.ColorAttachments.size(); ++I) {
    if (PassDesc.ColorAttachments[I].Load != offloadtest::LoadAction::Clear)
      continue;
    if (!RTTextures[I]->Desc.OptimizedClearValue)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "LoadAction::Clear requires the render target to have been "
          "created with an OptimizedClearValue.");
    const auto *CV =
        std::get_if<ClearColor>(&*RTTextures[I]->Desc.OptimizedClearValue);
    assert(CV && "RenderTarget OptimizedClearValue must be a ClearColor");
    const float ClearArr[4] = {CV->R, CV->G, CV->B, CV->A};
    CmdList->ClearRenderTargetView(RTVHandles[I], ClearArr, 0, nullptr);
  }
  if (PassDesc.DepthStencil) {
    D3D12_CLEAR_FLAGS Flags = static_cast<D3D12_CLEAR_FLAGS>(0);
    if (PassDesc.DepthStencil->DepthLoad == offloadtest::LoadAction::Clear)
      Flags |= D3D12_CLEAR_FLAG_DEPTH;
    if (PassDesc.DepthStencil->StencilLoad == offloadtest::LoadAction::Clear)
      Flags |= D3D12_CLEAR_FLAG_STENCIL;
    if (Flags != 0) {
      if (!DSTexture->Desc.OptimizedClearValue)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "LoadAction::Clear requires the depth-stencil texture to have "
            "been created with an OptimizedClearValue.");
      const auto *CV =
          std::get_if<ClearDepthStencil>(&*DSTexture->Desc.OptimizedClearValue);
      assert(CV &&
             "DepthStencil OptimizedClearValue must be a ClearDepthStencil");
      CmdList->ClearDepthStencilView(DSVHandle, Flags, CV->Depth, CV->Stencil,
                                     0, nullptr);
    }
  }

  auto Enc = std::make_unique<DXRenderEncoder>(*this);
  Enc->pushDebugGroup("RenderEncoder");
  return Enc;
}

class DXDevice : public offloadtest::Device {
private:
  ComPtr<IDXCoreAdapter> Adapter;
  ComPtr<ID3D12DeviceX> Device;
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
    ComPtr<ID3D12DescriptorHeap> SamplerHeap;
    std::unique_ptr<DXCommandBuffer> CB;
    std::unique_ptr<PipelineState> Pipeline;

    // Resources for graphics pipelines.
    std::unique_ptr<offloadtest::RenderPass> RenderPass;
    std::unique_ptr<offloadtest::Texture> RenderTarget;
    std::unique_ptr<offloadtest::Buffer> RTReadback;
    std::unique_ptr<offloadtest::Texture> DepthStencil;
    std::unique_ptr<offloadtest::Buffer> VB;

    llvm::SmallVector<DescriptorTable> DescTables;
    llvm::SmallVector<ResourcePair> RootResources;
  };

public:
  DXDevice(ComPtr<IDXCoreAdapter> A, ComPtr<ID3D12DeviceX> D, DXQueue Q,
           DescriptorAllocator RTVAllocator, DescriptorAllocator DSVAllocator,
           std::string Desc, std::string DriverVer)
      : Adapter(A), Device(D), GraphicsQueue(std::move(Q)),
        RTVAllocator(std::move(RTVAllocator)),
        DSVAllocator(std::move(DSVAllocator)) {
    Description = std::move(Desc);
    DriverName = "DirectX";
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
      llvm::StringRef, const BindingsDesc &BndDesc, bool IsGraphics,
      ComPtr<ID3D12RootSignature> &OutRootSignature) {
    uint32_t DescriptorCount = 0;
    for (auto &D : BndDesc.DescriptorSetDescs)
      DescriptorCount += D.ResourceBindings.size();

    std::vector<D3D12_ROOT_PARAMETER> RootParams;
    const std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]> Ranges(
        new D3D12_DESCRIPTOR_RANGE[DescriptorCount]);
    uint32_t RangeIdx = 0;
    for (const auto &Set : BndDesc.DescriptorSetDescs) {
      // D3D12 requires SAMPLER ranges in their own descriptor table because
      // samplers live in a different descriptor heap type from CBV/SRV/UAV.
      // Walk the bindings twice so the resulting Ranges array is contiguous
      // per root parameter (CBV/SRV/UAV first, then SAMPLER).
      uint32_t ResourceCount = 0;
      uint32_t SamplerCount = 0;
      uint32_t ResourceStart = RangeIdx;
      for (const auto &Binding : Set.ResourceBindings) {
        if (getDescriptorKind(Binding.Kind) == DescriptorKind::SAMPLER)
          continue;
        D3D12_DESCRIPTOR_RANGE_TYPE RangeType;
        switch (getDescriptorKind(Binding.Kind)) {
        case DescriptorKind::SRV:
          RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
          break;
        case DescriptorKind::UAV:
          RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
          break;
        case DescriptorKind::CBV:
          RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
          break;
        case DescriptorKind::SAMPLER:
          llvm_unreachable("SAMPLER handled in the sampler pass below");
        }
        Ranges.get()[RangeIdx].RangeType = RangeType;
        Ranges.get()[RangeIdx].NumDescriptors = Binding.DescriptorCount;
        Ranges.get()[RangeIdx].BaseShaderRegister = Binding.DXBinding.Register;
        Ranges.get()[RangeIdx].RegisterSpace = Binding.DXBinding.Space;
        Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart =
            ResourceCount;
        ResourceCount += Binding.DescriptorCount;
        ++RangeIdx;
      }
      if (ResourceCount > 0) {
        RootParams.push_back(D3D12_ROOT_PARAMETER{
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            {D3D12_ROOT_DESCRIPTOR_TABLE{
                RangeIdx - ResourceStart, &Ranges.get()[ResourceStart]}},
            D3D12_SHADER_VISIBILITY_ALL});
      }

      uint32_t SamplerStart = RangeIdx;
      for (const auto &Binding : Set.ResourceBindings) {
        if (getDescriptorKind(Binding.Kind) != DescriptorKind::SAMPLER)
          continue;
        Ranges.get()[RangeIdx].RangeType =
            D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        Ranges.get()[RangeIdx].NumDescriptors = Binding.DescriptorCount;
        Ranges.get()[RangeIdx].BaseShaderRegister = Binding.DXBinding.Register;
        Ranges.get()[RangeIdx].RegisterSpace = Binding.DXBinding.Space;
        Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart = SamplerCount;
        SamplerCount += Binding.DescriptorCount;
        ++RangeIdx;
      }
      if (SamplerCount > 0) {
        RootParams.push_back(D3D12_ROOT_PARAMETER{
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            {D3D12_ROOT_DESCRIPTOR_TABLE{RangeIdx - SamplerStart,
                                         &Ranges.get()[SamplerStart]}},
            D3D12_SHADER_VISIBILITY_ALL});
      }
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
  createRootSignature(llvm::StringRef Name, const BindingsDesc &BndDesc,
                      const ShaderContainer &Shader, bool IsGraphics,
                      ComPtr<ID3D12RootSignature> &OutRootSignature) {
    assert(OutRootSignature.Get() == nullptr);

    if (auto Err =
            createRootSignatureFromShader(Name, Shader, OutRootSignature))
      return Err;

    if (OutRootSignature.Get() != nullptr)
      return llvm::Error::success();

    return createRootSignatureFromBindingsDesc(Name, BndDesc, IsGraphics,
                                               OutRootSignature);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BndDesc,
                   ShaderContainer CS) override {
    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BndDesc, CS,
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

    return std::make_unique<DXPipelineState>(Name, RootSig, PSO, std::nullopt);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createTraditionalRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BndDesc,
      const TraditionalRasterPipelineCreateDesc &Desc) override {
    assert(Desc.RTFormats.size() <= 8);

    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BndDesc, Desc.VS,
                                       /*IsGraphics=*/true, RootSig))
      return Err;

    std::vector<D3D12_INPUT_ELEMENT_DESC> DXInputLayout;
    DXInputLayout.reserve(Desc.InputLayout.size());
    for (const InputLayoutDesc &Elem : Desc.InputLayout) {
      assert(!Elem.InstanceStepRate &&
             "Instance step rate is currently not supported.");

      D3D12_INPUT_ELEMENT_DESC ElementDesc = {};
      ElementDesc.SemanticName = Elem.Name.c_str();
      ElementDesc.SemanticIndex = 0;
      ElementDesc.Format = getDXGIFormat(Elem.Fmt);
      ElementDesc.InputSlot = 0;
      ElementDesc.AlignedByteOffset = Elem.OffsetInBytes;
      ElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
      DXInputLayout.push_back(ElementDesc);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC PSODesc = {};
    PSODesc.InputLayout = {DXInputLayout.data(), (UINT)DXInputLayout.size()};
    PSODesc.pRootSignature = RootSig.Get();
    PSODesc.VS = {Desc.VS.Shader->getBuffer().data(),
                  Desc.VS.Shader->getBuffer().size()};
    PSODesc.PS = {Desc.PS.Shader->getBuffer().data(),
                  Desc.PS.Shader->getBuffer().size()};
    if (PSODesc.VS.BytecodeLength == 0 || PSODesc.PS.BytecodeLength == 0)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Graphics pipeline requires both a vertex "
                                     "shader and a pixel shader.");
    if (Desc.GS)
      PSODesc.GS = {Desc.GS->Shader->getBuffer().data(),
                    Desc.GS->Shader->getBuffer().size()};

    PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    PSODesc.RasterizerState.FrontCounterClockwise = TRUE;
    PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    PSODesc.DepthStencilState.DepthEnable = true;
    PSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    PSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    PSODesc.DepthStencilState.StencilEnable = false;
    PSODesc.SampleMask = UINT_MAX;
    PSODesc.PrimitiveTopologyType = getDXPrimitiveTopologyType(Desc.Topology);
    PSODesc.NumRenderTargets = static_cast<UINT>(Desc.RTFormats.size());
    if (Desc.DSFormat)
      PSODesc.DSVFormat = getDXGIFormat(*Desc.DSFormat);
    for (size_t I = 0; I < Desc.RTFormats.size(); ++I)
      PSODesc.RTVFormats[I] = getDXGIFormat(Desc.RTFormats[I]);
    PSODesc.SampleDesc.Count = 1;

    ComPtr<ID3D12PipelineState> PSO;
    if (auto Err = HR::toError(
            Device->CreateGraphicsPipelineState(&PSODesc, IID_PPV_ARGS(&PSO)),
            "Failed to create graphics PSO."))
      return Err;

    return std::make_unique<DXPipelineState>(
        Name, RootSig, PSO, getDXPrimitiveTopology(Desc.Topology));
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineAsMsPs(llvm::StringRef Name, const BindingsDesc &BndDesc,
                       llvm::ArrayRef<Format> RTFormats,
                       std::optional<Format> DSFormat,
                       std::optional<ShaderContainer> AS, ShaderContainer MS,
                       std::optional<ShaderContainer> PS) {
    assert(RTFormats.size() <= 8);

    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BndDesc, MS,
                                       /*IsGraphics=*/true, RootSig))
      return Err;

    const D3D12_SHADER_BYTECODE MSBytecode = {MS.Shader->getBuffer().data(),
                                              MS.Shader->getBuffer().size()};
    if (MSBytecode.BytecodeLength == 0)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Mesh shader pipeline requires a mesh shader.");

    // The amplification (task) shader is optional.
    D3D12_SHADER_BYTECODE ASBytecode = {};
    if (AS) {
      assert((*AS).Shader->getBufferSize() > 0 &&
             "The passed task/amplification shader was empty.");
      ASBytecode = {(*AS).Shader->getBuffer().data(),
                    (*AS).Shader->getBuffer().size()};
    }

    // The pixel shader is optional
    D3D12_SHADER_BYTECODE PSBytecode = {};
    if (PS) {
      assert((*PS).Shader->getBufferSize() > 0 &&
             "The passed pixel shader was empty.");
      PSBytecode = {(*PS).Shader->getBuffer().data(),
                    (*PS).Shader->getBuffer().size()};
    }

    D3D12_RT_FORMAT_ARRAY RTArray = {};
    RTArray.NumRenderTargets = static_cast<UINT>(RTFormats.size());
    for (size_t I = 0; I < RTFormats.size(); ++I)
      RTArray.RTFormats[I] = getDXGIFormat(RTFormats[I]);

    CD3DX12_DEPTH_STENCIL_DESC1 DepthStencil(D3D12_DEFAULT);
    DepthStencil.DepthEnable = true;
    DepthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    DepthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    DepthStencil.StencilEnable = false;

    DXGI_SAMPLE_DESC SampleDesc = {};
    SampleDesc.Count = 1;

    CD3DX12_PIPELINE_MESH_STATE_STREAM Stream;
    Stream.pRootSignature = RootSig.Get();
    Stream.AS = ASBytecode;
    Stream.MS = MSBytecode;
    Stream.PS = PSBytecode;
    Stream.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    Stream.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    Stream.DepthStencilState = DepthStencil;
    Stream.SampleMask = UINT_MAX;
    Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    Stream.RTVFormats = RTArray;
    if (DSFormat)
      Stream.DSVFormat = getDXGIFormat(*DSFormat);
    Stream.SampleDesc = SampleDesc;

    const D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc = {sizeof(Stream),
                                                         &Stream};

    ComPtr<ID3D12PipelineState> PSO;
    if (auto Err = HR::toError(
            Device->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&PSO)),
            "Failed to create mesh shader PSO."))
      return Err;

    return std::make_unique<DXPipelineState>(Name, RootSig, PSO, std::nullopt);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override {
    return DXFence::create(Device.Get(), Name);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, const BufferCreateDesc &Desc,
               size_t SizeInBytes) override {
    const D3D12_HEAP_TYPE HeapType = getDXHeapType(Desc.Location);

    const D3D12_RESOURCE_FLAGS Flags =
        HeapType == D3D12_HEAP_TYPE_DEFAULT
            ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
            : D3D12_RESOURCE_FLAG_NONE;

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
  createTexture(std::string Name, const TextureCreateDesc &Desc) override {
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
    ComPtr<ID3D12DeviceX> Device;
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

    std::string DriverVer;
    if (Adapter->IsPropertySupported(DXCoreAdapterProperty::DriverVersion)) {
      uint64_t Packed = 0;
      if (SUCCEEDED(Adapter->GetProperty(DXCoreAdapterProperty::DriverVersion,
                                         sizeof(Packed), &Packed))) {
        const uint16_t Major = static_cast<uint16_t>((Packed >> 48) & 0xFFFF);
        const uint16_t Minor = static_cast<uint16_t>((Packed >> 32) & 0xFFFF);
        const uint16_t Build = static_cast<uint16_t>((Packed >> 16) & 0xFFFF);
        const uint16_t Revision = static_cast<uint16_t>(Packed & 0xFFFF);
        DriverVer = (llvm::Twine(Major) + "." + llvm::Twine(Minor) + "." +
                     llvm::Twine(Build) + "." + llvm::Twine(Revision))
                        .str();
      }
    }

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
        std::string(DescVec.data()), std::move(DriverVer));
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

  static llvm::Error configureInfoQueue(ID3D12DeviceX *Device) {
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
    // Count CBV/SRV/UAV bindings separately; samplers live in their own heap.
    uint32_t ResourceCount = 0;
    uint32_t SamplerCount = 0;
    for (const auto &Set : P.Sets)
      for (const auto &R : Set.Resources) {
        if (getDescriptorKind(R.Kind) == DescriptorKind::SAMPLER)
          SamplerCount += R.getArraySize();
        else
          ResourceCount += R.getArraySize();
      }

    if (ResourceCount > 0) {
      const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, ResourceCount,
          D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
      if (auto Err = HR::toError(Device->CreateDescriptorHeap(
                                     &HeapDesc, IID_PPV_ARGS(&State.DescHeap)),
                                 "Failed to create descriptor heap."))
        return Err;
    }

    if (SamplerCount > 0) {
      const D3D12_DESCRIPTOR_HEAP_DESC SamplerHeapDesc = {
          D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SamplerCount,
          D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
      if (auto Err =
              HR::toError(Device->CreateDescriptorHeap(
                              &SamplerHeapDesc, IID_PPV_ARGS(&State.SamplerHeap)),
                          "Failed to create sampler descriptor heap."))
        return Err;
    }
    return llvm::Error::success();
  }

  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override {
    return DXCommandBuffer::create(Device);
  }

  llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
  createRenderPass(const offloadtest::RenderPassDesc &Desc) override {
    return std::make_unique<DXRenderPass>(Desc);
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
      switch (getDescriptorKind(R.Kind)) {
      case DescriptorKind::SRV: {
        auto ExRes = createSRV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.push_back(std::make_pair(&R, *ExRes));
        break;
      }
      case DescriptorKind::UAV: {
        auto ExRes = createUAV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.push_back(std::make_pair(&R, *ExRes));
        break;
      }
      case DescriptorKind::CBV: {
        auto ExRes = createCBV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.push_back(std::make_pair(&R, *ExRes));
        break;
      }
      case DescriptorKind::SAMPLER:
        // Samplers don't have a backing GPU resource; the descriptor is
        // written directly into the sampler heap during binding. Push an empty
        // bundle so DescTables stays parallel with P.Sets[i].Resources.
        Resources.push_back(std::make_pair(&R, ResourceBundle{}));
        break;
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

    // Bind descriptors in descriptor tables. CBV/SRV/UAV descriptors go into
    // IS.DescHeap, samplers into IS.SamplerHeap; each heap tracks its own
    // sequential index.
    uint32_t HeapIndex = 0;
    uint32_t SamplerHeapIndex = 0;
    const uint32_t SamplerInc = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    for (auto &T : IS.DescTables) {
      for (auto &R : T.Resources) {
        switch (getDescriptorKind(R.first->Kind)) {
        case DescriptorKind::SRV:
          HeapIndex = bindSRV(*(R.first), IS, HeapIndex, R.second);
          break;
        case DescriptorKind::UAV:
          HeapIndex = bindUAV(*(R.first), IS, HeapIndex, R.second);
          break;
        case DescriptorKind::CBV:
          HeapIndex = bindCBV(*(R.first), IS, HeapIndex, R.second);
          break;
        case DescriptorKind::SAMPLER: {
          assert(IS.SamplerHeap && "Sampler heap must exist when binding a "
                                   "sampler descriptor.");
          assert(R.first->SamplerPtr && "Sampler resource is missing its "
                                        "schema-side Sampler description.");
          D3D12_CPU_DESCRIPTOR_HANDLE Handle =
              IS.SamplerHeap->GetCPUDescriptorHandleForHeapStart();
          Handle.ptr += SamplerHeapIndex * SamplerInc;
          const D3D12_SAMPLER_DESC Desc = getDXSamplerDesc(*R.first->SamplerPtr);
          Device->CreateSampler(&Desc, Handle);
          ++SamplerHeapIndex;
          break;
        }
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

    if (P.isTraditionalRaster() && P.Bindings.VertexBufferPtr) {
      const CPUBuffer *VBuffer = P.Bindings.VertexBufferPtr;

      BufferCreateDesc BufDesc = {};
      BufDesc.Location = MemoryLocation::CpuToGpu;
      BufDesc.Usage = BufferUsage::VertexBuffer;
      auto BufOrErr = createBufferWithData(*this, "VertexBuffer", BufDesc,
                                           VBuffer->Data[0].get(),
                                           VBuffer->size(), nullptr, nullptr);
      if (!BufOrErr)
        return BufOrErr.takeError();
      IS.VB = std::move(*BufOrErr);
      llvm::outs() << "Vertex buffer created.\n";
    }

    return llvm::Error::success();
  }

  void addUploadBeginBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R) {
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        R.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    IS.CB->CmdList->ResourceBarrier(1, &Barrier);
  }

  void addUploadEndBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R,
                           bool IsUAV) {
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        R.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
        IsUAV ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS
              : D3D12_RESOURCE_STATE_GENERIC_READ);
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

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    CD3DX12_GPU_DESCRIPTOR_HANDLE Handle;
    if (IS.DescHeap || IS.SamplerHeap) {
      llvm::SmallVector<ID3D12DescriptorHeap *, 2> Heaps;
      if (IS.DescHeap)
        Heaps.push_back(IS.DescHeap.Get());
      if (IS.SamplerHeap)
        Heaps.push_back(IS.SamplerHeap.Get());
      IS.CB->CmdList->SetDescriptorHeaps(static_cast<UINT>(Heaps.size()),
                                         Heaps.data());
      if (IS.DescHeap)
        Handle = IS.DescHeap->GetGPUDescriptorHandleForHeapStart();
    }
    const DXPipelineState &DXPipeline =
        llvm::cast<DXPipelineState>(*IS.Pipeline.get());
    IS.CB->CmdList->SetComputeRootSignature(DXPipeline.RootSig.Get());
    IS.CB->CmdList->SetPipelineState(DXPipeline.PSO.Get());

    const uint32_t Inc = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const uint32_t SamplerInc = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

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
        case dx::RootParamKind::DescriptorTable: {
          IS.CB->CmdList->SetComputeRootDescriptorTable(RootParamIndex++,
                                                        Handle);
          // Samplers live in a separate heap (SamplerHandle) and must not
          // count toward the CBV/SRV/UAV handle offset. Explicit RootParams
          // does not yet support binding sampler tables — see the
          // llvm_unreachable in the SAMPLER case below.
          uint32_t ResourceCount = 0;
          for (const auto &R : P.Sets[DescriptorTableIndex++].Resources)
            if (getDescriptorKind(R.Kind) != DescriptorKind::SAMPLER)
              ResourceCount += R.getArraySize();
          Handle.Offset(ResourceCount, Inc);
          break;
        }
        case dx::RootParamKind::RootDescriptor:
          assert(RootDescIt != IS.RootResources.end());
          if (RootDescIt->first->getArraySize() != 1)
            return llvm::createStringError(
                std::errc::value_too_large,
                "Root descriptor cannot refer to resource arrays.");
          switch (getDescriptorKind(RootDescIt->first->Kind)) {
          case DescriptorKind::SRV:
            IS.CB->CmdList->SetComputeRootShaderResourceView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case DescriptorKind::UAV:
            IS.CB->CmdList->SetComputeRootUnorderedAccessView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case DescriptorKind::CBV:
            IS.CB->CmdList->SetComputeRootConstantBufferView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case DescriptorKind::SAMPLER:
            llvm_unreachable("Not implemented yet.");
          }
          ++RootDescIt;
          break;
        }
      }
    } else {
      // If no explicit root parameters are provided, fall back to the
      // descriptor set layout. Each set with mixed CBV/SRV/UAV and SAMPLER
      // resources contributes two root parameters: one for the resource table,
      // then one for the sampler table (matching the order produced by
      // createRootSignatureFromBindingsDesc).
      CD3DX12_GPU_DESCRIPTOR_HANDLE SamplerHandle;
      if (IS.SamplerHeap)
        SamplerHandle = IS.SamplerHeap->GetGPUDescriptorHandleForHeapStart();
      uint32_t RootParamIndex = 0u;
      for (uint32_t Idx = 0u; Idx < P.Sets.size(); ++Idx) {
        uint32_t ResourceCount = 0;
        uint32_t SamplerCount = 0;
        for (const auto &R : P.Sets[Idx].Resources) {
          if (getDescriptorKind(R.Kind) == DescriptorKind::SAMPLER)
            SamplerCount += R.getArraySize();
          else
            ResourceCount += R.getArraySize();
        }
        if (ResourceCount > 0) {
          IS.CB->CmdList->SetComputeRootDescriptorTable(RootParamIndex++,
                                                        Handle);
          Handle.Offset(ResourceCount, Inc);
        }
        if (SamplerCount > 0) {
          IS.CB->CmdList->SetComputeRootDescriptorTable(RootParamIndex++,
                                                        SamplerHandle);
          SamplerHandle.Offset(SamplerCount, SamplerInc);
        }
      }
    }

    {
      auto EncoderOrErr = IS.CB->createComputeEncoder();
      if (!EncoderOrErr)
        return EncoderOrErr.takeError();
      auto &Encoder = *EncoderOrErr.get();
      if (auto Err =
              Encoder.dispatch(P.DispatchParameters.DispatchGroupCount[0],
                               P.DispatchParameters.DispatchGroupCount[1],
                               P.DispatchParameters.DispatchGroupCount[2]))
        return Err;
      Encoder.endEncoding();
    }

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

    void *Mapped = nullptr;
    auto &Readback = llvm::cast<DXBuffer>(*IS.RTReadback);
    if (auto Err = HR::toError(Readback.Buffer->Map(0, nullptr, &Mapped),
                               "Failed to map render target readback"))
      return Err;

    // Query the copy footprint to get the actual padded row pitch used by
    // the copy operation (D3D12 requires 256-byte aligned rows).
    auto &RT = llvm::cast<DXTexture>(*IS.RenderTarget);
    const D3D12_RESOURCE_DESC RTDesc = RT.Resource->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Placed = {};
    uint32_t NumRows = 0;
    uint64_t RowSizeInBytes = 0;
    uint64_t TotalBytes = 0;
    Device->GetCopyableFootprints(&RTDesc, 0u, 1u, 0u, &Placed, &NumRows,
                                  &RowSizeInBytes, &TotalBytes);

    P.Bindings.RTargetBufferPtr->copyFromTexture(Mapped,
                                                 Placed.Footprint.RowPitch);
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

    IS.RenderTarget = std::move(*TexOrErr);

    // Create readback buffer sized for the pixel data with row pitch padded
    // up to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, which is what D3D12 requires
    // for the placed footprint used by CopyTextureRegion. The compaction
    // back to a tight layout happens in readBack() via GetCopyableFootprints.
    BufferCreateDesc BufDesc = {};
    BufDesc.Location = MemoryLocation::GpuToCpu;
    BufDesc.Usage = BufferUsage::Storage;
    auto BufOrErr = createBuffer("RTReadback", BufDesc,
                                 getAlignedTextureBufferSize(OutBuf));
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
    IS.DepthStencil = std::move(*TexOrErr);
    return llvm::Error::success();
  }

  llvm::Error createGraphicsCommands(Pipeline &P, InvocationState &IS) {
    auto &RT = llvm::cast<DXTexture>(*IS.RenderTarget);
    auto &DS = llvm::cast<DXTexture>(*IS.DepthStencil);
    auto &RTReadback = llvm::cast<DXBuffer>(*IS.RTReadback);

    const DXPipelineState &DXPipeline =
        llvm::cast<DXPipelineState>(*IS.Pipeline.get());
    IS.CB->CmdList->SetGraphicsRootSignature(DXPipeline.RootSig.Get());
    if (IS.DescHeap || IS.SamplerHeap) {
      llvm::SmallVector<ID3D12DescriptorHeap *, 2> Heaps;
      if (IS.DescHeap)
        Heaps.push_back(IS.DescHeap.Get());
      if (IS.SamplerHeap)
        Heaps.push_back(IS.SamplerHeap.Get());
      IS.CB->CmdList->SetDescriptorHeaps(static_cast<UINT>(Heaps.size()),
                                         Heaps.data());

      const uint32_t Inc = Device->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
      const uint32_t SamplerInc = Device->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
      CD3DX12_GPU_DESCRIPTOR_HANDLE ResHandle;
      if (IS.DescHeap)
        ResHandle = IS.DescHeap->GetGPUDescriptorHandleForHeapStart();
      CD3DX12_GPU_DESCRIPTOR_HANDLE SamplerHandle;
      if (IS.SamplerHeap)
        SamplerHandle = IS.SamplerHeap->GetGPUDescriptorHandleForHeapStart();
      uint32_t RootParamIndex = 0u;
      for (uint32_t Idx = 0u; Idx < P.Sets.size(); ++Idx) {
        uint32_t ResourceCount = 0;
        uint32_t SamplerCount = 0;
        for (const auto &R : P.Sets[Idx].Resources) {
          if (getDescriptorKind(R.Kind) == DescriptorKind::SAMPLER)
            SamplerCount += R.getArraySize();
          else
            ResourceCount += R.getArraySize();
        }
        if (ResourceCount > 0) {
          IS.CB->CmdList->SetGraphicsRootDescriptorTable(RootParamIndex++,
                                                         ResHandle);
          ResHandle.Offset(ResourceCount, Inc);
        }
        if (SamplerCount > 0) {
          IS.CB->CmdList->SetGraphicsRootDescriptorTable(RootParamIndex++,
                                                         SamplerHandle);
          SamplerHandle.Offset(SamplerCount, SamplerInc);
        }
      }
    }

    RenderPassBeginDesc BeginDesc = {};
    BeginDesc.Pass = IS.RenderPass.get();
    BeginDesc.ColorAttachments.push_back(&RT);
    BeginDesc.DepthStencil = &DS;

    auto EncOrErr = IS.CB->createRenderEncoder(BeginDesc);
    if (!EncOrErr)
      return EncOrErr.takeError();
    auto &Encoder = *EncOrErr.get();

    Viewport VP;
    VP.Width =
        static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Width);
    VP.Height =
        static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Height);
    Encoder.setViewport(VP);

    ScissorRect Scissor;
    Scissor.Width = static_cast<uint32_t>(VP.Width);
    Scissor.Height = static_cast<uint32_t>(VP.Height);
    Encoder.setScissor(Scissor);

    if (P.isTraditionalRaster()) {
      if (IS.VB)
        Encoder.setVertexBuffer(0, IS.VB.get(), 0,
                                P.Bindings.getVertexStride());

      if (auto Err =
              Encoder.drawInstanced(*IS.Pipeline.get(), P.getVertexCount(),
                                    /*InstanceCount=*/1))
        return Err;
    } else {
      if (auto Err = Encoder.dispatchMesh(
              *IS.Pipeline.get(), P.DispatchParameters.DispatchGroupCount[0],
              P.DispatchParameters.DispatchGroupCount[1],
              P.DispatchParameters.DispatchGroupCount[2]))
        return Err;
    }

    Encoder.endEncoding();

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
            B.OutputProps.Height, 1,
            getAlignedTexturePitch(B.OutputProps.Width, B.getElementSize()))};
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

    BindingsDesc BndDesc = {};
    for (auto &S : P.Sets) {
      DescriptorSetLayoutDesc Layout;
      for (auto &R : S.Resources) {
        ResourceBindingDesc ResourceBinding = {};
        ResourceBinding.Kind = R.Kind;
        ResourceBinding.DXBinding.Register = R.DXBinding.Register;
        ResourceBinding.DXBinding.Space = R.DXBinding.Space;
        ResourceBinding.VKBinding = R.VKBinding;
        ResourceBinding.DescriptorCount = R.getArraySize();

        Layout.ResourceBindings.push_back(ResourceBinding);
      }

      BndDesc.DescriptorSetDescs.push_back(Layout);
    }

    if (P.isRaster()) {
      // Create render target and depth/stencil
      if (auto Err = createRenderTarget(P, State))
        return Err;
      llvm::outs() << "Render target created.\n";
      // TODO: Always created for graphics pipelines. Consider making this
      // conditional on the pipeline definition.
      if (auto Err = createDepthStencil(P, State))
        return Err;
      llvm::outs() << "Depth stencil created.\n";
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
          createPipelineCs("Compute Pipeline State", BndDesc, CS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Compute Pipeline created.\n";
      if (auto Err = createComputeCommands(P, State))
        return Err;
      llvm::outs() << "Compute command list created.\n";

    } else if (P.isRaster()) {

      // Begin a render pass: bind RT/DSV and clear depth-stencil. Color
      // load action is Load — the existing inline code didn't clear color.
      ColorAttachmentFormatDesc ColorAttachment = {};
      ColorAttachment.Fmt = State.RenderTarget->getDesc().Fmt;
      ColorAttachment.Load = LoadAction::Load;
      ColorAttachment.Store = StoreAction::Store;

      DepthStencilAttachmentFormatDesc DSAttachment = {};
      DSAttachment.Fmt = State.DepthStencil->getDesc().Fmt;
      DSAttachment.DepthLoad = LoadAction::Clear;
      DSAttachment.DepthStore = StoreAction::Store;
      DSAttachment.StencilLoad = LoadAction::DontCare;
      DSAttachment.StencilStore = StoreAction::DontCare;

      RenderPassDesc PassDesc;
      PassDesc.ColorAttachments.push_back(ColorAttachment);
      PassDesc.DepthStencil = DSAttachment;

      auto RenderPassOrErr = createRenderPass(PassDesc);
      if (!RenderPassOrErr)
        return RenderPassOrErr.takeError();
      State.RenderPass = std::move(*RenderPassOrErr);
      llvm::outs() << "Render pass created.\n";

      if (P.isTraditionalRaster()) {
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

        TraditionalRasterPipelineCreateDesc PipelineDesc = {};
        PipelineDesc.Topology = P.Bindings.Topology;
        PipelineDesc.DSFormat = Format::D32FloatS8Uint;
        for (auto &Shader : P.Shaders) {
          ShaderContainer SC = {};
          SC.EntryPoint = Shader.Entry;
          SC.Shader = Shader.Shader.get();
          PipelineDesc.setShader(Shader.Stage, std::move(SC));
        }

        // Create the input layout based on the vertex attributes.
        for (auto &Attr : P.Bindings.VertexAttributes) {
          auto FormatOrErr = toFormat(Attr.Format, Attr.Channels);
          if (!FormatOrErr)
            return FormatOrErr.takeError();

          InputLayoutDesc Layout = {};
          Layout.Name = Attr.Name;
          Layout.Fmt = *FormatOrErr;
          Layout.OffsetInBytes = Attr.Offset;
          PipelineDesc.InputLayout.push_back(Layout);
        }

        auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                    P.Bindings.RTargetBufferPtr->Channels);
        if (!FormatOrErr)
          return FormatOrErr.takeError();
        PipelineDesc.RTFormats.push_back(*FormatOrErr);

        auto PipelineStateOrErr = createTraditionalRasterPipeline(
            "Graphics Pipeline State", BndDesc, PipelineDesc);
        if (!PipelineStateOrErr)
          return PipelineStateOrErr.takeError();
        State.Pipeline = std::move(*PipelineStateOrErr);
        llvm::outs() << "Traditional Raster Pipeline created.\n";

      } else if (P.isMeshShaderRaster()) {

        std::optional<ShaderContainer> AS = {};
        ShaderContainer MS = {};
        std::optional<ShaderContainer> PS = {};
        for (auto &Shader : P.Shaders) {
          if (Shader.Stage == Stages::Amplification) {
            ShaderContainer Container;
            Container.EntryPoint = Shader.Entry;
            Container.Shader = Shader.Shader.get();
            AS = Container;
          } else if (Shader.Stage == Stages::Mesh) {
            MS.EntryPoint = Shader.Entry;
            MS.Shader = Shader.Shader.get();
          } else if (Shader.Stage == Stages::Pixel) {
            ShaderContainer Container;
            Container.EntryPoint = Shader.Entry;
            Container.Shader = Shader.Shader.get();
            PS = Container;
          }
        }

        auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                    P.Bindings.RTargetBufferPtr->Channels);
        if (!FormatOrErr)
          return FormatOrErr.takeError();

        llvm::SmallVector<Format> RTFormats;
        RTFormats.push_back(*FormatOrErr);

        auto PipelineStateOrErr =
            createPipelineAsMsPs("Mesh Shader Pipeline State", BndDesc,
                                 RTFormats, Format::D32FloatS8Uint, AS, MS, PS);

        if (!PipelineStateOrErr)
          return PipelineStateOrErr.takeError();
        State.Pipeline = std::move(*PipelineStateOrErr);
        llvm::outs() << "Mesh Shader Pipeline created.\n";
      }

      if (auto Err = createGraphicsCommands(P, State))
        return Err;
      llvm::outs() << "Graphics command list created complete.\n";
    } else {
      return llvm::createStringError("Pipeline was neither Compute nor Raster");
    }

    auto SubmitResult = GraphicsQueue.submit(std::move(State.CB));
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
