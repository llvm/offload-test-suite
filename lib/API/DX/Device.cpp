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
#include "API/DX/AccelerationStructure.h"
#include "API/DX/Buffer.h"
#include "API/DX/CommandBuffer.h"
#include "API/DX/Descriptors.h"
#include "API/DX/Device.h"
#include "API/DX/Encoder.h"
#include "API/DX/MemoryHeap.h"
#include "API/DX/PipelineState.h"
#include "API/DX/Queue.h"
#include "API/DX/RenderPass.h"
#include "API/DX/SBT.h"
#include "API/DX/Sampler.h"
#include "API/DX/Texture.h"
#include "API/FormatConversion.h"
#include "API/Util.h"
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

#include "../Support/OffloadMigration.h"

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>

using namespace offloadtest;
using Microsoft::WRL::ComPtr;

template <> char CapabilityValueEnum<directx::ShaderModel>::ID = 0;
template <> char CapabilityValueEnum<directx::RootSignature>::ID = 0;
template <> char CapabilityValueEnum<directx::MeshShaderTier>::ID = 0;
template <> char CapabilityValueEnum<directx::RaytracingTier>::ID = 0;

static std::mutex SignalHandlerMutex;
static llvm::SmallVector<ID3D12DeviceX *> SignalHandlerDevices;

static size_t getCBVSize(size_t Sz) { return (Sz + 255u) & 0xFFFFFFFFFFFFFF00; }

static D3D12_HIT_GROUP_TYPE getDXHitGroupType(HitGroupType T) {
  switch (T) {
  case HitGroupType::Triangles:
    return D3D12_HIT_GROUP_TYPE_TRIANGLES;
  case HitGroupType::Procedural:
    return D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
  }
  llvm_unreachable("All HitGroupType cases handled");
}

