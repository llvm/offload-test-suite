//===- DX/Queue.cpp - DirectX Queue API -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/DX/Queue.h"
#include "API/DX/Buffer.h"
#include "API/DX/CommandBuffer.h"
#include "API/DX/MemoryHeap.h"
#include "API/DX/Texture.h"

#include "Support/WinError.h"

using namespace offloadtest;

llvm::Expected<std::unique_ptr<DXFence>> DXFence::create(ID3D12DeviceX *Device,
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

DXFence::~DXFence() {
#ifdef _WIN32
  CloseHandle(Event);
#else // WSL
  close(Event);
#endif
}

uint64_t DXFence::getFenceValue() { return Fence->GetCompletedValue(); }

llvm::Error DXFence::waitForCompletion(uint64_t SignalValue) {
  if (Fence->GetCompletedValue() >= SignalValue)
    return llvm::Error::success();

#ifdef _WIN32
  if (auto Err = HR::toError(Fence->SetEventOnCompletion(SignalValue, Event),
                             "Failed to register end event."))
    return Err;
  WaitForSingleObject(Event, INFINITE);
#else // WSL
  if (auto Err = HR::toError(Fence->SetEventOnCompletion(
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

DXQueue::~DXQueue() {}

llvm::Expected<DXQueue>
DXQueue::createGraphicsQueue(ComPtr<ID3D12DeviceX> Device) {
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

llvm::Expected<offloadtest::SubmitResult>
DXQueue::updateTileMappings(offloadtest::Buffer &Resource,
                            llvm::ArrayRef<TileMapping> Mappings) {
  return updateTileMappingsImpl(llvm::cast<DXBuffer>(Resource).Buffer.Get(),
                                Mappings);
}

llvm::Expected<offloadtest::SubmitResult>
DXQueue::updateTileMappings(offloadtest::Texture &Resource,
                            llvm::ArrayRef<TileMapping> Mappings) {
  return updateTileMappingsImpl(llvm::cast<DXTexture>(Resource).Resource.Get(),
                                Mappings);
}

// Shared tile-mapping path for buffers and textures: they differ only in how
// a TileRegion maps onto the underlying ID3D12Resource, so both variants
// resolve to a native resource and funnel through here.
llvm::Expected<offloadtest::SubmitResult>
DXQueue::updateTileMappingsImpl(ID3D12Resource *Resource,
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
