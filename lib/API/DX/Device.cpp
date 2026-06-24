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

#include "../Support/OffloadMigration.h"

#include <atomic>
#include <codecvt>
#include <locale>
#include <mutex>

using namespace offloadtest;
using Microsoft::WRL::ComPtr;

using ID3D12DeviceX = ID3D12Device5;
using ID3D12GraphicsCommandListX = ID3D12GraphicsCommandList6;

template <> char CapabilityValueEnum<directx::ShaderModel>::ID = 0;
template <> char CapabilityValueEnum<directx::RootSignature>::ID = 0;
template <> char CapabilityValueEnum<directx::MeshShaderTier>::ID = 0;
template <> char CapabilityValueEnum<directx::RaytracingTier>::ID = 0;

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

namespace {
class DXDevice;

class DXBuffer : public offloadtest::Buffer {
public:
  ComPtr<ID3D12Resource> Buffer;
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;
  uint64_t CounterOffsetInBytes;

  // Contract: If a command on the command buffer needs a resource to be in a
  // different state it should always transition it back to the PreferredState
  // afterwards. The PreferredState is the state of the most common use case for
  // that resource. This allows us to do state transitions without state
  // tracking.
  D3D12_RESOURCE_STATES PreferredState;

  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;
  D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle;
  D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle;

  DXBuffer(ComPtr<ID3D12Resource> Buffer, llvm::StringRef Name,
           BufferCreateDesc Desc, size_t SizeInBytes,
           uint64_t CounterOffsetInBytes, D3D12_RESOURCE_STATES PreferredState,
           D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,
           D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle,
           D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle)
      : offloadtest::Buffer(GPUAPI::DirectX), Buffer(Buffer), Name(Name),
        Desc(Desc), SizeInBytes(SizeInBytes),
        CounterOffsetInBytes(CounterOffsetInBytes),
        PreferredState(PreferredState), SRVHandle(SRVHandle),
        UAVHandle(UAVHandle), CBVHandle(CBVHandle) {}
  DXBuffer(const DXBuffer &) = delete;
  DXBuffer(DXBuffer &&) = delete;
  DXBuffer &operator=(const DXBuffer &) = delete;
  DXBuffer &operator=(DXBuffer &&) = delete;

  size_t getSizeInBytes() const override { return SizeInBytes; }

  size_t querySparseTileSizeInBytes(const Device & /*Dev*/) const override {
    return D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
  }

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

  const BufferCreateDesc &getDesc() const override { return Desc; }

  static bool classof(const offloadtest::Buffer *B) {
    return B->getAPI() == GPUAPI::DirectX;
  }
};

class DXTexture : public offloadtest::Texture {
public:
  ComPtr<ID3D12Resource> Resource;

  // Contract: If a command on the command buffer needs a resource to be in a
  // different state it should always transition it back to the PreferredState
  // afterwards. The PreferredState is the state of the most common use case for
  // that resource. This allows us to do state transitions without state
  // tracking.
  D3D12_RESOURCE_STATES PreferredState;

  // Optional descriptors, depending on Desc.Usage.
  // A zero ptr means no descriptor was created for that view type.
  D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle = {};

  std::string Name;
  TextureCreateDesc Desc;

  DXTexture(ComPtr<ID3D12Resource> Resource, llvm::StringRef Name,
            TextureCreateDesc Desc, D3D12_RESOURCE_STATES PreferredState,
            D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle)
      : offloadtest::Texture(GPUAPI::DirectX), Resource(Resource),
        PreferredState(PreferredState), SRVHandle(SRVHandle),
        UAVHandle(UAVHandle), Name(Name), Desc(Desc) {}

  TileShape querySparseTileShape(const Device &Dev) const override;

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
  // True for pipelines created via createPipelineRT — used by SBT / dispatch
  // code to safely downcast to DXRayTracingPipelineState (parallel to
  // VulkanPipelineState::IsRayTracing).
  bool IsRayTracing = false;

  DXPipelineState(llvm::StringRef Name, ComPtr<ID3D12RootSignature> RootSig,
                  ComPtr<ID3D12PipelineState> PSO,
                  std::optional<D3D_PRIMITIVE_TOPOLOGY> Topology,
                  bool IsRT = false)
      : offloadtest::PipelineState(GPUAPI::DirectX), Name(Name),
        RootSig(RootSig), PSO(PSO), Topology(Topology), IsRayTracing(IsRT) {}

  static bool classof(const offloadtest::PipelineState *B) {
    return B->getAPI() == GPUAPI::DirectX;
  }
};

/// RT pipeline state: holds the ID3D12StateObject + cached
/// ID3D12StateObjectProperties for SBT identifier queries plus a
/// shader-name → identifier-pointer map. The `void *` identifiers are
/// owned by Properties — keep it alive for the SBT's lifetime.
class DXRayTracingPipelineState : public DXPipelineState {
public:
  ComPtr<ID3D12StateObject> StateObject;
  ComPtr<ID3D12StateObjectProperties> Properties;
  llvm::StringMap<const void *> ShaderIdentifiers;

  DXRayTracingPipelineState(llvm::StringRef Name,
                            ComPtr<ID3D12RootSignature> RootSig,
                            ComPtr<ID3D12StateObject> SO,
                            ComPtr<ID3D12StateObjectProperties> Props)
      : DXPipelineState(Name, RootSig, /*PSO=*/nullptr, std::nullopt,
                        /*IsRT=*/true),
        StateObject(SO), Properties(Props) {}