static std::wstring widen(llvm::StringRef S) {
  // Entry-point names and hit-group names are ASCII; a straight 1:1 widen
  // is sufficient.
  return std::wstring(S.begin(), S.end());
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

static D3D12_PRIMITIVE_TOPOLOGY_TYPE
getDXPrimitiveTopologyType(PrimitiveTopology Topology) {
  switch (Topology) {
  case PrimitiveTopology::TriangleList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  case PrimitiveTopology::PointList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  case PrimitiveTopology::PatchList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
  case PrimitiveTopology::LineList:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  }
  llvm_unreachable("All PrimitiveTopology cases handled");
}

static D3D_PRIMITIVE_TOPOLOGY
getDXPrimitiveTopology(PrimitiveTopology Topology,
                       std::optional<uint32_t> PatchControlPoints) {
  switch (Topology) {
  case PrimitiveTopology::TriangleList:
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  case PrimitiveTopology::PointList:
    return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  case PrimitiveTopology::PatchList:
    // _N_CONTROL_POINT_PATCHLIST enums are contiguous from 1..32.
    assert(PatchControlPoints && *PatchControlPoints >= 1 &&
           *PatchControlPoints <= 32);
    return static_cast<D3D_PRIMITIVE_TOPOLOGY>(
        D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST +
        (*PatchControlPoints - 1));
  case PrimitiveTopology::LineList:
    return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  }
  llvm_unreachable("All PrimitiveTopology cases handled");
}

static D3D12_FILTER getDXFilterMode(FilterMode MinFilter, FilterMode MagFilter,
                                    bool IsComparison) {
  if (IsComparison) {
    if (MinFilter == FilterMode::Nearest)
      return MagFilter == FilterMode::Nearest
                 ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT
                 : D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
    else
      return MagFilter == FilterMode::Nearest
                 ? D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT
                 : D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
  } else {
    if (MinFilter == FilterMode::Nearest)
      return MagFilter == FilterMode::Nearest
                 ? D3D12_FILTER_MIN_MAG_MIP_POINT
                 : D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    else
      return MagFilter == FilterMode::Nearest
                 ? D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT
                 : D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
  }
}

static D3D12_TEXTURE_ADDRESS_MODE getDXTextureAddressMode(AddressMode Mode) {
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
  llvm_unreachable("All cases handled.");
}

static D3D12_COMPARISON_FUNC getDXComparisonFunc(CompareFunction ComparisonOp) {
  switch (ComparisonOp) {
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
  llvm_unreachable("All cases handled.");
}

llvm::Expected<DescriptorAllocator> DescriptorAllocator::create(
    ID3D12DeviceX *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t Capacity) {
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

llvm::Expected<D3D12_CPU_DESCRIPTOR_HANDLE> DescriptorAllocator::allocate() {
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

DXDevice::DXDevice(ComPtr<IDXCoreAdapter> A, ComPtr<ID3D12DeviceX> D, DXQueue Q,
                   DescriptorAllocator RTVAllocator,
                   DescriptorAllocator DSVAllocator,
                   DescriptorAllocator CSUAllocator,
                   DescriptorAllocator SamplerAllocator, std::string Desc,
                   std::string DriverVer)
    : Adapter(A), Device(D), GraphicsQueue(std::move(Q)),
      RTVAllocator(std::move(RTVAllocator)),
      DSVAllocator(std::move(DSVAllocator)),
      CSUAllocator(std::move(CSUAllocator)),
      SamplerAllocator(std::move(SamplerAllocator)) {
  Description = std::move(Desc);
  DriverVersion = std::move(DriverVer);
  DriverName = "DirectX";

  DXCoreHardwareID HardwareID;
  if (SUCCEEDED(Adapter->GetProperty(DXCoreAdapterProperty::HardwareID,
                                     &HardwareID))) {
    // 0x8086 is the Vendor ID for Intel
    if (HardwareID.vendorID == 0x8086) {
      FamilyPrefix = static_cast<uint16_t>(HardwareID.deviceID) & 0xFF00;
      const IntelGpuEra Era =
          getIntelGpuEra(static_cast<uint16_t>(HardwareID.deviceID));
      if (Era == IntelGpuEra::Gen7_to_10)
        GPUGeneration = "Intel Gen7-10";
      else if (Era == IntelGpuEra::Gen11_to_14_and_Xe)
        GPUGeneration = "Intel Gen11-14/Xe";
      else
        GPUGeneration = "Intel Unknown";
    } else {
      // We don't have a need yet to identify other GPU vendors.
      GPUGeneration = "Unknown";
    }
  }
}

DXDevice::~DXDevice() {
  const std::lock_guard<std::mutex> Lock(SignalHandlerMutex);
  llvm::erase(SignalHandlerDevices, Device.Get());
}

llvm::StringRef DXDevice::getAPIName() const { return "DirectX"; }
GPUAPI DXDevice::getAPI() const { return GPUAPI::DirectX; }

bool DXDevice::classof(const offloadtest::Device *D) {
  return D->getAPI() == GPUAPI::DirectX;
}

Queue &DXDevice::getGraphicsQueue() { return GraphicsQueue; }

llvm::Error DXDevice::createRootSignatureFromShader(
    llvm::StringRef Name, const ShaderContainer &Shader,
    ComPtr<ID3D12RootSignature> &OutRootSignature,
    DescriptorSetsLayout &Layout) {
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

    const std::wstring WStr(Name.begin(), Name.end());
    OutRootSignature->SetName(WStr.c_str());

    // Deserialize the root signature to determine how we need to bind
    // descriptor tables
    ComPtr<ID3D12VersionedRootSignatureDeserializer> Deserializer;
    if (auto Err = HR::toError(
            D3D12CreateVersionedRootSignatureDeserializer(
                Binary.data(), Binary.size(), IID_PPV_ARGS(&Deserializer)),
            "Failed to create Root Signature Deserializer"))
      return Err;

    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC *RootSigDesc = nullptr;
    if (auto Err = HR::toError(Deserializer->GetRootSignatureDescAtVersion(
                                   D3D_ROOT_SIGNATURE_VERSION_1, &RootSigDesc),
                               "Failed to deseralize root signature"))
      return Err;

    for (uint32_t I = 0; I < RootSigDesc->Desc_1_0.NumParameters; ++I) {
      const D3D12_ROOT_PARAMETER &Parameter =
          RootSigDesc->Desc_1_0.pParameters[I];
      switch (Parameter.ParameterType) {
      case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE: {
        uint32_t DescriptorCount = 0;
        for (uint32_t I = 0; I < Parameter.DescriptorTable.NumDescriptorRanges;
             ++I) {
          const uint32_t RangeSize =
              Parameter.DescriptorTable.pDescriptorRanges[I].NumDescriptors;
          if (RangeSize == UINT_MAX)
            DescriptorCount += 1024; // Force unbounded arrays to 1024 entries
                                     // in this framework
          else
            DescriptorCount += RangeSize;
        }

        if (Parameter.DescriptorTable.NumDescriptorRanges > 0 &&
            Parameter.DescriptorTable.pDescriptorRanges[0].RangeType ==
                D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
          Layout.RSigLayout.push_back(RootSignatureLayout(
              RootParameterType::SamplerTable, DescriptorCount));
          Layout.Sets.push_back({0, DescriptorCount});
        } else {
          Layout.RSigLayout.push_back(RootSignatureLayout(
              RootParameterType::DescriptorTable, DescriptorCount));
          Layout.Sets.push_back({DescriptorCount, 0});
        }
        break;
      }
      case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
        Layout.RSigLayout.push_back(RootSignatureLayout(
            RootParameterType::Constant, Parameter.Constants.Num32BitValues));
        break;
      case D3D12_ROOT_PARAMETER_TYPE_CBV:
        Layout.RSigLayout.push_back(
            RootSignatureLayout(RootParameterType::CBV, 1));
        break;
      case D3D12_ROOT_PARAMETER_TYPE_SRV:
        Layout.RSigLayout.push_back(
            RootSignatureLayout(RootParameterType::SRV, 1));
        break;
      case D3D12_ROOT_PARAMETER_TYPE_UAV:
        Layout.RSigLayout.push_back(
            RootSignatureLayout(RootParameterType::UAV, 1));
        break;
      }
    }
  }

  return llvm::Error::success();
}

llvm::Error DXDevice::createRootSignatureFromBindingsDesc(
    llvm::StringRef Name, const BindingsDesc &BndDesc, bool IsGraphics,
    ComPtr<ID3D12RootSignature> &OutRootSignature,
    DescriptorSetsLayout &Layout) {
  uint32_t DescriptorCount = 0;
  for (auto &D : BndDesc.DescriptorSetDescs)
    DescriptorCount += D.ResourceBindings.size();

  std::vector<D3D12_ROOT_PARAMETER> RootParams;
  const std::unique_ptr<D3D12_DESCRIPTOR_RANGE[]> Ranges(
      new D3D12_DESCRIPTOR_RANGE[DescriptorCount]);
  uint32_t RangeIdx = 0;
  for (const auto &Set : BndDesc.DescriptorSetDescs) {
    uint32_t DescriptorIdx = 0;
    const uint32_t StartRangeIdx = RangeIdx;
    for (const auto &Binding : Set.ResourceBindings) {
      const DescriptorKind Kind = getDescriptorKind(Binding.Kind);
      if (Kind == DescriptorKind::SAMPLER)
        continue;

      switch (Kind) {
      case DescriptorKind::SRV:
        Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        break;
      case DescriptorKind::UAV:
        Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        break;
      case DescriptorKind::CBV:
        Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        break;
      case DescriptorKind::SAMPLER:
        llvm_unreachable("Sampler should have been filtered out.");
        break;
      }
      Ranges.get()[RangeIdx].NumDescriptors = Binding.DescriptorCount;
      Ranges.get()[RangeIdx].BaseShaderRegister = Binding.DXBinding.Register;
      Ranges.get()[RangeIdx].RegisterSpace = Binding.DXBinding.Space;
      Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart = DescriptorIdx;
      RangeIdx++;
      DescriptorIdx += Binding.DescriptorCount;
    }
    const uint32_t RangeCount = static_cast<uint32_t>(RangeIdx - StartRangeIdx);
    if (RangeCount > 0) {
      RootParams.push_back(
          D3D12_ROOT_PARAMETER{D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
                               {D3D12_ROOT_DESCRIPTOR_TABLE{
                                   RangeCount, &Ranges.get()[StartRangeIdx]}},
                               D3D12_SHADER_VISIBILITY_ALL});
      Layout.RSigLayout.push_back(RootSignatureLayout(
          RootParameterType::DescriptorTable, DescriptorIdx));
    }

    uint32_t SamplerDescriptorIdx = 0;
    const uint32_t SamplerStartRangeIdx = RangeIdx;
    for (const auto &Binding : Set.ResourceBindings) {
      const DescriptorKind Kind = getDescriptorKind(Binding.Kind);
      if (Kind != DescriptorKind::SAMPLER)
        continue;

      Ranges.get()[RangeIdx].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
      Ranges.get()[RangeIdx].NumDescriptors = Binding.DescriptorCount;
      Ranges.get()[RangeIdx].BaseShaderRegister = Binding.DXBinding.Register;
      Ranges.get()[RangeIdx].RegisterSpace = Binding.DXBinding.Space;
      Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart =
          SamplerDescriptorIdx;

      assert(Binding.DescriptorCount == 1 && "Manon expected this to be 1.");
      RangeIdx++;
      SamplerDescriptorIdx += Binding.DescriptorCount;
    }

    const uint32_t SamplerRangeCount =
        static_cast<uint32_t>(RangeIdx - SamplerStartRangeIdx);
    if (SamplerRangeCount > 0) {
      RootParams.push_back(D3D12_ROOT_PARAMETER{
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
          {D3D12_ROOT_DESCRIPTOR_TABLE{
              static_cast<uint32_t>(RangeIdx - SamplerStartRangeIdx),
              &Ranges.get()[SamplerStartRangeIdx]}},
          D3D12_SHADER_VISIBILITY_ALL});
      Layout.RSigLayout.push_back(RootSignatureLayout(
          RootParameterType::SamplerTable, SamplerDescriptorIdx));
    }

    Layout.Sets.push_back({
        DescriptorIdx,       // DescriptorCount
        SamplerDescriptorIdx // SamplerCount
    });
  }

  CD3DX12_ROOT_SIGNATURE_DESC Desc;
  Desc.Init(
      static_cast<uint32_t>(RootParams.size()), RootParams.data(), 0, nullptr,
      IsGraphics ? D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
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

  const std::wstring WStr(Name.begin(), Name.end());
  OutRootSignature->SetName(WStr.c_str());

  return llvm::Error::success();
}

llvm::Error
DXDevice::createRootSignature(llvm::StringRef Name, const BindingsDesc &BndDesc,
                              const ShaderContainer &Shader, bool IsGraphics,
                              ComPtr<ID3D12RootSignature> &OutRootSignature,
                              DescriptorSetsLayout &Layout) {
  assert(OutRootSignature.Get() == nullptr);

  if (auto Err =
          createRootSignatureFromShader(Name, Shader, OutRootSignature, Layout))
    return Err;

  if (OutRootSignature.Get() != nullptr)
    return llvm::Error::success();

  return createRootSignatureFromBindingsDesc(Name, BndDesc, IsGraphics,
                                             OutRootSignature, Layout);
}

llvm::Expected<std::unique_ptr<PipelineState>>
DXDevice::createPipelineCs(llvm::StringRef Name, const BindingsDesc &BndDesc,
                           ShaderContainer CS) {
  ComPtr<ID3D12RootSignature> RootSig;
  DescriptorSetsLayout Layout;
  if (auto Err = createRootSignature(Name, BndDesc, CS,
                                     /*IsGraphics=*/false, RootSig, Layout))
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

  return std::make_unique<DXPipelineState>(Name, RootSig, std::move(Layout),
                                           PSO, std::nullopt);
}

llvm::Expected<std::unique_ptr<PipelineState>>
DXDevice::createTraditionalRasterPipeline(
    llvm::StringRef Name, const BindingsDesc &BndDesc,
    const TraditionalRasterPipelineCreateDesc &Desc) {
  assert(Desc.RTFormats.size() <= 8);

  ComPtr<ID3D12RootSignature> RootSig;
  DescriptorSetsLayout Layout;
  if (auto Err = createRootSignature(Name, BndDesc, Desc.VS,
                                     /*IsGraphics=*/true, RootSig, Layout))
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
  if (Desc.HS)
    PSODesc.HS = {Desc.HS->Shader->getBuffer().data(),
                  Desc.HS->Shader->getBuffer().size()};
  if (Desc.DS)
    PSODesc.DS = {Desc.DS->Shader->getBuffer().data(),
                  Desc.DS->Shader->getBuffer().size()};
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
      Name, RootSig, std::move(Layout), PSO,
      getDXPrimitiveTopology(Desc.Topology, Desc.PatchControlPoints));
}

llvm::Expected<std::unique_ptr<PipelineState>>
DXDevice::createMeshShaderRasterPipeline(
    llvm::StringRef Name, const BindingsDesc &BindingsDesc,
    const MeshShaderRasterPipelineCreateDesc &Desc) {
  assert(Desc.RTFormats.size() <= 8);

  ComPtr<ID3D12RootSignature> RootSig;
  DescriptorSetsLayout Layout;
  if (auto Err = createRootSignature(Name, BindingsDesc, Desc.MS,
                                     /*IsGraphics=*/true, RootSig, Layout))
    return Err;

  const D3D12_SHADER_BYTECODE MSBytecode = {Desc.MS.Shader->getBuffer().data(),
                                            Desc.MS.Shader->getBuffer().size()};
  if (MSBytecode.BytecodeLength == 0)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Mesh shader pipeline requires a mesh shader.");

  // The amplification (task) shader is optional.
  D3D12_SHADER_BYTECODE ASBytecode = {};
  if (Desc.AS) {
    assert((*Desc.AS).Shader->getBufferSize() > 0 &&
           "The passed task/amplification shader was empty.");
    ASBytecode = {(*Desc.AS).Shader->getBuffer().data(),
                  (*Desc.AS).Shader->getBuffer().size()};
  }

  // The pixel shader is optional
  D3D12_SHADER_BYTECODE PSBytecode = {};
  if (Desc.PS) {
    assert((*Desc.PS).Shader->getBufferSize() > 0 &&
           "The passed pixel shader was empty.");
    PSBytecode = {(*Desc.PS).Shader->getBuffer().data(),
                  (*Desc.PS).Shader->getBuffer().size()};
  }

  D3D12_RT_FORMAT_ARRAY RTArray = {};
  RTArray.NumRenderTargets = static_cast<UINT>(Desc.RTFormats.size());
  for (size_t I = 0; I < Desc.RTFormats.size(); ++I)
    RTArray.RTFormats[I] = getDXGIFormat(Desc.RTFormats[I]);

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
  Stream.PrimitiveTopologyType = getDXPrimitiveTopologyType(Desc.Topology);
  Stream.RTVFormats = RTArray;
  if (Desc.DSFormat)
    Stream.DSVFormat = getDXGIFormat(*Desc.DSFormat);
  Stream.SampleDesc = SampleDesc;

  const D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc = {sizeof(Stream), &Stream};

  ComPtr<ID3D12PipelineState> PSO;
  if (auto Err = HR::toError(
          Device->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&PSO)),
          "Failed to create mesh shader PSO."))
    return Err;

  return std::make_unique<DXPipelineState>(Name, RootSig, std::move(Layout),
                                           PSO, std::nullopt);
}

llvm::Expected<std::unique_ptr<PipelineState>>
DXDevice::createPipelineRT(llvm::StringRef Name, const BindingsDesc &BndDesc,
                           const RayTracingPipelineCreateDesc &Desc) {
  if (!Desc.Library)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "RayTracingPipelineCreateDesc.Library is "
                                   "null — backend needs a DXIL blob.");

  // Global root signature: try the library's embedded RTS0 part first;
  // fall back to building one from BindingsDesc.
  ShaderContainer LibContainer = {};
  LibContainer.Shader = Desc.Library;
  ComPtr<ID3D12RootSignature> RootSig;
  DescriptorSetsLayout Layout;
  if (auto Err = createRootSignature(Name, BndDesc, LibContainer,
                                     /*IsGraphics=*/false, RootSig, Layout))
    return Err;

  CD3DX12_STATE_OBJECT_DESC SODesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

  // DXIL library subobject — add every Shader's entry point as an export.
  // Wide-string storage must outlive SODesc since the subobject only stores
  // pointers into it.
  auto *Lib = SODesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
  const llvm::StringRef LibBytes = Desc.Library->getBuffer();
  const D3D12_SHADER_BYTECODE Bytecode = {LibBytes.data(), LibBytes.size()};
  Lib->SetDXILLibrary(&Bytecode);
  llvm::SmallVector<std::wstring, 8> WideNames;
  WideNames.reserve(Desc.Shaders.size() + Desc.HitGroups.size());
  for (const auto &Sh : Desc.Shaders) {
    WideNames.push_back(widen(Sh.EntryPoint));
    Lib->DefineExport(WideNames.back().c_str());
  }

  // One hit-group subobject per HitGroup entry.
  for (const auto &HG : Desc.HitGroups) {
    auto *HGObj = SODesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
    HGObj->SetHitGroupType(getDXHitGroupType(HG.Type));
    WideNames.push_back(widen(HG.Name));
    HGObj->SetHitGroupExport(WideNames.back().c_str());
    WideNames.push_back(widen(HG.ClosestHit));
    HGObj->SetClosestHitShaderImport(WideNames.back().c_str());
    if (HG.AnyHit) {
      WideNames.push_back(widen(*HG.AnyHit));
      HGObj->SetAnyHitShaderImport(WideNames.back().c_str());
    }
    if (HG.Intersection) {
      WideNames.push_back(widen(*HG.Intersection));
      HGObj->SetIntersectionShaderImport(WideNames.back().c_str());
    }
  }

  // Pipeline-wide shader config (max payload + max attribute bytes).
  auto *ShaderCfg =
      SODesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
  ShaderCfg->Config(Desc.Config.MaxPayloadSizeInBytes,
                    Desc.Config.MaxAttributeSizeInBytes);

  // Pipeline-wide config (max recursion depth).
  auto *PipelineCfg =
      SODesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
  PipelineCfg->Config(Desc.Config.MaxTraceRecursionDepth);

  // Global root signature.
  auto *GlobalRS =
      SODesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
  GlobalRS->SetRootSignature(RootSig.Get());

  ComPtr<ID3D12StateObject> StateObject;
  if (auto Err = HR::toError(
          Device->CreateStateObject(SODesc, IID_PPV_ARGS(&StateObject)),
          "Failed to create raytracing state object."))
    return Err;

  ComPtr<ID3D12StateObjectProperties> Properties;
  if (auto Err = HR::toError(
          StateObject.As(&Properties),
          "Failed to query ID3D12StateObjectProperties from state object."))
    return Err;

  auto State = std::make_unique<DXRayTracingPipelineState>(
      Name, RootSig, Layout, StateObject, Properties);
  // Cache identifiers up-front. The driver-owned blobs are alive for
  // Properties' lifetime, which lives on the PSO.
  //
  // GetShaderIdentifier only returns non-null for entries that are
  // directly bindable from an SBT record: raygen / miss / callable
  // shaders and hit groups. Closest-hit / any-hit / intersection are
  // bound *through* a hit-group subobject and aren't separately
  // addressable, so skip them.
  for (const auto &Sh : Desc.Shaders) {
    switch (Sh.Stage) {
    case Stages::RayGeneration:
    case Stages::Miss:
    case Stages::Callable:
      break;
    default:
      continue;
    }
    const std::wstring W = widen(Sh.EntryPoint);
    const void *Id = Properties->GetShaderIdentifier(W.c_str());
    if (!Id)
      return llvm::createStringError(
          "GetShaderIdentifier returned null for shader '%s'",
          Sh.EntryPoint.c_str());
    State->ShaderIdentifiers[Sh.EntryPoint] = Id;
  }
  for (const auto &HG : Desc.HitGroups) {
    const std::wstring W = widen(HG.Name);
    const void *Id = Properties->GetShaderIdentifier(W.c_str());
    if (!Id)
      return llvm::createStringError(
          "GetShaderIdentifier returned null for hit group '%s'",
          HG.Name.c_str());
    State->ShaderIdentifiers[HG.Name] = Id;
  }
  return State;
}

