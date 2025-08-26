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
#include "DXFeatures.h"
#include "Support/Pipeline.h"
#include "Support/WinError.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Object/DXContainer.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Signals.h"

#include <codecvt>
#include <locale>

using namespace offloadtest;
using Microsoft::WRL::ComPtr;

template <> char CapabilityValueEnum<directx::ShaderModel>::ID = 0;
template <> char CapabilityValueEnum<directx::RootSignature>::ID = 0;

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
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return DXGI_FORMAT_UNKNOWN;
}

static DXGI_FORMAT getRawDXFormat(const Resource &R) {
  if (!R.isByteAddressBuffer())
    return DXGI_FORMAT_UNKNOWN;

  switch (R.BufferPtr->Format) {
  case DataFormat::Hex32:
  case DataFormat::UInt32:
  case DataFormat::Int32:
  case DataFormat::Float32:
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
  }
  llvm_unreachable("All cases handled");
}

enum DXResourceKind { UAV, SRV, CBV };

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
  }
  llvm_unreachable("All cases handled");
}

static D3D12_RESOURCE_DESC getResourceDescription(const Resource &R) {
  const D3D12_RESOURCE_DIMENSION Dimension = getDXDimension(R.Kind);
  const offloadtest::Buffer &B = *R.BufferPtr;
  const DXGI_FORMAT Format =
      R.isTexture() ? getDXFormat(B.Format, B.Channels) : DXGI_FORMAT_UNKNOWN;
  const uint32_t Width =
      R.isTexture() ? B.OutputProps.Width : getUAVBufferSize(R);
  const uint32_t Height = R.isTexture() ? B.OutputProps.Height : 1;
  const D3D12_TEXTURE_LAYOUT Layout = R.isTexture()
                                          ? D3D12_TEXTURE_LAYOUT_UNKNOWN
                                          : D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
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
    llvm_unreachable("Not an SRV type!");
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
    llvm_unreachable("Not a UAV type!");
  }
  return Desc;
}

namespace {

class DXDevice : public offloadtest::Device {
private:
  ComPtr<IDXCoreAdapter> Adapter;
  ComPtr<ID3D12Device> Device;
  Capabilities Caps;

  struct ResourceSet {
    ComPtr<ID3D12Resource> Upload;
    ComPtr<ID3D12Resource> Buffer;
    ComPtr<ID3D12Resource> Readback;
    ResourceSet(ComPtr<ID3D12Resource> Upload, ComPtr<ID3D12Resource> Buffer,
                ComPtr<ID3D12Resource> Readback)
        : Upload(Upload), Buffer(Buffer), Readback(Readback) {}
  };

  // ResourceBundle will contain one ResourceSet for a singular resource
  // or multiple ResourceSets for resource array.
  using ResourceBundle = llvm::SmallVector<ResourceSet>;
  using ResourcePair = std::pair<offloadtest::Resource *, ResourceBundle>;

  struct DescriptorTable {
    llvm::SmallVector<ResourcePair> Resources;
  };

  struct InvocationState {
    ComPtr<ID3D12RootSignature> RootSig;
    ComPtr<ID3D12DescriptorHeap> DescHeap;
    ComPtr<ID3D12PipelineState> PSO;
    ComPtr<ID3D12CommandQueue> Queue;
    ComPtr<ID3D12CommandAllocator> Allocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<ID3D12Fence> Fence;
#ifdef _WIN32
    HANDLE Event;
#else // WSL
    int Event;
#endif
    llvm::SmallVector<DescriptorTable> DescTables;
    llvm::SmallVector<ResourcePair> RootResources;
  };

public:
  DXDevice(ComPtr<IDXCoreAdapter> A, ComPtr<ID3D12Device> D, std::string Desc)
      : Adapter(A), Device(D) {
    Description = Desc;
  }
  DXDevice(const DXDevice &) = default;

  ~DXDevice() override = default;

  llvm::StringRef getAPIName() const override { return "DirectX"; }
  GPUAPI getAPI() const override { return GPUAPI::DirectX; }

