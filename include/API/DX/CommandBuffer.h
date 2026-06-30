//===- DX/CommandBuffer.h - Offload API DX CommandBuffer API --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_COMMANDBUFFER_H
#define OFFLOADTEST_API_DX_COMMANDBUFFER_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/Buffer.h"
#include "API/CommandBuffer.h"
#include "API/DX/Common.h"
#include "API/DX/Encoder.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXDevice;

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
  create(ComPtr<ID3D12DeviceX> Device);

  ~DXCommandBuffer() override = default;

  static bool classof(const CommandBuffer *CB);

  void addPendingUAVBarrier();
  void addResourceTransition(ID3D12Resource *Resource,
                             D3D12_RESOURCE_STATES StateBefore,
                             D3D12_RESOURCE_STATES StateAfter);

  void flushBarrier();

  void bindPool(const DescriptorPool &Pool) override;

  llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
  createComputeEncoder() override;

  llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
  createRenderEncoder(const offloadtest::RenderPassBeginDesc &Desc) override;

private:
  DXCommandBuffer() : CommandBuffer(GPUAPI::DirectX) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_COMMANDBUFFER_H