  static bool classof(const offloadtest::PipelineState *B) {
    if (B->getAPI() != GPUAPI::DirectX)
      return false;
    return static_cast<const DXPipelineState *>(B)->IsRayTracing;
  }
};

class DXShaderBindingTable : public offloadtest::ShaderBindingTable {
public:
  ComPtr<ID3D12Resource> Buffer;
  // Pre-built ranges for D3D12_DISPATCH_RAYS_DESC. Sizes are zero for
  // empty regions; raygen is a single record so it uses the no-stride
  // variant.
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE RayGenRange{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MissRange{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HitGroupRange{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CallableRange{};

  DXShaderBindingTable(ComPtr<ID3D12Resource> Buf,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE RG,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MS,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HG,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CL)
      : offloadtest::ShaderBindingTable(GPUAPI::DirectX), Buffer(Buf),
        RayGenRange(RG), MissRange(MS), HitGroupRange(HG), CallableRange(CL) {}

  static bool classof(const offloadtest::ShaderBindingTable *S) {
    return S->getAPI() == GPUAPI::DirectX;
  }
};

class DXAccelerationStructure : public offloadtest::AccelerationStructure {
public:
  ComPtr<ID3D12Resource> Resource;
  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;

  DXAccelerationStructure(ComPtr<ID3D12Resource> Resource,
                          const AccelerationStructureSizes &Sizes)
      : offloadtest::AccelerationStructure(GPUAPI::DirectX, Sizes),
        Resource(Resource) {}

  D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress() const {
    return Resource->GetGPUVirtualAddress();
  }

  static bool classof(const offloadtest::AccelerationStructure *AS) {
    return AS->getAPI() == GPUAPI::DirectX;
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

class DXMemoryHeap : public offloadtest::MemoryHeap {
public:
  DXMemoryHeap(ComPtr<ID3D12Heap> Heap, llvm::StringRef Name)
      : offloadtest::MemoryHeap(GPUAPI::DirectX), Name(Name), Heap(Heap) {}

  std::string Name;
  ComPtr<ID3D12Heap> Heap;

  static bool classof(const offloadtest::MemoryHeap *H) {
    return H->getAPI() == GPUAPI::DirectX;
  }

  static llvm::Expected<std::unique_ptr<DXMemoryHeap>>
  create(ID3D12DeviceX *Device, llvm::StringRef Name, size_t SizeInBytes) {
    D3D12_HEAP_DESC HeapDesc = {};
    HeapDesc.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    HeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    // Heap sizes must be a multiple of the placement alignment (64KB), which
    // is also the tiled-resource tile size.
    HeapDesc.SizeInBytes =
        llvm::alignTo(SizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
    HeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

    ComPtr<ID3D12Heap> Heap;
    if (auto Err =
            HR::toError(Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(&Heap)),
                        "Failed to create memory heap."))
      return Err;

    const std::wstring WStr(Name.begin(), Name.end());
    Heap->SetName(WStr.c_str());

    return std::make_unique<DXMemoryHeap>(Heap, Name);
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

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Buffer &Resource,
                     llvm::ArrayRef<TileMapping> Mappings) override {
    return updateTileMappingsImpl(llvm::cast<DXBuffer>(Resource).Buffer.Get(),
                                  Mappings);
  }

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Texture &Resource,
                     llvm::ArrayRef<TileMapping> Mappings) override {
    return updateTileMappingsImpl(
        llvm::cast<DXTexture>(Resource).Resource.Get(), Mappings);
  }

  // Shared tile-mapping path for buffers and textures: they differ only in how
  // a TileRegion maps onto the underlying ID3D12Resource, so both variants
  // resolve to a native resource and funnel through here.
  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappingsImpl(ID3D12Resource *Resource,
                         llvm::ArrayRef<TileMapping> Mappings) {
    for (const TileMapping &M : Mappings) {
      const D3D12_TILED_RESOURCE_COORDINATE StartCoord = {
          M.Region.TileOffsetX, M.Region.TileOffsetY, M.Region.TileOffsetZ,
          M.Region.Subresource};

      D3D12_TILE_REGION_SIZE RegionSize = {};
      RegionSize.Width = M.Region.NumTilesX;
      RegionSize.Height = static_cast<UINT16>(M.Region.NumTilesY);
      RegionSize.Depth = static_cast<UINT16>(M.Region.NumTilesZ);
      // A box is only needed for multi-dimensional (texture) regions; a buffer
      // is always a single linear run of tiles along X.
      RegionSize.UseBox = M.Region.NumTilesY > 1 || M.Region.NumTilesZ > 1;
      RegionSize.NumTiles =
          M.Region.NumTilesX * M.Region.NumTilesY * M.Region.NumTilesZ;

      // A null heap unbinds the region (reads return zero); otherwise bind it
      // to the heap starting at BackingTileOffset.
      ID3D12Heap *Heap = nullptr;
      D3D12_TILE_RANGE_FLAGS RangeFlag = D3D12_TILE_RANGE_FLAG_NULL;
      UINT HeapRangeStartOffset = 0;
      if (M.Backing) {
        Heap = llvm::cast<DXMemoryHeap>(M.Backing)->Heap.Get();
        RangeFlag = D3D12_TILE_RANGE_FLAG_NONE;
        HeapRangeStartOffset = static_cast<UINT>(M.BackingTileOffset);
      }
      const UINT RangeTileCount = RegionSize.NumTiles;

      Queue->UpdateTileMappings(Resource, 1, &StartCoord, &RegionSize, Heap, 1,
                                &RangeFlag, &HeapRangeStartOffset,
                                &RangeTileCount, D3D12_TILE_MAPPING_FLAG_NONE);
    }

    // UpdateTileMappings runs on the queue timeline (not a command list), so
    // signal the submit fence afterwards exactly like submit() does.
    const uint64_t CurrentCounter = ++FenceCounter;
    if (auto Err =
            HR::toError(Queue->Signal(SubmitFence->Fence.Get(), CurrentCounter),
                        "Failed to signal after tile mapping update."))
      return Err;

    return offloadtest::SubmitResult{SubmitFence.get(), CurrentCounter};
  }
};

class DXDevice; // forward decl — defined below in this same anon ns

class DXCommandBuffer : public offloadtest::CommandBuffer {
public:
  ComPtr<ID3D12CommandAllocator> Allocator;
  ComPtr<ID3D12GraphicsCommandListX> CmdList;
  /// Back-pointer to the owning device. Used by encoders that need access to
  /// device-level resources (e.g. allocating AS scratch buffers).
  DXDevice *Dev = nullptr;
  /// Whether a UAV barrier is pending from a prior compute command.
  bool PendingUAVBarrier = false;
  llvm::SmallVector<D3D12_RESOURCE_BARRIER> PendingTransitions;
  /// Buffers that must outlive command-buffer submission (e.g. AS scratch
  /// and TLAS instance buffers used during builds).
  llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> KeepAliveOwned;

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
  void addResourceTransition(ID3D12Resource *Resource,
                             D3D12_RESOURCE_STATES StateBefore,
                             D3D12_RESOURCE_STATES StateAfter) {

    for (auto TransIt = PendingTransitions.begin(),
              End = PendingTransitions.end();
         TransIt != End; ++TransIt) {
      if (TransIt->Transition.pResource != Resource)
        continue;

      assert(StateBefore == TransIt->Transition.StateAfter);

      if (StateAfter == TransIt->Transition.StateBefore) {
        // We actually need the resource to be in the original state!
        // Remove the transition from the list of Pending Transitions.
        PendingTransitions.erase(TransIt);

        // NOTE: This could cause an execution barrier issue since legacy
        // barriers don't allow us to express execution barriers, cache
        // flushes, and layout transitions separately.
        // As a workaround we add a UAV barrier so an execution barrier and
        // cache flush is inserted.
        addPendingUAVBarrier();
      } else {
        TransIt->Transition.StateAfter = StateAfter;
      }

      return;
    }

    PendingTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
        Resource, StateBefore, StateAfter));
  }

  void flushBarrier() {

    if (PendingUAVBarrier) {
      PendingTransitions.push_back(CD3DX12_RESOURCE_BARRIER::UAV(nullptr));
      PendingUAVBarrier = false;
    }

    if (!PendingTransitions.empty()) {
      CmdList->ResourceBarrier(PendingTransitions.size(),
                               PendingTransitions.data());
      PendingTransitions.clear();
    }
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

  llvm::Error dispatch(const offloadtest::PipelineState &PSO,
                       uint32_t GroupCountX, uint32_t GroupCountY,
                       uint32_t GroupCountZ) override {
    const auto &DXPSO = llvm::cast<DXPipelineState>(PSO);
    addUAVBarrier();
    insertDebugSignpost(llvm::formatv("Dispatch [{0},{1},{2}]", GroupCountX,
                                      GroupCountY, GroupCountZ)
                            .str());
    CB.CmdList->SetPipelineState(DXPSO.PSO.Get());
    CB.CmdList->Dispatch(GroupCountX, GroupCountY, GroupCountZ);
    return llvm::Error::success();
  }

  llvm::Error copyBufferToBuffer(offloadtest::Buffer &Src, size_t SrcOffset,
                                 offloadtest::Buffer &Dst, size_t DstOffset,
                                 size_t Size) override {
    auto &DXSrc = static_cast<DXBuffer &>(Src);
    auto &DXDst = static_cast<DXBuffer &>(Dst);

    // NOTE: Edge case in case of all the following being the case
    // - multiple calls of copyBufferToBuffer with the same Dst Buffer
    // - The Dst Buffer having a PreferredState of
    // D3D12_RESOURCE_STATE_COPY_DEST
    // - Each Src Buffer having a PreferredState of
    // D3D12_RESOURCE_STATE_COPY_SOURCE
    // In that case no barrier would be emitted
    // and a race condition would occur. There are ways to solve this with
    // legacy barriers, but switching to enhanced barriers is a better solution
    // to this problem.

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Buffer.Get(), DXSrc.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_SOURCE);
    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Buffer.Get(), DXDst.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_DEST);
    CB.flushBarrier();

    insertDebugSignpost(llvm::formatv("CopyBuffer {0}B", Size).str());
    CB.CmdList->CopyBufferRegion(DXDst.Buffer.Get(), DstOffset,
                                 DXSrc.Buffer.Get(), SrcOffset, Size);

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Buffer.Get(),
                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                               DXSrc.PreferredState);
    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Buffer.Get(),
                               D3D12_RESOURCE_STATE_COPY_DEST,
                               DXDst.PreferredState);