  static llvm::Expected<DXDevice> create(ComPtr<IDXCoreAdapter> Adapter,
                                         const DeviceConfig &Config) {
    ComPtr<ID3D12Device> Device;
    if (auto Err =
            HR::toError(D3D12CreateDevice(Adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                          IID_PPV_ARGS(&Device)),
                        "Failed to create D3D device"))
      return Err;
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
    return DXDevice(Adapter, Device, std::string(DescVec.data()));
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
      std::make_pair(#Name, make_capability<bool>(#Name, Features.Name())));

#define D3D_FEATURE_UINT(Name)                                                 \
  Caps.insert(std::make_pair(                                                  \
      #Name, make_capability<uint32_t>(#Name, Features.Name())));

#define D3D_FEATURE_ENUM(NewEnum, Name)                                        \
  Caps.insert(std::make_pair(                                                  \
      #Name, make_capability<NewEnum>(                                         \
                 #Name, static_cast<NewEnum>(Features.Name()))));

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

  llvm::Error createRootSignature(Pipeline &P, InvocationState &State) {
    // Try pulling a root signature from the DXIL first

    auto ExContainer = llvm::object::DXContainer::create(
        P.Shaders[0].Shader->getMemBufferRef());
    // If this fails we really have a problem...
    if (!ExContainer)
      return ExContainer.takeError();

    bool HasRootSigPart = false;
    for (const auto &Part : *ExContainer)
      if (memcmp(Part.Part.Name, "RTS0", 4) == 0)
        HasRootSigPart = true;

    if (HasRootSigPart) {
      const llvm::StringRef Binary = P.Shaders[0].Shader->getBuffer();
      if (auto Err = HR::toError(
              Device->CreateRootSignature(0, Binary.data(), Binary.size(),
                                          IID_PPV_ARGS(&State.RootSig)),
              "Failed to create root signature."))
        return Err;
      return llvm::Error::success();
    }

    std::vector<D3D12_ROOT_PARAMETER> RootParams;
    const uint32_t DescriptorCount = P.getDescriptorCount();
    const std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]> Ranges =
        std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]>(
            new D3D12_DESCRIPTOR_RANGE[DescriptorCount]);

    uint32_t RangeIdx = 0;
    for (const auto &D : P.Sets) {
      uint32_t DescriptorIdx = 0;
      const uint32_t StartRangeIdx = RangeIdx;
      for (const auto &R : D.Resources) {
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
        }
        Ranges.get()[RangeIdx].NumDescriptors = R.BufferPtr->ArraySize;
        Ranges.get()[RangeIdx].BaseShaderRegister = R.DXBinding.Register;
        Ranges.get()[RangeIdx].RegisterSpace = R.DXBinding.Space;
        Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart =
            DescriptorIdx;
        RangeIdx++;
        DescriptorIdx += R.BufferPtr->ArraySize;
      }
      RootParams.push_back(
          D3D12_ROOT_PARAMETER{D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                               {D3D12_ROOT_DESCRIPTOR_TABLE{
                                   static_cast<uint32_t>(D.Resources.size()),
                                   &Ranges.get()[StartRangeIdx]}},
                               D3D12_SHADER_VISIBILITY_ALL});
    }

    CD3DX12_ROOT_SIGNATURE_DESC Desc;
    Desc.Init(static_cast<uint32_t>(RootParams.size()), RootParams.data(), 0,
              nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

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
                                        IID_PPV_ARGS(&State.RootSig)),
            "Failed to create root signature."))
      return Err;

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

  llvm::Error createPSO(llvm::StringRef DXIL, InvocationState &State) {
    const D3D12_COMPUTE_PIPELINE_STATE_DESC Desc = {
        State.RootSig.Get(),
        {DXIL.data(), DXIL.size()},
        0,
        {
            nullptr,
            0,
        },
        D3D12_PIPELINE_STATE_FLAG_NONE};
    if (auto Err = HR::toError(
            Device->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&State.PSO)),
            "Failed to create PSO."))
      return Err;
    return llvm::Error::success();
  }

