//===- DX/Queue.h - Offload API DX Queue API ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_QUEUE_H
#define OFFLOADTEST_API_DX_QUEUE_H

#include "API/DX/Common.h"
#include "API/Device.h"

#include <cstdint>
#include <string>

namespace offloadtest {

class DXFence : public offloadtest::Fence {
public:
#ifdef _WIN32
  DXFence(ComPtr<ID3D12Fence> Fence, HANDLE Event, llvm::StringRef Name)
#else // WSL
  DXFence(ComPtr<ID3D12Fence> Fence, int Event, llvm::StringRef Name)
#endif
      : Name(Name), Fence(Fence), Event(Event) {
  }

  ~DXFence() override;

  std::string Name;
  ComPtr<ID3D12Fence> Fence;
#ifdef _WIN32
  HANDLE Event;
#else // WSL
  int Event;
#endif

  static llvm::Expected<std::unique_ptr<DXFence>> create(ID3D12DeviceX *Device,
                                                         llvm::StringRef Name);

  uint64_t getFenceValue() override;

  llvm::Error waitForCompletion(uint64_t SignalValue) override;
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
  ~DXQueue() override;

  static llvm::Expected<DXQueue>
  createGraphicsQueue(ComPtr<ID3D12DeviceX> Device);

  llvm::Expected<offloadtest::SubmitResult>
  submit(llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs)
      override;

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Buffer &Resource,
                     llvm::ArrayRef<TileMapping> Mappings) override;

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Texture &Resource,
                     llvm::ArrayRef<TileMapping> Mappings) override;

  // Shared tile-mapping path for buffers and textures: they differ only in how
  // a TileRegion maps onto the underlying ID3D12Resource, so both variants
  // resolve to a native resource and funnel through here.
  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappingsImpl(ID3D12Resource *Resource,
                         llvm::ArrayRef<TileMapping> Mappings);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_QUEUE_H