    return llvm::Error::success();
  }

  llvm::Error copyBufferToTexture(Buffer &Src, Texture &Dst) override {
    auto &DXSrc = llvm::cast<DXBuffer>(Src);
    auto &DXDst = llvm::cast<DXTexture>(Dst);

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Buffer.Get(), DXSrc.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_SOURCE);

    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Resource.Get(), DXDst.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_DEST);
    CB.flushBarrier();

    const uint32_t ElementSize = getFormatSizeInBytes(DXDst.Desc.Fmt);
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
        0,
        CD3DX12_SUBRESOURCE_FOOTPRINT(
            getDXGIFormat(DXDst.Desc.Fmt), DXDst.Desc.Width, DXDst.Desc.Height,
            1, getAlignedTexturePitch(DXDst.Desc.Width, ElementSize))};
    const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(DXDst.Resource.Get(), 0);
    const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(DXSrc.Buffer.Get(), Footprint);
    CB.CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Buffer.Get(),
                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                               DXSrc.PreferredState);

    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Resource.Get(),
                               D3D12_RESOURCE_STATE_COPY_DEST,
                               DXDst.PreferredState);

    return llvm::Error::success();
  }

  llvm::Error copyCounterToBuffer(offloadtest::Buffer &Src,
                                  offloadtest::Buffer &Dst) override {
    auto &DXSrc = llvm::cast<DXBuffer>(Src);
    auto &DXDst = llvm::cast<DXBuffer>(Dst);

    if (!DXSrc.Desc.HasCounter)
      return llvm::createStringError(
          "Counter resource passed does not hvae a counter.");

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Buffer.Get(), DXSrc.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_SOURCE);
    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Buffer.Get(), DXDst.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_DEST);
    CB.flushBarrier();

    insertDebugSignpost("copyCounterToBuffer 4B");
    CB.CmdList->CopyBufferRegion(DXDst.Buffer.Get(), 0, DXSrc.Buffer.Get(),
                                 DXSrc.CounterOffsetInBytes, sizeof(uint32_t));

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Buffer.Get(),
                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                               DXSrc.PreferredState);
    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Buffer.Get(),
                               D3D12_RESOURCE_STATE_COPY_DEST,
                               DXDst.PreferredState);

    return llvm::Error::success();
  }

  llvm::Error copyTextureToBuffer(Texture &Src, Buffer &Dst) override {
    auto &DXSrc = llvm::cast<DXTexture>(Src);
    auto &DXDst = llvm::cast<DXBuffer>(Dst);

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Resource.Get(), DXSrc.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_SOURCE);

    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Buffer.Get(), DXDst.PreferredState,
                               D3D12_RESOURCE_STATE_COPY_DEST);

    CB.flushBarrier();

    const uint32_t ElementSize = getFormatSizeInBytes(DXSrc.Desc.Fmt);
    const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
        0,
        CD3DX12_SUBRESOURCE_FOOTPRINT(
            getDXGIFormat(DXSrc.Desc.Fmt), DXSrc.Desc.Width, DXSrc.Desc.Height,
            1, getAlignedTexturePitch(DXSrc.Desc.Width, ElementSize))};
    const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(DXDst.Buffer.Get(), Footprint);
    const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(DXSrc.Resource.Get(), 0);
    CB.CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);

    if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      CB.addResourceTransition(DXSrc.Resource.Get(),
                               D3D12_RESOURCE_STATE_COPY_SOURCE,
                               DXSrc.PreferredState);

    if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
      CB.addResourceTransition(DXDst.Buffer.Get(),
                               D3D12_RESOURCE_STATE_COPY_DEST,
                               DXDst.PreferredState);

    return llvm::Error::success();
  }

  // Defined out-of-line below — needs DXDevice's full type for access to the
  // ID3D12Device5 entry point and helper allocators.
  llvm::Error batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) override;

  // Defined out-of-line below — needs DXDevice's full type for access to
  // Device5 and the DXRayTracingPipelineState definition.
  llvm::Error dispatchRays(const PipelineState &PSO,
                           const ShaderBindingTable &SBT, uint32_t Width,
                           uint32_t Height, uint32_t Depth) override;

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
  offloadtest::RenderPassBeginDesc Desc;

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
  DXRenderEncoder(DXCommandBuffer &CB,
                  const offloadtest::RenderPassBeginDesc &Desc)
      : RenderEncoder(GPUAPI::DirectX), CB(CB), Desc(Desc) {}
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

  void endEncodingImpl() override {
    // State transitions
    for (offloadtest::Texture *Tex : Desc.ColorAttachments) {
      auto &DXTex = llvm::cast<DXTexture>(*Tex);
      if (DXTex.PreferredState != D3D12_RESOURCE_STATE_RENDER_TARGET)
        CB.addResourceTransition(DXTex.Resource.Get(),
                                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                                 DXTex.PreferredState);
    }
    if (Desc.DepthStencil) {
      auto &DXTex = llvm::cast<DXTexture>(*Desc.DepthStencil);
      if (DXTex.PreferredState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
        CB.addResourceTransition(DXTex.Resource.Get(),
                                 D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                 DXTex.PreferredState);
    }

    popDebugGroup();
  }
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

  // State transitions
  for (offloadtest::Texture *Tex : Desc.ColorAttachments) {
    auto &DXTex = llvm::cast<DXTexture>(*Tex);
    if (DXTex.PreferredState != D3D12_RESOURCE_STATE_RENDER_TARGET)
      this->addResourceTransition(DXTex.Resource.Get(), DXTex.PreferredState,
                                  D3D12_RESOURCE_STATE_RENDER_TARGET);
  }
  if (Desc.DepthStencil) {
    auto &DXTex = llvm::cast<DXTexture>(*Desc.DepthStencil);
    if (DXTex.PreferredState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
      this->addResourceTransition(DXTex.Resource.Get(), DXTex.PreferredState,
                                  D3D12_RESOURCE_STATE_DEPTH_WRITE);
  }

  this->flushBarrier();

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

  auto Enc = std::make_unique<DXRenderEncoder>(*this, Desc);
  Enc->pushDebugGroup("RenderEncoder");
  return Enc;
}

class DXDevice : public offloadtest::Device {
public:
  ComPtr<IDXCoreAdapter> Adapter;
  ComPtr<ID3D12DeviceX> Device;
  DXQueue GraphicsQueue;
  Capabilities Caps;
  DescriptorAllocator RTVAllocator;
  DescriptorAllocator DSVAllocator;
  DescriptorAllocator CSUAllocator;

  DXDevice(ComPtr<IDXCoreAdapter> A, ComPtr<ID3D12DeviceX> D, DXQueue Q,
           DescriptorAllocator RTVAllocator, DescriptorAllocator DSVAllocator,
           DescriptorAllocator CSUAllocator, std::string Desc,
           std::string DriverVer)
      : Adapter(A), Device(D), GraphicsQueue(std::move(Q)),
        RTVAllocator(std::move(RTVAllocator)),
        DSVAllocator(std::move(DSVAllocator)),
        CSUAllocator(std::move(CSUAllocator)) {
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
  DXDevice(const DXDevice &) = delete;
  DXDevice &operator=(const DXDevice &) = delete;

  ~DXDevice() override {
    const std::lock_guard<std::mutex> Lock(SignalHandlerMutex);
    llvm::erase(SignalHandlerDevices, Device.Get());
  }

  llvm::StringRef getAPIName() const override { return "DirectX"; }
  GPUAPI getAPI() const override { return GPUAPI::DirectX; }

  static bool classof(const offloadtest::Device *D) {
    return D->getAPI() == GPUAPI::DirectX;
  }

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
      uint32_t DescriptorIdx = 0;
      const uint32_t StartRangeIdx = RangeIdx;
      for (const auto &Binding : Set.ResourceBindings) {
        switch (getDescriptorKind(Binding.Kind)) {
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
          llvm_unreachable("Not implemented yet."); // Requires a separate heap
        }
        Ranges.get()[RangeIdx].NumDescriptors = Binding.DescriptorCount;
        Ranges.get()[RangeIdx].BaseShaderRegister = Binding.DXBinding.Register;
        Ranges.get()[RangeIdx].RegisterSpace = Binding.DXBinding.Space;
        Ranges.get()[RangeIdx].OffsetInDescriptorsFromTableStart =
            DescriptorIdx;
        RangeIdx++;
        DescriptorIdx += Binding.DescriptorCount;
      }
      RootParams.push_back(D3D12_ROOT_PARAMETER{
          D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
          {D3D12_ROOT_DESCRIPTOR_TABLE{
              static_cast<uint32_t>(Set.ResourceBindings.size()),
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
        Name, RootSig, PSO,
        getDXPrimitiveTopology(Desc.Topology, Desc.PatchControlPoints));
  }

  llvm::Expected<std::unique_ptr<PipelineState>> createMeshShaderRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const MeshShaderRasterPipelineCreateDesc &Desc) override {
    assert(Desc.RTFormats.size() <= 8);

    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BindingsDesc, Desc.MS,
                                       /*IsGraphics=*/true, RootSig))
      return Err;

    const D3D12_SHADER_BYTECODE MSBytecode = {
        Desc.MS.Shader->getBuffer().data(), Desc.MS.Shader->getBuffer().size()};
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

    const D3D12_PIPELINE_STATE_STREAM_DESC StreamDesc = {sizeof(Stream),
                                                         &Stream};

    ComPtr<ID3D12PipelineState> PSO;
    if (auto Err = HR::toError(
            Device->CreatePipelineState(&StreamDesc, IID_PPV_ARGS(&PSO)),
            "Failed to create mesh shader PSO."))
      return Err;

    return std::make_unique<DXPipelineState>(Name, RootSig, PSO, std::nullopt);
  }

  static std::wstring widen(llvm::StringRef S) {
    // Entry-point names and hit-group names are ASCII; a straight 1:1 widen
    // is sufficient.
    return std::wstring(S.begin(), S.end());
  }

  static D3D12_HIT_GROUP_TYPE getDXHitGroupType(HitGroupType T) {
    switch (T) {
    case HitGroupType::Triangles:
      return D3D12_HIT_GROUP_TYPE_TRIANGLES;
    case HitGroupType::Procedural:
      return D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
    }
    llvm_unreachable("All HitGroupType cases handled");
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineRT(llvm::StringRef Name, const BindingsDesc &BndDesc,
                   const RayTracingPipelineCreateDesc &Desc) override {
    if (!Desc.Library)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "RayTracingPipelineCreateDesc.Library is "
                                     "null — backend needs a DXIL blob.");

    // Global root signature: try the library's embedded RTS0 part first;
    // fall back to building one from BindingsDesc.
    ShaderContainer LibContainer = {};
    LibContainer.Shader = Desc.Library;
    ComPtr<ID3D12RootSignature> RootSig;
    if (auto Err = createRootSignature(Name, BndDesc, LibContainer,
                                       /*IsGraphics=*/false, RootSig))
      return Err;

    CD3DX12_STATE_OBJECT_DESC SODesc(
        D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

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
        Name, RootSig, StateObject, Properties);
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
  createShaderBindingTable(const PipelineState &PSO,
                           const ShaderBindingTableDesc &Desc) override {
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
    const D3D12_RESOURCE_DESC BufDesc =
        CD3DX12_RESOURCE_DESC::Buffer(TotalSize);
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
        MakeRangeAndStride(Layout.HitGroup),
        MakeRangeAndStride(Layout.Callable));
  }

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override {
    return DXFence::create(Device.Get(), Name);
  }

  llvm::Expected<std::unique_ptr<offloadtest::MemoryHeap>>
  createMemoryHeap(std::string Name, size_t SizeInBytes) override {
    return DXMemoryHeap::create(Device.Get(), Name, SizeInBytes);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, const BufferCreateDesc &Desc,
               size_t SizeInBytes) override {
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

      CounterOffsetInBytes = llvm::alignTo(
          BufferSizeInBytes, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);
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
      if (auto Err = HR::toError(Device->CreateCommittedResource(
                                     &HeapProps, D3D12_HEAP_FLAG_NONE,
                                     &BufferDesc, InitialState, nullptr,
                                     IID_PPV_ARGS(&BufferObject)),
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
      SRVDesc.Shader4ComponentMapping =
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
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
      SRVDesc.Shader4ComponentMapping =
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      SRVDesc.Format = getDXGIFormatSRV(Desc.Fmt);
      SRVDesc.Texture2D.MostDetailedMip = 0;
      SRVDesc.Texture2D.MipLevels = Desc.MipLevels;
      SRVDesc.Texture2D.PlaneSlice = 0;
      SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;

      Device->CreateShaderResourceView(TextureObject.Get(), &SRVDesc,
                                       SRVHandle);
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
    auto Tex = std::make_unique<DXTexture>(
        TextureObject, Name, Desc, PreferredState, SRVHandle, UAVHandle);

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

  uint32_t getTextureUploadRowStrideInBytes(
      const TextureCreateDesc &Desc) const override {
    return getAlignedTexturePitch(Desc.Width, getFormatSizeInBytes(Desc.Fmt));
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

    auto CSUHeapOrErr = DescriptorAllocator::create(
        Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
    if (!CSUHeapOrErr)
      return CSUHeapOrErr.takeError();

    return std::make_unique<DXDevice>(
        Adapter, Device, std::move(*GraphicsQueueOrErr),
        std::move(*RTVHeapOrErr), std::move(*DSVHeapOrErr),
        std::move(*CSUHeapOrErr), std::string(DescVec.data()),
        std::move(DriverVer));
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

  llvm::Error createDescriptorHeap(Pipeline &P,
                                   ComPtr<ID3D12DescriptorHeap> &DescHeap) {
    if (P.getDescriptorCount() == 0)
      return llvm::Error::success();
    const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        P.getDescriptorCountWithFlattenedArrays(),
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    if (auto Err = HR::toError(
            Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&DescHeap)),
            "Failed to create descriptor heap."))
      return Err;
    return llvm::Error::success();
  }

  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override {
    auto CBOrErr = DXCommandBuffer::create(Device);
    if (!CBOrErr)
      return CBOrErr.takeError();
    (*CBOrErr)->Dev = this;
    return std::unique_ptr<offloadtest::CommandBuffer>(std::move(*CBOrErr));
  }

  llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
  createRenderPass(const offloadtest::RenderPassDesc &Desc) override {
    return std::make_unique<DXRenderPass>(Desc);
  }

  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<TriangleGeometryDesc> Triangles) override {
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
  getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) override {
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

  AccelerationStructureSizes queryBLASPrebuildSize(
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
  allocateAS(const AccelerationStructureSizes &Sizes, const char *Kind) {
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
  getTLASBuildSizes(uint32_t InstanceCount) override {
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
  createBLAS(const AccelerationStructureSizes &Sizes) override {
    return allocateAS(Sizes, "BLAS");
  }

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createTLAS(const AccelerationStructureSizes &Sizes) override {
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
    SRVDesc.ViewDimension =
        D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.RaytracingAccelerationStructure.Location =
        TLASDX.getGPUVirtualAddress();

    // AS SRVs are created with a null resource; the AS lives in the
    // buffer referenced by Location.
    Device->CreateShaderResourceView(nullptr, &SRVDesc, TLASDX.SRVHandle);

    return TLAS;
  }

  static size_t getCBVSize(size_t Sz) {
    return (Sz + 255u) & 0xFFFFFFFFFFFFFF00;
  }

  void buildDescriptorTables(llvm::ArrayRef<DescriptorTable> DescTables,
                             const ComPtr<ID3D12DescriptorHeap> &DescHeap) {
    // Bind descriptors in descriptor tables.
    if (!DescHeap)
      return;
    uint32_t HeapIndex = 0;
    const D3D12_CPU_DESCRIPTOR_HANDLE HeapStart =
        DescHeap->GetCPUDescriptorHandleForHeapStart();
    const uint32_t DescHandleIncSize = Device->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (auto &T : DescTables) {
      for (auto &R : T.Resources) {
        for (const auto &Set : R.second) {
          D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle = {};
          if (Set.Buffer != nullptr) {
            const DXBuffer &BufferDX = llvm::cast<DXBuffer>(*Set.Buffer.get());
            switch (getDescriptorKind(R.first->Kind)) {
            case DescriptorKind::SRV:
              assert(BufferDX.SRVHandle.ptr != 0 &&
                     "Missing SRV Descriptor. Is BufferUsage correct?");
              DescriptorHandle = BufferDX.SRVHandle;
              break;
            case DescriptorKind::UAV:
              assert(BufferDX.UAVHandle.ptr != 0 &&
                     "Missing UAV Descriptor. Is BufferUsage correct?");
              DescriptorHandle = BufferDX.UAVHandle;
              break;
            case DescriptorKind::CBV:
              assert(BufferDX.CBVHandle.ptr != 0 &&
                     "Missing CBV Descriptor. Is BufferUsage correct?");
              DescriptorHandle = BufferDX.CBVHandle;
              break;
            default:
              llvm_unreachable("Invalid DescriptorKind for a Buffer.");
              break;
            }
          } else if (Set.Texture != nullptr) {
            const DXTexture &TextureDX =
                llvm::cast<DXTexture>(*Set.Texture.get());
            switch (getDescriptorKind(R.first->Kind)) {
            case DescriptorKind::SRV:
              assert(TextureDX.SRVHandle.ptr != 0 &&
                     "Missing SRV Descriptor. Is TextureUsage correct?");
              DescriptorHandle = TextureDX.SRVHandle;
              break;
            case DescriptorKind::UAV:
              assert(TextureDX.UAVHandle.ptr != 0 &&
                     "Missing UAV Descriptor. Is TextureUsage correct?");
              DescriptorHandle = TextureDX.UAVHandle;
              break;
            default:
              llvm_unreachable("Invalid DescriptorKind for a Texture.");
              break;
            }
          } else if (Set.AS != nullptr) {
            const DXAccelerationStructure &AccelerationStructureDX =
                llvm::cast<DXAccelerationStructure>(*Set.AS);
            DescriptorHandle = AccelerationStructureDX.SRVHandle;
          } else {
            llvm_unreachable("Resource was a texture nor buffer. Samplers "
                             "are unsupported");
          }

          assert(DescriptorHandle.ptr != 0 &&
                 "Somehow got a null descriptor :(");
          Device->CopyDescriptorsSimple(
              1, {HeapStart.ptr + HeapIndex * DescHandleIncSize},
              DescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
          HeapIndex += 1;
        }
      }
    }
  }

  llvm::Error
  createComputeCommands(Pipeline &P, SharedInvocationState &IS,
                        const ComPtr<ID3D12DescriptorHeap> &DescHeap) {
    DXCommandBuffer &DXCB = llvm::cast<DXCommandBuffer>(*IS.CB);

    CD3DX12_GPU_DESCRIPTOR_HANDLE Handle;
    if (DescHeap) {
      ID3D12DescriptorHeap *const Heaps[] = {DescHeap.Get()};
      DXCB.CmdList->SetDescriptorHeaps(1, Heaps);
      Handle = DescHeap->GetGPUDescriptorHandleForHeapStart();
    }
    const DXPipelineState &DXPipeline =
        llvm::cast<DXPipelineState>(*IS.Pipeline.get());
    DXCB.CmdList->SetComputeRootSignature(DXPipeline.RootSig.Get());

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
          DXCB.CmdList->SetComputeRoot32BitConstants(
              RootParamIndex++, NumValues,
              Constant.BufferPtr->Data.back().get(), ConstantOffset);
          ConstantOffset += NumValues;
          break;
        }
        case dx::RootParamKind::DescriptorTable:
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
        DXCB.CmdList->SetComputeRootDescriptorTable(Idx, Handle);
        Handle.Offset(P.Sets[Idx].Resources.size(), Inc);
      }
    }

    {
      auto EncoderOrErr = IS.CB->createComputeEncoder();
      if (!EncoderOrErr)
        return EncoderOrErr.takeError();
      auto &Encoder = *EncoderOrErr.get();
      if (P.isRayTracing()) {
        if (auto Err = Encoder.dispatchRays(
                *IS.Pipeline, *IS.SBT,
                P.DispatchParameters.DispatchGroupCount[0],
                P.DispatchParameters.DispatchGroupCount[1],
                P.DispatchParameters.DispatchGroupCount[2]))
          return Err;
      } else if (auto Err = Encoder.dispatch(
                     *IS.Pipeline.get(),
                     P.DispatchParameters.DispatchGroupCount[0],
                     P.DispatchParameters.DispatchGroupCount[1],
                     P.DispatchParameters.DispatchGroupCount[2])) {
        return Err;
      }
      Encoder.endEncoding();
    }

    auto EncoderOrErr = IS.CB->createComputeEncoder();
    if (!EncoderOrErr)
      return EncoderOrErr.takeError();
    auto ReadbackEncoder = std::move(*EncoderOrErr);

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        if (auto Err = copyBackResource(*ReadbackEncoder, R))
          return Err;

    for (auto &R : IS.RootResources)
      if (auto Err = copyBackResource(*ReadbackEncoder, R))
        return Err;

    ReadbackEncoder->endEncoding();

    return llvm::Error::success();
  }

  llvm::Error
  createGraphicsCommands(Pipeline &P, SharedInvocationState &IS,
                         const ComPtr<ID3D12DescriptorHeap> &DescHeap) {
    const DXPipelineState &DXPipeline =
        llvm::cast<DXPipelineState>(*IS.Pipeline.get());
    DXCommandBuffer &DXCB = llvm::cast<DXCommandBuffer>(*IS.CB);

    DXCB.CmdList->SetGraphicsRootSignature(DXPipeline.RootSig.Get());
    if (DescHeap) {
      ID3D12DescriptorHeap *const Heaps[] = {DescHeap.Get()};
      DXCB.CmdList->SetDescriptorHeaps(1, Heaps);
      DXCB.CmdList->SetGraphicsRootDescriptorTable(
          0, DescHeap->GetGPUDescriptorHandleForHeapStart());
    }

    RenderPassBeginDesc BeginDesc = {};
    BeginDesc.Pass = IS.RenderPass.get();
    BeginDesc.ColorAttachments.push_back(IS.RenderTarget.get());
    BeginDesc.DepthStencil = IS.DepthStencil.get();

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

    auto EncoderOrErr = IS.CB->createComputeEncoder();
    if (!EncoderOrErr)
      return EncoderOrErr.takeError();
    auto ReadbackEncoder = std::move(*EncoderOrErr);

    if (auto Err = ReadbackEncoder->copyTextureToBuffer(*IS.RenderTarget,
                                                        *IS.RTReadback))
      return Err;

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        if (auto Err = copyBackResource(*ReadbackEncoder, R))
          return Err;

    for (auto &R : IS.RootResources)
      if (auto Err = copyBackResource(*ReadbackEncoder, R))
        return Err;

    ReadbackEncoder->endEncoding();

    return llvm::Error::success();
  }

  llvm::Error executeProgram(Pipeline &P) override {
    llvm::outs() << "Configuring execution on device: " << Description << "\n";

    ComPtr<ID3D12DescriptorHeap> DescHeap;
    if (auto Err = createDescriptorHeap(P, DescHeap))
      return Err;
    llvm::outs() << "Descriptor heap created.\n";

    SharedInvocationState State;
    auto CBOrErr = createCommandBuffer();
    if (!CBOrErr)
      return CBOrErr.takeError();
    State.CB = std::move(*CBOrErr);
    llvm::outs() << "Command buffer created.\n";

    if (auto Err = createResources(*this, P, State))
      return Err;
    llvm::outs() << "Buffers created.\n";

    buildDescriptorTables(State.DescTables, DescHeap);

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
      if (auto Err = createComputeCommands(P, State, DescHeap))
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

      if (auto Err = createGraphicsCommands(P, State, DescHeap))
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

      if (auto Err = createComputeCommands(P, State, DescHeap))
        return Err;
      llvm::outs() << "RayTracing command list created.\n";
    } else {
      return llvm::createStringError("Pipeline was neither Compute nor Raster");
    }

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
};

TileShape DXTexture::querySparseTileShape(const Device &Dev) const {
  const DXDevice &DeviceDX = llvm::cast<DXDevice>(Dev);

  D3D12_TILE_SHAPE Shape = {};
  UINT NumTiles = 0;
  UINT NumSubresourceTilings = 0;
  DeviceDX.Device->GetResourceTiling(Resource.Get(), &NumTiles, nullptr, &Shape,
                                     &NumSubresourceTilings, 0, nullptr);
  return TileShape{Shape.WidthInTexels, Shape.HeightInTexels,
                   Shape.DepthInTexels};
}

llvm::Error DXComputeEncoder::batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) {
  if (Items.empty())
    return llvm::Error::success();
  if (!CB.Dev || !CB.Dev->Device)
    return llvm::createStringError(
        std::errc::not_supported,
        "Ray tracing not supported on this command buffer's device.");
  DXDevice *Dev = CB.Dev;

  // BuildRaytracingAccelerationStructure() lives on ID3D12GraphicsCommandList4.
  ComPtr<ID3D12GraphicsCommandList4> CmdList4;
  if (auto Err = HR::toError(CB.CmdList.As(&CmdList4),
                             "Failed to query ID3D12GraphicsCommandList4."))
    return Err;

  // Flush a pending barrier before reading, like dispatch(): a TLAS build must
  // observe BLASes built in the previous batch.
  CB.flushBarrier();

  // Per the ComputeEncoder::batchBuildAS() contract, the caller guarantees no
  // inter-item memory dependencies within a batch (BLAS and TLAS go in
  // separate batches, so a TLAS never sees BLASes from the same call). Each
  // item also gets its own scratch resource, so there's no aliasing between
  // the builds — no intra-loop UAV barrier is needed.
  for (const auto &Item : Items) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {};
    llvm::SmallVector<D3D12_RAYTRACING_GEOMETRY_DESC> GeomDescs;
    uint64_t ScratchSize = 0;

    if (const auto *BLAS = llvm::dyn_cast<const BLASBuildRequest *>(Item)) {
      auto *DXAS = llvm::cast<DXAccelerationStructure>(BLAS->AS);
      Desc.DestAccelerationStructureData = DXAS->getGPUVirtualAddress();
      Desc.Inputs.Type =
          D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      Desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

      if (const auto *Tris =
              std::get_if<llvm::SmallVector<TriangleGeometryDesc>>(
                  &BLAS->Geometry)) {
        GeomDescs.reserve(Tris->size());
        for (const auto &T : *Tris) {
          D3D12_RAYTRACING_GEOMETRY_DESC GD = {};
          GD.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
          if (T.Opaque)
            GD.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
          auto *VB = llvm::cast<DXBuffer>(T.VertexBuffer);
          GD.Triangles.VertexBuffer.StartAddress =
              VB->Buffer->GetGPUVirtualAddress() + T.VertexBufferOffset;
          GD.Triangles.VertexBuffer.StrideInBytes = T.VertexStride;
          GD.Triangles.VertexCount = T.VertexCount;
          GD.Triangles.VertexFormat = getDXGIFormat(T.VertexFormat);
          if (T.IndexBuffer) {
            auto *IB = llvm::cast<DXBuffer>(T.IndexBuffer);
            GD.Triangles.IndexBuffer =
                IB->Buffer->GetGPUVirtualAddress() + T.IndexBufferOffset;
            GD.Triangles.IndexCount = T.IndexCount;
            GD.Triangles.IndexFormat = getDXGIIndexFormat(T.IdxFormat);
          }
          GeomDescs.push_back(GD);
        }
      } else {
        const auto &AABBs =
            std::get<llvm::SmallVector<AABBGeometryDesc>>(BLAS->Geometry);
        GeomDescs.reserve(AABBs.size());
        for (const auto &A : AABBs) {
          D3D12_RAYTRACING_GEOMETRY_DESC GD = {};
          GD.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
          if (A.Opaque)
            GD.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
          auto *AB = llvm::cast<DXBuffer>(A.AABBBuffer);
          GD.AABBs.AABBs.StartAddress =
              AB->Buffer->GetGPUVirtualAddress() + A.AABBBufferOffset;
          GD.AABBs.AABBs.StrideInBytes = A.AABBStride;
          GD.AABBs.AABBCount = A.AABBCount;
          GeomDescs.push_back(GD);
        }
      }
      Desc.Inputs.NumDescs = static_cast<UINT>(GeomDescs.size());
      Desc.Inputs.pGeometryDescs = GeomDescs.data();
      ScratchSize = BLAS->AS->getSizes().ScratchDataSizeInBytes;
    } else {
      const auto *TLAS = llvm::cast<const TLASBuildRequest *>(Item);
      auto *DXAS = llvm::cast<DXAccelerationStructure>(TLAS->AS);
      Desc.DestAccelerationStructureData = DXAS->getGPUVirtualAddress();
      Desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
      Desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
      Desc.Inputs.NumDescs = static_cast<UINT>(TLAS->Instances.size());

      // D3D12_RAYTRACING_INSTANCE_DESC has the same byte layout as
      // VkAccelerationStructureInstanceKHR. Serialize and upload via the
      // shared abstract-API helper using an upload-heap (CpuToGpu) buffer.
      llvm::SmallVector<D3D12_RAYTRACING_INSTANCE_DESC> Native;
      Native.reserve(TLAS->Instances.size());
      for (const auto &Inst : TLAS->Instances) {
        D3D12_RAYTRACING_INSTANCE_DESC NI = {};
        static_assert(sizeof(NI.Transform) == sizeof(Inst.Transform),
                      "Transform layout mismatch");
        memcpy(&NI.Transform, Inst.Transform, sizeof(Inst.Transform));
        // D3D12_RAYTRACING_INSTANCE_DESC packs InstanceID into a 24-bit
        // bitfield; truncate explicitly so the value matches the VK path
        // (vkInstanceCustomIndex is likewise 24-bit) instead of relying on
        // silent narrowing.
        NI.InstanceID = Inst.InstanceID & 0xFFFFFFu;
        NI.InstanceMask = Inst.InstanceMask;
        NI.InstanceContributionToHitGroupIndex =
            Inst.InstanceContributionToHitGroupIndex & 0xFFFFFFu;
        // Bits in AccelerationStructureInstanceFlags match
        // D3D12_RAYTRACING_INSTANCE_FLAGS by design.
        NI.Flags = static_cast<D3D12_RAYTRACING_INSTANCE_FLAGS>(Inst.Flags);
        auto *BLASPtr = llvm::cast<DXAccelerationStructure>(Inst.BLAS);
        NI.AccelerationStructure = BLASPtr->getGPUVirtualAddress();
        Native.push_back(NI);
      }
      const size_t Bytes =
          Native.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

      const BufferCreateDesc UploadDesc = BufferCreateDesc::uploadBuffer();
      auto InstBufOrErr = offloadtest::createBufferWithData(
          *Dev, "TLAS-Instances", UploadDesc, Native.data(), Bytes, nullptr,
          nullptr);
      if (!InstBufOrErr)
        return InstBufOrErr.takeError();
      auto *DXInstBuf = llvm::cast<DXBuffer>(InstBufOrErr->get());
      Desc.Inputs.InstanceDescs = DXInstBuf->Buffer->GetGPUVirtualAddress();

      CB.KeepAliveOwned.push_back(std::move(*InstBufOrErr));
      ScratchSize = TLAS->AS->getSizes().ScratchDataSizeInBytes;
    }

    const BufferCreateDesc ScratchDesc = BufferCreateDesc::scratchBuffer();
    auto ScratchOrErr =
        Dev->createBuffer("AS-Scratch", ScratchDesc, ScratchSize);
    if (!ScratchOrErr)
      return ScratchOrErr.takeError();
    auto *DXScratchBuf = llvm::cast<DXBuffer>(ScratchOrErr->get());
    Desc.ScratchAccelerationStructureData =
        DXScratchBuf->Buffer->GetGPUVirtualAddress();
    CB.KeepAliveOwned.push_back(std::move(*ScratchOrErr));

    insertDebugSignpost("BuildRaytracingAccelerationStructure");
    CmdList4->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
  }

  // Signal that this batch's AS writes need a barrier before the next reader.
  CB.addPendingUAVBarrier();
  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::dispatchRays(const PipelineState &PSO,
                                           const ShaderBindingTable &SBT,
                                           uint32_t Width, uint32_t Height,
                                           uint32_t Depth) {
  if (!llvm::isa<DXRayTracingPipelineState>(&PSO))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a RayTracing PipelineState.");
  if (!llvm::isa<DXShaderBindingTable>(&SBT))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a DirectX ShaderBindingTable.");
  const auto &DXRTPSO = llvm::cast<DXRayTracingPipelineState>(PSO);
  const auto &DXSBT = llvm::cast<DXShaderBindingTable>(SBT);

  // SetPipelineState1 and DispatchRays live on ID3D12GraphicsCommandList4.
  // The AS-build path (line ~3000 above) follows the same query pattern.
  ComPtr<ID3D12GraphicsCommandList4> CmdList4;
  if (auto Err =
          HR::toError(CB.CmdList.As(&CmdList4),
                      "ID3D12GraphicsCommandList4 query failed; raytracing "
                      "is unsupported on this command list."))
    return Err;

  addUAVBarrier();
  insertDebugSignpost(
      llvm::formatv("DispatchRays [{0},{1},{2}]", Width, Height, Depth).str());

  // Global root signature is shared with the compute bind point; bind it on
  // the underlying command list before SetPipelineState1.
  CB.CmdList->SetComputeRootSignature(DXRTPSO.RootSig.Get());
  CmdList4->SetPipelineState1(DXRTPSO.StateObject.Get());

  D3D12_DISPATCH_RAYS_DESC RaysDesc{};
  RaysDesc.RayGenerationShaderRecord = DXSBT.RayGenRange;
  RaysDesc.MissShaderTable = DXSBT.MissRange;
  RaysDesc.HitGroupTable = DXSBT.HitGroupRange;
  RaysDesc.CallableShaderTable = DXSBT.CallableRange;
  RaysDesc.Width = Width;
  RaysDesc.Height = Height;
  RaysDesc.Depth = Depth;
  CmdList4->DispatchRays(&RaysDesc);
  return llvm::Error::success();
}
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