  llvm::Error createCommandStructures(InvocationState &IS) {
    const D3D12_COMMAND_QUEUE_DESC Desc = {D3D12_COMMAND_LIST_TYPE_DIRECT, 0,
                                           D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    if (auto Err = HR::toError(
            Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(&IS.Queue)),
            "Failed to create command queue."))
      return Err;
    if (auto Err = HR::toError(
            Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           IID_PPV_ARGS(&IS.Allocator)),
            "Failed to create command allocator."))
      return Err;
    if (auto Err = HR::toError(
            Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IS.Allocator.Get(), nullptr,
                                      IID_PPV_ARGS(&IS.CmdList)),
            "Failed to create command list."))
      return Err;
    return llvm::Error::success();
  }

  void addResourceUploadCommands(Resource &R, InvocationState &IS,
                                 ComPtr<ID3D12Resource> Destination,
                                 ComPtr<ID3D12Resource> Source) {
    addUploadBeginBarrier(IS, Destination);
    if (R.isTexture()) {
      const offloadtest::Buffer &B = *R.BufferPtr;
      const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
          0, CD3DX12_SUBRESOURCE_FOOTPRINT(
                 getDXFormat(B.Format, B.Channels), B.OutputProps.Width,
                 B.OutputProps.Height, 1,
                 B.OutputProps.Width * B.getElementSize())};
      const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(Destination.Get(), 0);
      const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(Source.Get(), Footprint);

      IS.CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);
    } else
      IS.CmdList->CopyBufferRegion(Destination.Get(), 0, Source.Get(), 0,
                                   R.size());
    addUploadEndBarrier(IS, Destination, R.isReadWrite());
  }

  llvm::Expected<ResourceBundle> createSRV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;

    const D3D12_HEAP_PROPERTIES HeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_RESOURCE_DESC ResDesc = getResourceDescription(R);
    const D3D12_HEAP_PROPERTIES UploadHeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    const D3D12_RESOURCE_DESC UploadResDesc =
        CD3DX12_RESOURCE_DESC::Buffer(R.size());

    uint32_t RegOffset = 0;
    for (const auto &ResData : R.BufferPtr->Data) {
      llvm::outs() << "Creating SRV: { Size = " << R.size() << ", Register = t"
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

      // Initialize the SRV data
      void *ResDataPtr = nullptr;
      if (auto Err = HR::toError(UploadBuffer->Map(0, nullptr, &ResDataPtr),
                                 "Failed to acquire UAV data pointer."))
        return Err;
      memcpy(ResDataPtr, ResData.get(), R.size());
      UploadBuffer->Unmap(0, nullptr);

      addResourceUploadCommands(R, IS, Buffer, UploadBuffer);

      Bundle.emplace_back(UploadBuffer, Buffer, nullptr);
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

    const D3D12_HEAP_PROPERTIES HeapProp =
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_RESOURCE_DESC ResDesc = getResourceDescription(R);

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
                   << ", HasCounter = " << R.HasCounter << " }\n";

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

      ComPtr<ID3D12Resource> ReadBackBuffer;
      if (auto Err = HR::toError(
              Device->CreateCommittedResource(
                  &ReadBackHeapProp, D3D12_HEAP_FLAG_NONE, &ReadBackResDesc,
                  D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                  IID_PPV_ARGS(&ReadBackBuffer)),
              "Failed to create committed resource (readback buffer)."))
        return Err;

      // Initialize the UAV data
      void *ResDataPtr = nullptr;
      if (auto Err = HR::toError(UploadBuffer->Map(0, nullptr, &ResDataPtr),
                                 "Failed to acquire UAV data pointer."))
        return Err;
      memcpy(ResDataPtr, ResData.get(), R.size());
      UploadBuffer->Unmap(0, nullptr);

      addResourceUploadCommands(R, IS, Buffer, UploadBuffer);

      Bundle.emplace_back(UploadBuffer, Buffer, ReadBackBuffer);
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
      memcpy(ResDataPtr, ResData.get(), R.size());
      // Zero any remaining bytes
      if (R.size() < CBVSize) {
        void *ExtraData = static_cast<char *>(ResDataPtr) + R.size();
        memset(ExtraData, 0, CBVSize - R.size() - 1);
      }
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
        }
      }
    }

    // Setup root descriptors
    for (auto &R : P.Settings.DX.RootParams) {
      if (R.Kind != dx::RootParamKind::RootDescriptor)
        continue;
      auto &Resource = std::get<dx::RootResource>(R.Data);
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
    IS.CmdList->ResourceBarrier(1, &Barrier);
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
    IS.CmdList->ResourceBarrier(1, &Barrier);
  }

  void addReadbackBeginBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R) {
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        R.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    IS.CmdList->ResourceBarrier(1, &Barrier);
  }

  void addReadbackEndBarrier(InvocationState &IS, ComPtr<ID3D12Resource> R) {
    const D3D12_RESOURCE_BARRIER Barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        R.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    IS.CmdList->ResourceBarrier(1, &Barrier);
  }

  llvm::Error createEvent(InvocationState &IS) {
    if (auto Err = HR::toError(Device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                                   IID_PPV_ARGS(&IS.Fence)),
                               "Failed to create fence."))
      return Err;