llvm::Expected<std::unique_ptr<ShaderBindingTable>>
DXDevice::createShaderBindingTable(const PipelineState &PSO,
                                   const ShaderBindingTableDesc &Desc) {
  if (!llvm::isa<DXRayTracingPipelineState>(&PSO))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "createShaderBindingTable requires a RayTracing PipelineState");
  const auto &DXRTPSO = llvm::cast<DXRayTracingPipelineState>(PSO);

  constexpr uint32_t IdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
  const SBTLayout Layout =
      computeSBTLayout(IdSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT,
                       D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, Desc);
  const uint32_t TotalSize = Layout.TotalSize;
  const llvm::ArrayRef<SBTEntry> RGEntries(&Desc.RayGen, 1);

  // Upload heap so the CPU can write the SBT directly. The state-object
  // identifiers don't need to live in default heap; using upload keeps
  // PR3 simple. A staging copy to default heap is a follow-up.
  const D3D12_HEAP_PROPERTIES HeapProps =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
  const D3D12_RESOURCE_DESC BufDesc = CD3DX12_RESOURCE_DESC::Buffer(TotalSize);
  ComPtr<ID3D12Resource> Buffer;
  if (auto Err = HR::toError(Device->CreateCommittedResource(
                                 &HeapProps, D3D12_HEAP_FLAG_NONE, &BufDesc,
                                 D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                 IID_PPV_ARGS(&Buffer)),
                             "Failed to create SBT buffer."))
    return Err;

  void *Mapped = nullptr;
  const D3D12_RANGE ReadRange{0, 0};
  if (auto Err = HR::toError(Buffer->Map(0, &ReadRange, &Mapped),
                             "Failed to map SBT buffer."))
    return Err;
  std::memset(Mapped, 0, TotalSize);
  auto *MappedBytes = static_cast<uint8_t *>(Mapped);

  auto WriteEntries = [&](uint8_t *Region, llvm::ArrayRef<SBTEntry> Entries,
                          uint32_t Stride) -> llvm::Error {
    for (size_t I = 0; I < Entries.size(); ++I) {
      const auto &E = Entries[I];
      auto It = DXRTPSO.ShaderIdentifiers.find(E.ShaderName);
      if (It == DXRTPSO.ShaderIdentifiers.end())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "SBT references unknown shader/hit-group name: '%s'",
            E.ShaderName.c_str());
      uint8_t *Dst = Region + I * Stride;
      std::memcpy(Dst, It->second, IdSize);
      if (!E.LocalRootData.empty())
        std::memcpy(Dst + IdSize, E.LocalRootData.data(),
                    E.LocalRootData.size());
    }
    return llvm::Error::success();
  };

  auto WriteRegion = [&](const SBTRegionLayout &R,
                         llvm::ArrayRef<SBTEntry> Entries) -> llvm::Error {
    return WriteEntries(MappedBytes + R.Offset, Entries, R.Stride);
  };
  auto UnmapAndReturn = [&](llvm::Error Err) {
    Buffer->Unmap(0, nullptr);
    return Err;
  };
  if (auto Err = WriteRegion(Layout.RayGen, RGEntries))
    return UnmapAndReturn(std::move(Err));
  if (auto Err = WriteRegion(Layout.Miss, Desc.Miss))
    return UnmapAndReturn(std::move(Err));
  if (auto Err = WriteRegion(Layout.HitGroup, Desc.HitGroup))
    return UnmapAndReturn(std::move(Err));
  if (auto Err = WriteRegion(Layout.Callable, Desc.Callable))
    return UnmapAndReturn(std::move(Err));
  Buffer->Unmap(0, nullptr);

  // D3D12_GPU_VIRTUAL_ADDRESS_RANGE / …_AND_STRIDE expect a zero address
  // for empty regions, matching the helper's Size == 0 sentinel.
  const D3D12_GPU_VIRTUAL_ADDRESS Base = Buffer->GetGPUVirtualAddress();
  auto MakeRange = [&](const SBTRegionLayout &R) {
    return D3D12_GPU_VIRTUAL_ADDRESS_RANGE{R.Size ? Base + R.Offset : 0,
                                           R.Size};
  };
  auto MakeRangeAndStride = [&](const SBTRegionLayout &R) {
    return D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE{
        R.Size ? Base + R.Offset : 0, R.Size, R.Stride};
  };
  return std::make_unique<DXShaderBindingTable>(
      Buffer, MakeRange(Layout.RayGen), MakeRangeAndStride(Layout.Miss),
      MakeRangeAndStride(Layout.HitGroup), MakeRangeAndStride(Layout.Callable));
}

llvm::Expected<std::unique_ptr<offloadtest::Fence>>
DXDevice::createFence(llvm::StringRef Name) {
  return DXFence::create(Device.Get(), Name);
}

llvm::Expected<std::unique_ptr<offloadtest::MemoryHeap>>
DXDevice::createMemoryHeap(std::string Name, size_t SizeInBytes) {
  return DXMemoryHeap::create(Device.Get(), Name, SizeInBytes);
}

llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
DXDevice::createBuffer(std::string Name, const BufferCreateDesc &Desc,
                       size_t SizeInBytes) {
  const D3D12_HEAP_TYPE HeapType = getDXHeapType(Desc.Location);
  // This flag is only allowed on GpuOnly memory.
  const D3D12_RESOURCE_FLAGS Flags =
      Desc.Location == MemoryLocation::GpuOnly
          ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
          : D3D12_RESOURCE_FLAG_NONE;

  // Modify the size if needed
  UINT64 CounterOffsetInBytes = 0;
  UINT64 BufferSizeInBytes = SizeInBytes;
  if (Desc.Usage == BufferUsage::ConstantBuffer) {
    if (Desc.HasCounter)
      return llvm::createStringError(
          "Constant Buffers are not allowed to have a counter.");

    BufferSizeInBytes = getCBVSize(BufferSizeInBytes);
  } else if (Desc.HasCounter) {
    if (Desc.AccessType == BufferShaderAccessType::Raw)
      return llvm::createStringError(
          "Raw Resources are not allowed to have a counter.");

    CounterOffsetInBytes =
        llvm::alignTo(BufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);
    BufferSizeInBytes = CounterOffsetInBytes + sizeof(uint32_t);
  }

  const D3D12_HEAP_PROPERTIES HeapProps = CD3DX12_HEAP_PROPERTIES(HeapType);
  const D3D12_RESOURCE_DESC BufferDesc =
      CD3DX12_RESOURCE_DESC::Buffer(BufferSizeInBytes, Flags);

  D3D12_RESOURCE_STATES PreferredState = D3D12_RESOURCE_STATE_COMMON;
  ComPtr<ID3D12Resource> BufferObject;
  if (Desc.Backing == MemoryBacking::Sparse) {
    if (auto Err = HR::toError(Device->CreateReservedResource(
                                   &BufferDesc, D3D12_RESOURCE_STATE_COMMON,
                                   nullptr, IID_PPV_ARGS(&BufferObject)),
                               "Failed to create reserved buffer."))
      return Err;
  } else {
    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    if (HeapType == D3D12_HEAP_TYPE_UPLOAD)
      InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    else if (HeapType == D3D12_HEAP_TYPE_READBACK)
      // As per the readback heap docs
      // > Resources in this heap must be created with
      // > D3D12_RESOURCE_STATE_COPY_DEST, and cannot be changed away from
      // this.
      InitialState = D3D12_RESOURCE_STATE_COPY_DEST;
    PreferredState = InitialState;
    if (auto Err =
            HR::toError(Device->CreateCommittedResource(
                            &HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
                            InitialState, nullptr, IID_PPV_ARGS(&BufferObject)),
                        "Failed to create buffer."))
      return Err;
  }

  const std::wstring WStr(Name.begin(), Name.end());
  BufferObject->SetName(WStr.c_str());

  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = {};
  {
    auto SRVHandleOrErr = CSUAllocator.allocate();
    if (!SRVHandleOrErr)
      return SRVHandleOrErr.takeError();
    SRVHandle = *SRVHandleOrErr;

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (Desc.AccessType) {
    case BufferShaderAccessType::Raw:
      SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
      SRVDesc.Buffer.NumElements = static_cast<uint32_t>(SizeInBytes / 4);
      SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      break;
    case BufferShaderAccessType::Typed:
      SRVDesc.Format = getDXGIFormat(Desc.AccessTypeParams.Fmt);
      SRVDesc.Buffer.NumElements = static_cast<uint32_t>(
          SizeInBytes / getFormatSizeInBytes(Desc.AccessTypeParams.Fmt));
      break;
    case BufferShaderAccessType::Structured:
      assert(Desc.AccessTypeParams.StructureStride > 0 &&
             "Structured buffers must have a Structure Stride.");
      SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
      SRVDesc.Buffer.NumElements = static_cast<uint32_t>(
          SizeInBytes / Desc.AccessTypeParams.StructureStride);
      SRVDesc.Buffer.StructureByteStride =
          Desc.AccessTypeParams.StructureStride;
      break;
    }

    Device->CreateShaderResourceView(BufferObject.Get(), &SRVDesc, SRVHandle);
  }

  D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle = {};
  if ((Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0) {
    auto UAVHandleOrErr = CSUAllocator.allocate();
    if (!UAVHandleOrErr)
      return UAVHandleOrErr.takeError();
    UAVHandle = *UAVHandleOrErr;

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    switch (Desc.AccessType) {
    case BufferShaderAccessType::Raw:
      UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
      UAVDesc.Buffer.NumElements = static_cast<uint32_t>(SizeInBytes / 4);
      UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
      break;
    case BufferShaderAccessType::Typed:
      UAVDesc.Format = getDXGIFormat(Desc.AccessTypeParams.Fmt);
      UAVDesc.Buffer.NumElements = static_cast<uint32_t>(
          SizeInBytes / getFormatSizeInBytes(Desc.AccessTypeParams.Fmt));
      break;
    case BufferShaderAccessType::Structured:
      assert(Desc.AccessTypeParams.StructureStride > 0 &&
             "Structured buffers must have a Structure Stride.");
      UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
      UAVDesc.Buffer.NumElements = static_cast<uint32_t>(
          SizeInBytes / Desc.AccessTypeParams.StructureStride);
      UAVDesc.Buffer.StructureByteStride =
          Desc.AccessTypeParams.StructureStride;
      break;
    }

    ID3D12Resource *CounterObject = nullptr;
    if (Desc.HasCounter) {
      UAVDesc.Buffer.CounterOffsetInBytes = CounterOffsetInBytes;
      CounterObject = BufferObject.Get();
    }

    Device->CreateUnorderedAccessView(BufferObject.Get(), CounterObject,
                                      &UAVDesc, UAVHandle);
  }

  D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle = {};
  if (Desc.Usage == BufferUsage::ConstantBuffer) {
    auto CBVHandleOrErr = CSUAllocator.allocate();
    if (!CBVHandleOrErr)
      return CBVHandleOrErr.takeError();
    CBVHandle = *CBVHandleOrErr;

    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {};
    CBVDesc.BufferLocation = BufferObject->GetGPUVirtualAddress();
    CBVDesc.SizeInBytes = BufferSizeInBytes;

    Device->CreateConstantBufferView(&CBVDesc, CBVHandle);
  }

  return std::make_unique<DXBuffer>(BufferObject, Name, Desc, SizeInBytes,
                                    CounterOffsetInBytes, PreferredState,
                                    SRVHandle, UAVHandle, CBVHandle);
}

llvm::Expected<std::unique_ptr<offloadtest::Texture>>
DXDevice::createTexture(std::string Name, const TextureCreateDesc &Desc) {
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
  if (Desc.Location == MemoryLocation::GpuOnly) {
    if (Desc.Backing == MemoryBacking::Sparse)
      TexDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
    else
      TexDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  } else {
    TexDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  }
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

  D3D12_RESOURCE_STATES InitialState =
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
      D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  if ((Desc.Usage & TextureUsage::Storage))
    InitialState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

  ComPtr<ID3D12Resource> TextureObject;
  if (Desc.Backing == MemoryBacking::Sparse) {
    if (auto Err = HR::toError(Device->CreateReservedResource(
                                   &TexDesc, InitialState, ClearValuePtr,
                                   IID_PPV_ARGS(&TextureObject)),
                               "Failed to create reserved texture."))
      return Err;
  } else {
    if (auto Err = HR::toError(Device->CreateCommittedResource(
                                   &HeapProps, D3D12_HEAP_FLAG_NONE, &TexDesc,
                                   InitialState, ClearValuePtr,
                                   IID_PPV_ARGS(&TextureObject)),
                               "Failed to create texture."))
      return Err;
  }

  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = {};
  {
    auto SRVHandleOrErr = CSUAllocator.allocate();
    if (!SRVHandleOrErr)
      return SRVHandleOrErr.takeError();
    SRVHandle = *SRVHandleOrErr;

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension =
        D3D12_SRV_DIMENSION_TEXTURE2D; // assume this is correct for now.
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Format = getDXGIFormatSRV(Desc.Fmt);
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = Desc.MipLevels;
    SRVDesc.Texture2D.PlaneSlice = 0;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    Device->CreateShaderResourceView(TextureObject.Get(), &SRVDesc, SRVHandle);
  }

  D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle = {};
  if ((TexDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0) {
    auto UAVHandleOrErr = CSUAllocator.allocate();
    if (!UAVHandleOrErr)
      return UAVHandleOrErr.takeError();
    UAVHandle = *UAVHandleOrErr;

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    UAVDesc.Texture2D.MipSlice = 0;
    UAVDesc.Texture2D.PlaneSlice = 0;

    Device->CreateUnorderedAccessView(TextureObject.Get(), nullptr, &UAVDesc,
                                      UAVHandle);
  }
  const D3D12_RESOURCE_STATES PreferredState = InitialState;
  auto Tex = std::make_unique<DXTexture>(TextureObject, Name, Desc,
                                         PreferredState, SRVHandle, UAVHandle);

  const bool IsRT = (Desc.Usage & TextureUsage::RenderTarget) != 0;
  const bool IsDS = (Desc.Usage & TextureUsage::DepthStencil) != 0;
  if (IsRT) {
    auto HandleOrErr = RTVAllocator.allocate();
    if (!HandleOrErr)
      return HandleOrErr.takeError();
    Tex->RTVHandle = *HandleOrErr;
    Device->CreateRenderTargetView(TextureObject.Get(), nullptr,
                                   Tex->RTVHandle);
  }
  if (IsDS) {
    auto HandleOrErr = DSVAllocator.allocate();
    if (!HandleOrErr)
      return HandleOrErr.takeError();
    Tex->DSVHandle = *HandleOrErr;
    Device->CreateDepthStencilView(TextureObject.Get(), nullptr,
                                   Tex->DSVHandle);
  }

  return Tex;
}

llvm::Expected<std::unique_ptr<Sampler>>
DXDevice::createSampler(std::string Name, const SamplerCreateDesc &Desc) {

  auto HandleOrErr = SamplerAllocator.allocate();
  if (!HandleOrErr)
    return HandleOrErr.takeError();
  const D3D12_CPU_DESCRIPTOR_HANDLE Handle = *HandleOrErr;

  const D3D12_TEXTURE_ADDRESS_MODE AddressMode =
      getDXTextureAddressMode(Desc.Address);

  bool IsComparison = false;
  D3D12_COMPARISON_FUNC ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
  if (Desc.Kind == SamplerKind::SamplerComparison) {
    IsComparison = true;
    ComparisonFunc = getDXComparisonFunc(Desc.ComparisonOp);
  }

  const D3D12_SAMPLER_DESC SamplerDesc = {
      getDXFilterMode(Desc.MinFilter, Desc.MagFilter, IsComparison),
      AddressMode, // U
      AddressMode, // V
      AddressMode, // W
      Desc.MipLODBias,
      0, // MaxAnisotropy
      ComparisonFunc,
      {0.0f, 0.0f, 0.0f, 0.0f}, // BorderColor
      Desc.MinLOD,
      Desc.MaxLOD,
  };

  Device->CreateSampler(&SamplerDesc, Handle);

  return std::make_unique<DXSampler>(Name, Desc, Handle);
}

uint32_t DXDevice::getTextureUploadRowStrideInBytes(
    const TextureCreateDesc &Desc) const {
  return getAlignedTexturePitch(Desc.Width, getFormatSizeInBytes(Desc.Fmt));
}

TextureUploadLayout
DXDevice::getTextureUploadLayout(const TextureCreateDesc &Desc) const {
  // Only the fields GetCopyableFootprints consults are needed here; layout,
  // flags, and clear value do not affect the copyable footprint.
  D3D12_RESOURCE_DESC TexDesc = {};
  TexDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  TexDesc.Width = Desc.Width;
  TexDesc.Height = Desc.Height;
  TexDesc.DepthOrArraySize = 1;
  TexDesc.MipLevels = static_cast<UINT16>(Desc.MipLevels);
  TexDesc.Format = getDXGIFormat(Desc.Fmt);
  TexDesc.SampleDesc.Count = 1;

  const uint32_t NumSubresources = Desc.MipLevels;
  llvm::SmallVector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Footprints(
      NumSubresources);
  llvm::SmallVector<UINT> NumRows(NumSubresources);
  llvm::SmallVector<UINT64> RowSizes(NumSubresources);
  UINT64 TotalBytes = 0;
  Device->GetCopyableFootprints(&TexDesc, 0, NumSubresources, 0,
                                Footprints.data(), NumRows.data(),
                                RowSizes.data(), &TotalBytes);

  TextureUploadLayout Layout;
  Layout.TotalSizeInBytes = TotalBytes;
  Layout.Subresources.reserve(NumSubresources);
  for (uint32_t I = 0; I < NumSubresources; ++I) {
    SubresourceFootprint Sub;
    Sub.Offset = Footprints[I].Offset;
    Sub.RowPitchInBytes = Footprints[I].Footprint.RowPitch;
    Sub.RowSizeInBytes = static_cast<uint32_t>(RowSizes[I]);
    Sub.NumRows = NumRows[I];
    Layout.Subresources.push_back(Sub);
  }
  return Layout;
}

llvm::Expected<std::unique_ptr<offloadtest::Device>>
DXDevice::create(ComPtr<IDXCoreAdapter> Adapter, const DeviceConfig &Config) {
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

  auto CSUHeapOrErr = DescriptorAllocator::create(
      Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
  if (!CSUHeapOrErr)
    return CSUHeapOrErr.takeError();

  auto SamplerHeapOrErr = DescriptorAllocator::create(
      Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256);
  if (!SamplerHeapOrErr)
    return SamplerHeapOrErr.takeError();

  return std::make_unique<DXDevice>(
      Adapter, Device, std::move(*GraphicsQueueOrErr), std::move(*RTVHeapOrErr),
      std::move(*DSVHeapOrErr), std::move(*CSUHeapOrErr),
      std::move(*SamplerHeapOrErr), std::string(DescVec.data()),
      std::move(DriverVer));
}

const Capabilities &DXDevice::getCapabilities() {
  if (Caps.empty())
    queryCapabilities();
  return Caps;
}

void DXDevice::queryCapabilities() {
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

llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
DXDevice::createCommandBuffer() {
  auto CBOrErr = DXCommandBuffer::create(Device);
  if (!CBOrErr)
    return CBOrErr.takeError();
  (*CBOrErr)->Dev = this;
  return std::unique_ptr<offloadtest::CommandBuffer>(std::move(*CBOrErr));
}

llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
DXDevice::createRenderPass(const offloadtest::RenderPassDesc &Desc) {
  return std::make_unique<DXRenderPass>(Desc);
}

llvm::Expected<AccelerationStructureSizes>
DXDevice::getBLASBuildSizes(llvm::ArrayRef<TriangleGeometryDesc> Triangles) {
  if (auto Err = validateBLASGeometry(Triangles))
    return Err;

  llvm::SmallVector<D3D12_RAYTRACING_GEOMETRY_DESC> GeomDescs;
  GeomDescs.reserve(Triangles.size());
  for (const auto &T : Triangles) {
    D3D12_RAYTRACING_GEOMETRY_DESC GD = {};
    GD.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    if (T.Opaque)
      GD.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    auto &Tri = GD.Triangles;
    // GPU addresses are not needed for the prebuild size query; they will
    // be populated at build time.
    Tri.VertexBuffer.StrideInBytes = T.VertexStride;
    Tri.VertexCount = T.VertexCount;
    Tri.VertexFormat = getDXGIFormat(T.VertexFormat);

    if (T.IndexBuffer) {
      Tri.IndexCount = T.IndexCount;
      Tri.IndexFormat = getDXGIIndexFormat(T.IdxFormat);
    }

    GeomDescs.push_back(GD);
  }
  return queryBLASPrebuildSize(GeomDescs);
}

llvm::Expected<AccelerationStructureSizes>
DXDevice::getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) {
  if (auto Err = validateBLASGeometry(AABBs))
    return Err;

  llvm::SmallVector<D3D12_RAYTRACING_GEOMETRY_DESC> GeomDescs;
  GeomDescs.reserve(AABBs.size());
  for (const auto &A : AABBs) {
    D3D12_RAYTRACING_GEOMETRY_DESC GD = {};
    GD.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
    if (A.Opaque)
      GD.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    GD.AABBs.AABBs.StrideInBytes = A.AABBStride;
    GD.AABBs.AABBCount = A.AABBCount;

    GeomDescs.push_back(GD);
  }
  return queryBLASPrebuildSize(GeomDescs);
}

AccelerationStructureSizes DXDevice::queryBLASPrebuildSize(
    llvm::ArrayRef<D3D12_RAYTRACING_GEOMETRY_DESC> GeomDescs) {
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
  Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  Inputs.NumDescs = GeomDescs.size();
  Inputs.pGeometryDescs = GeomDescs.data();

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
  Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

  return {Info.ResultDataMaxSizeInBytes, Info.ScratchDataSizeInBytes,
          Info.UpdateScratchDataSizeInBytes};
}

llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
DXDevice::allocateAS(const AccelerationStructureSizes &Sizes,
                     const char *Kind) {
  const uint64_t AlignedSize =
      llvm::alignTo(Sizes.ResultDataMaxSizeInBytes,
                    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
  const D3D12_HEAP_PROPERTIES HeapProps =
      CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  const D3D12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
      AlignedSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

  ComPtr<ID3D12Resource> ASBuffer;
  if (auto Err = HR::toError(
          Device->CreateCommittedResource(
              &HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
              D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr,
              IID_PPV_ARGS(&ASBuffer)),
          "Failed to create " + llvm::Twine(Kind) + " resource."))
    return Err;

  return std::make_unique<DXAccelerationStructure>(ASBuffer, Sizes);
}

llvm::Expected<AccelerationStructureSizes>
DXDevice::getTLASBuildSizes(uint32_t InstanceCount) {
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
  Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  Inputs.NumDescs = InstanceCount;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = {};
  Device->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

  return AccelerationStructureSizes{Info.ResultDataMaxSizeInBytes,
                                    Info.ScratchDataSizeInBytes,
                                    Info.UpdateScratchDataSizeInBytes};
}

llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
DXDevice::createBLAS(const AccelerationStructureSizes &Sizes) {
  return allocateAS(Sizes, "BLAS");
}

llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
DXDevice::createTLAS(const AccelerationStructureSizes &Sizes,
                     uint32_t /*InstanceCount*/) {
  auto TLASOrErr = allocateAS(Sizes, "TLAS");
  if (!TLASOrErr)
    return TLASOrErr.takeError();
  auto TLAS = std::move(*TLASOrErr);
  DXAccelerationStructure &TLASDX =
      llvm::cast<DXAccelerationStructure>(*TLAS.get());

  auto SRVHandleOrErr = CSUAllocator.allocate();
  if (!SRVHandleOrErr)
    return SRVHandleOrErr.takeError();
  TLASDX.SRVHandle = *SRVHandleOrErr;

  D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
  SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
  SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
  SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  SRVDesc.RaytracingAccelerationStructure.Location =
      TLASDX.getGPUVirtualAddress();

  // AS SRVs are created with a null resource; the AS lives in the
  // buffer referenced by Location.
  Device->CreateShaderResourceView(nullptr, &SRVDesc, TLASDX.SRVHandle);

  return TLAS;
}

llvm::Expected<std::unique_ptr<DescriptorPool>>
DXDevice::createDescriptorPool() {
  return DXDescriptorPool::create(Device);
}

llvm::Expected<std::unique_ptr<DescriptorSetsBuilder>>
DXDevice::createDescriptorSetsBuilder(DescriptorPool &Pool,
                                      const PipelineState &Pipeline) {
  DXDescriptorPool &PoolDX = llvm::cast<DXDescriptorPool>(Pool);
  const DXPipelineState &PipelineDX = llvm::cast<DXPipelineState>(Pipeline);

  llvm::SmallVector<DXDescriptorSet> Sets;
  for (const auto &Counts : PipelineDX.Layout.Sets) {
    DXDescriptorSet Set = {};
    if (Counts.DescriptorCount > 0)
      PoolDX.allocateDescriptors(Counts.DescriptorCount, Set.CSUHandle,
                                 Set.CSUHandleGPU);
    if (Counts.SamplerCount > 0)
      PoolDX.allocateSamplers(Counts.SamplerCount, Set.SamplerHandle,
                              Set.SamplerHandleGPU);
    Sets.push_back(Set);
  }

  const uint32_t CSUIncSize = Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const uint32_t SamplerIncSize = Device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

  return std::make_unique<DXDescriptorSetsBuilder>(Device, std::move(Sets),
                                                   CSUIncSize, SamplerIncSize);
}

static llvm::Error createComputeCommands(Pipeline &P, SharedInvocationState &IS,
                                         Device &Dev, DescriptorPool &Pool) {
  IS.CB->bindPool(Pool);

  auto DescSetsOrErr =
      buildDescriptorSets(Dev, Pool, *IS.Pipeline, IS.DescTables);
  if (!DescSetsOrErr)
    return DescSetsOrErr.takeError();
  auto DescSets = std::move(*DescSetsOrErr);

  if (P.Settings.DX.RootParams.size() > 0) {
    const DXCommandBuffer &DXCB = llvm::cast<DXCommandBuffer>(*IS.CB);
    const DXDescriptorPool &PoolDX = llvm::cast<DXDescriptorPool>(Pool);
    const DXPipelineState &DXPipeline =
        llvm::cast<DXPipelineState>(*IS.Pipeline.get());

    CD3DX12_GPU_DESCRIPTOR_HANDLE Handle(PoolDX.CSUHandleGPU);
    const uint32_t Inc = PoolDX.CSUIncSize;
    DXCB.CmdList->SetComputeRootSignature(DXPipeline.RootSig.Get());

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
        DXCB.CmdList->SetComputeRoot32BitConstants(
            RootParamIndex++, NumValues, Constant.BufferPtr->Data.back().get(),
            ConstantOffset);
        ConstantOffset += NumValues;
        break;
      }
      case dx::RootParamKind::DescriptorTable:
        // TODO(manon): Add support for descriptor tables containing samplers
        DXCB.CmdList->SetComputeRootDescriptorTable(RootParamIndex++, Handle);
        Handle.Offset(P.Sets[DescriptorTableIndex++].Resources.size(), Inc);
        break;
      case dx::RootParamKind::RootDescriptor:
        assert(RootDescIt != IS.RootResources.end());
        if (RootDescIt->first->getArraySize() != 1)
          return llvm::createStringError(
              std::errc::value_too_large,
              "Root descriptor cannot refer to resource arrays.");

        const DXBuffer *BufferDX = llvm::cast_if_present<DXBuffer>(
            RootDescIt->second.back().Buffer.get());
        if (!BufferDX) {
          return llvm::createStringError(
              std::errc::value_too_large,
              "Root descriptor can only refer to buffers.");
        }

        const D3D12_GPU_VIRTUAL_ADDRESS VirtualAddress =
            BufferDX->Buffer->GetGPUVirtualAddress();
        switch (getDescriptorKind(RootDescIt->first->Kind)) {
        case DescriptorKind::SRV:
          DXCB.CmdList->SetComputeRootShaderResourceView(RootParamIndex++,
                                                         VirtualAddress);
          break;
        case DescriptorKind::UAV:
          DXCB.CmdList->SetComputeRootUnorderedAccessView(RootParamIndex++,
                                                          VirtualAddress);
          break;
        case DescriptorKind::CBV:
          DXCB.CmdList->SetComputeRootConstantBufferView(RootParamIndex++,
                                                         VirtualAddress);
          break;
        case DescriptorKind::SAMPLER:
          llvm_unreachable(
              "Samplers cannot be written directly into the Root Signature.");
        }
        ++RootDescIt;
        break;
      }
    }
  }

  auto EncoderOrErr = IS.CB->createComputeEncoder();
  if (!EncoderOrErr)
    return EncoderOrErr.takeError();
  auto &Encoder = *EncoderOrErr.get();

  if (P.isRayTracing()) {
    if (P.Settings.DX.RootParams.empty())
      Encoder.bindRayTracingDescriptorSets(*IS.Pipeline, *DescSets);

    if (auto Err = Encoder.dispatchRays(
            *IS.Pipeline, *IS.SBT, P.DispatchParameters.DispatchGroupCount[0],
            P.DispatchParameters.DispatchGroupCount[1],
            P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
  } else {
    if (P.Settings.DX.RootParams.empty())
      Encoder.bindComputeDescriptorSets(*IS.Pipeline, *DescSets);

    if (auto Err = Encoder.dispatch(*IS.Pipeline.get(),
                                    P.DispatchParameters.DispatchGroupCount[0],
                                    P.DispatchParameters.DispatchGroupCount[1],
                                    P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
  }
  Encoder.endEncoding();

  return llvm::Error::success();
}

static llvm::Error createGraphicsCommands(Pipeline &P,
                                          SharedInvocationState &IS,
                                          Device &Dev, DescriptorPool &Pool) {
  IS.CB->bindPool(Pool);
  auto DescSetsOrErr =
      buildDescriptorSets(Dev, Pool, *IS.Pipeline, IS.DescTables);
  if (!DescSetsOrErr)
    return DescSetsOrErr.takeError();
  auto DescSets = std::move(*DescSetsOrErr);

  RenderPassBeginDesc BeginDesc = {};
  BeginDesc.Pass = IS.RenderPass.get();
  BeginDesc.ColorAttachments.push_back(IS.RenderTarget.get());
  BeginDesc.DepthStencil = IS.DepthStencil.get();

  auto EncOrErr = IS.CB->createRenderEncoder(BeginDesc);
  if (!EncOrErr)
    return EncOrErr.takeError();
  auto &Encoder = *EncOrErr.get();

  Encoder.bindDescriptorSets(*IS.Pipeline, *DescSets);

  Viewport VP;
  VP.Width = static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Width);
  VP.Height =
      static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Height);
  Encoder.setViewport(VP);

  ScissorRect Scissor;
  Scissor.Width = static_cast<uint32_t>(VP.Width);
  Scissor.Height = static_cast<uint32_t>(VP.Height);
  Encoder.setScissor(Scissor);

  if (P.isTraditionalRaster()) {
    if (IS.VB)
      Encoder.setVertexBuffer(0, IS.VB.get(), 0, P.Bindings.getVertexStride());

    if (auto Err = Encoder.drawInstanced(*IS.Pipeline.get(), P.getVertexCount(),
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

  return llvm::Error::success();
}

llvm::Error DXDevice::executeProgram(Pipeline &P) {
  llvm::outs() << "Configuring execution on device: " << Description << "\n";

  auto DescriptorPoolOrErr = createDescriptorPool();
  if (!DescriptorPoolOrErr)
    return DescriptorPoolOrErr.takeError();
  auto DescriptorPool = std::move(*DescriptorPoolOrErr);

  SharedInvocationState State;
  auto CBOrErr = createCommandBuffer();
  if (!CBOrErr)
    return CBOrErr.takeError();
  State.CB = std::move(*CBOrErr);
  llvm::outs() << "Command buffer created.\n";

  if (auto Err = createResources(*this, P, State))
    return Err;
  llvm::outs() << "Buffers created.\n";

  if (!P.AccelStructs.BLAS.empty() || !P.AccelStructs.TLAS.empty()) {
    auto EncOrErr = State.CB->createComputeEncoder();
    if (!EncOrErr)
      return EncOrErr.takeError();
    if (auto Err = offloadtest::buildPipelineAccelerationStructures(
            *this, **EncOrErr, P, State.BLASes, State.TLASes,
            State.ASInputBuffers))
      return Err;
    (*EncOrErr)->endEncoding();
  }

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
    if (auto Err = createComputeCommands(P, State, *this, *DescriptorPool))
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
      PipelineDesc.PatchControlPoints = P.Bindings.PatchControlPoints;
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
      MeshShaderRasterPipelineCreateDesc PipelineDesc = {};
      PipelineDesc.Topology = P.Bindings.Topology;
      PipelineDesc.DSFormat = Format::D32FloatS8Uint;
      for (auto &Shader : P.Shaders) {
        ShaderContainer SC = {};
        SC.EntryPoint = Shader.Entry;
        SC.Shader = Shader.Shader.get();
        PipelineDesc.setShader(Shader.Stage, std::move(SC));
      }

      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();
      PipelineDesc.RTFormats.push_back(*FormatOrErr);

      auto PipelineStateOrErr = createMeshShaderRasterPipeline(
          "Mesh Shader Pipeline State", BndDesc, PipelineDesc);

      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Mesh Shader Pipeline created.\n";
    }

    if (auto Err = createGraphicsCommands(P, State, *this, *DescriptorPool))
      return Err;
    llvm::outs() << "Graphics command list created complete.\n";
  } else if (P.isRayTracing()) {
    if (P.Shaders.empty() || !P.SBT || !P.RTConfig)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RayTracing pipeline requires Shaders, "
          "ShaderBindingTable, and RayTracingPipelineConfig.");

    RayTracingPipelineCreateDesc RTDesc{};
    RTDesc.Library = P.Shaders.front().Shader.get();
    RTDesc.HitGroups = P.HitGroups;
    RTDesc.Config = *P.RTConfig;
    RTDesc.Shaders.reserve(P.Shaders.size());
    for (const auto &Sh : P.Shaders)
      RTDesc.Shaders.push_back({Sh.Stage, Sh.Entry});

    auto PSOOrErr =
        createPipelineRT("RayTracing Pipeline State", BndDesc, RTDesc);
    if (!PSOOrErr)
      return PSOOrErr.takeError();
    State.Pipeline = std::move(*PSOOrErr);
    llvm::outs() << "RayTracing Pipeline created.\n";

    auto SBTOrErr = createShaderBindingTable(*State.Pipeline, *P.SBT);
    if (!SBTOrErr)
      return SBTOrErr.takeError();
    State.SBT = std::move(*SBTOrErr);
    llvm::outs() << "Shader Binding Table created.\n";

    if (auto Err = createComputeCommands(P, State, *this, *DescriptorPool))
      return Err;
    llvm::outs() << "RayTracing command list created.\n";
  } else {
    return llvm::createStringError("Pipeline was neither Compute nor Raster");
  }

  auto EncoderOrErr = State.CB->createComputeEncoder();
  if (!EncoderOrErr)
    return EncoderOrErr.takeError();
  auto ReadbackEncoder = std::move(*EncoderOrErr);

  if (State.RenderTarget) {
    if (auto Err = ReadbackEncoder->copyTextureToBuffer(*State.RenderTarget,
                                                        *State.RTReadback))
      return Err;
  }

  for (auto &Table : State.DescTables)
    for (auto &R : Table.Resources)
      if (auto Err = copyBackResource(*ReadbackEncoder, R))
        return Err;

  for (auto &R : State.RootResources)
    if (auto Err = copyBackResource(*ReadbackEncoder, R))
      return Err;

  ReadbackEncoder->endEncoding();

  auto SubmitResult = GraphicsQueue.submit(std::move(State.CB));
  if (!SubmitResult)
    return SubmitResult.takeError();
  llvm::outs() << "Compute commands executed.\n";
  if (auto Err = SubmitResult->waitForCompletion())
    return Err;
  if (auto Err = readBack(*this, P, State))
    return Err;
  llvm::outs() << "Read data back.\n";

  return llvm::Error::success();
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