#ifdef _WIN32
    IS.Event = CreateEventA(nullptr, false, false, nullptr);
    if (!IS.Event)
#else // WSL
    IS.Event = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (IS.Event == -1)
#endif
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create event.");
    return llvm::Error::success();
  }

  llvm::Error waitForSignal(InvocationState &IS) {
    // This is a hack but it works since this is all single threaded code.
    static uint64_t FenceCounter = 0;
    const uint64_t CurrentCounter = FenceCounter + 1;

    if (auto Err = HR::toError(IS.Queue->Signal(IS.Fence.Get(), CurrentCounter),
                               "Failed to add signal."))
      return Err;

    if (IS.Fence->GetCompletedValue() < CurrentCounter) {
#ifdef _WIN32
      HANDLE Event = IS.Event;
#else // WSL
      HANDLE Event = reinterpret_cast<HANDLE>(IS.Event);
#endif
      if (auto Err =
              HR::toError(IS.Fence->SetEventOnCompletion(CurrentCounter, Event),
                          "Failed to register end event."))
        return Err;

#ifdef _WIN32
      WaitForSingleObject(IS.Event, INFINITE);
#else // WSL
      pollfd PollEvent;
      PollEvent.fd = IS.Event;
      PollEvent.events = POLLIN;
      PollEvent.revents = 0;
      if (poll(&PollEvent, 1, -1) == -1)
        return llvm::createStringError(
            std::error_code(errno, std::system_category()), strerror(errno));
#endif
    }
    FenceCounter = CurrentCounter;
    return llvm::Error::success();
  }

  llvm::Error executeCommandList(InvocationState &IS) {
    if (auto Err =
            HR::toError(IS.CmdList->Close(), "Failed to close command list."))
      return Err;

    ID3D12CommandList *CmdLists[] = {IS.CmdList.Get()};
    IS.Queue->ExecuteCommandLists(1, CmdLists);

    return waitForSignal(IS);
  }

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    IS.CmdList->SetPipelineState(IS.PSO.Get());
    IS.CmdList->SetComputeRootSignature(IS.RootSig.Get());

    const uint32_t Inc = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_GPU_DESCRIPTOR_HANDLE Handle;

    if (IS.DescHeap) {
      ID3D12DescriptorHeap *const Heaps[] = {IS.DescHeap.Get()};
      IS.CmdList->SetDescriptorHeaps(1, Heaps);
      Handle = IS.DescHeap->GetGPUDescriptorHandleForHeapStart();
    }

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
          IS.CmdList->SetComputeRoot32BitConstants(
              RootParamIndex++, NumValues,
              Constant.BufferPtr->Data.back().get(), ConstantOffset);
          ConstantOffset += NumValues;
          break;
        }
        case dx::RootParamKind::DescriptorTable:
          IS.CmdList->SetComputeRootDescriptorTable(RootParamIndex++, Handle);
          Handle.Offset(P.Sets[DescriptorTableIndex++].Resources.size(), Inc);
          break;
        case dx::RootParamKind::RootDescriptor:
          assert(RootDescIt != IS.RootResources.end());
          if (RootDescIt->first->BufferPtr->ArraySize != 1)
            return llvm::createStringError(
                std::errc::value_too_large,
                "Root descriptor cannot refer to resource arrays.");
          switch (getDXKind(RootDescIt->first->Kind)) {
          case SRV:
            IS.CmdList->SetComputeRootShaderResourceView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case UAV:
            IS.CmdList->SetComputeRootUnorderedAccessView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
          case CBV:
            IS.CmdList->SetComputeRootConstantBufferView(
                RootParamIndex++,
                RootDescIt->second.back().Buffer->GetGPUVirtualAddress());
            break;
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
        IS.CmdList->SetComputeRootDescriptorTable(Idx, Handle);
        Handle.Offset(P.Sets[Idx].Resources.size(), Inc);
      }
    }

    const llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);

    IS.CmdList->Dispatch(DispatchSize[0], DispatchSize[1], DispatchSize[2]);

    auto CopyBackResource = [&IS, this](ResourcePair &R) {
      if (R.first->isTexture()) {
        const offloadtest::Buffer &B = *R.first->BufferPtr;
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
          IS.CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);
          addReadbackEndBarrier(IS, RS.Buffer);
        }
        return;
      }
      for (const ResourceSet &RS : R.second) {
        if (RS.Readback == nullptr)
          continue;
        addReadbackBeginBarrier(IS, RS.Buffer);
        IS.CmdList->CopyResource(RS.Readback.Get(), RS.Buffer.Get());
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

  llvm::Error readBack(InvocationState &IS) {
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
    return llvm::Error::success();
  }

  llvm::Error executeProgram(Pipeline &P) override {

    llvm::sys::AddSignalHandler(
        [](void *Cookie) {
          ID3D12Device *Device = (ID3D12Device *)Cookie;

          ComPtr<ID3D12InfoQueue> InfoQueue;
          HRESULT HR = Device->QueryInterface(InfoQueue.GetAddressOf());
          if (FAILED(HR)) {
            llvm::errs() << "Failed to query D3D info queue\n";
            return;
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
        },
        (void *)Device.Get());

    InvocationState State;
    llvm::outs() << "Configuring execution on device: " << Description << "\n";
    if (auto Err = createRootSignature(P, State))
      return Err;
    llvm::outs() << "RootSignature created.\n";
    if (auto Err = createDescriptorHeap(P, State))
      return Err;
    llvm::outs() << "Descriptor heap created.\n";
    if (auto Err = createPSO(P.Shaders[0].Shader->getBuffer(), State))
      return Err;
    llvm::outs() << "PSO created.\n";
    if (auto Err = createCommandStructures(State))
      return Err;
    llvm::outs() << "Command structures created.\n";
    if (auto Err = createBuffers(P, State))
      return Err;
    llvm::outs() << "Buffers created.\n";
    if (auto Err = createEvent(State))
      return Err;
    llvm::outs() << "Event prepared.\n";
    if (auto Err = createComputeCommands(P, State))
      return Err;
    llvm::outs() << "Compute command list created.\n";
    if (auto Err = executeCommandList(State))
      return Err;
    llvm::outs() << "Compute commands executed.\n";
    if (auto Err = readBack(State))
      return Err;
    llvm::outs() << "Read data back.\n";

    return llvm::Error::success();
  }
};
} // namespace

llvm::Error Device::initializeDXDevices(const DeviceConfig Config) {
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
    auto ShPtr = std::make_shared<DXDevice>(*ExDevice);
    Device::registerDevice(std::static_pointer_cast<Device>(ShPtr));
  }
  return llvm::Error::success();
}
