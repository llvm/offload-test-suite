//===- DX/Descriptors.h - Offload API DX Descriptors API ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_DESCRIPTORS_H
#define OFFLOADTEST_API_DX_DESCRIPTORS_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/DX/Common.h"
#include "API/Descriptors.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <atomic>

namespace offloadtest {

using Microsoft::WRL::ComPtr;

struct DXDescriptorSet {
  D3D12_CPU_DESCRIPTOR_HANDLE CSUHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE CSUHandleGPU;

  D3D12_CPU_DESCRIPTOR_HANDLE SamplerHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE SamplerHandleGPU;
};

class DXDescriptorSets : public DescriptorSets {
public:
  llvm::SmallVector<DXDescriptorSet> Sets;

  DXDescriptorSets(llvm::SmallVector<DXDescriptorSet> Sets);

  static bool classof(const DescriptorSets *S);
};

class DXDescriptorPool : public DescriptorPool {
public:
  ComPtr<ID3D12DescriptorHeap> CSUHeap;
  ComPtr<ID3D12DescriptorHeap> SamplerHeap;

  uint32_t CSUIncSize;
  uint32_t SamplerIncSize;

  D3D12_CPU_DESCRIPTOR_HANDLE CSUHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE CSUHandleGPU;

  D3D12_CPU_DESCRIPTOR_HANDLE SamplerHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE SamplerHandleGPU;

  std::atomic<uint32_t> CSUAllocator = 0;
  std::atomic<uint32_t> SamplerAllocator = 0;

  DXDescriptorPool(ComPtr<ID3D12DescriptorHeap> CSUHeap,
                   ComPtr<ID3D12DescriptorHeap> SamplerHeap,
                   uint32_t CSUIncSize, uint32_t SamplerIncSize);

  static llvm::Expected<std::unique_ptr<DescriptorPool>>
  create(ComPtr<ID3D12DeviceX> Dev);

  void allocateDescriptors(uint32_t Count, D3D12_CPU_DESCRIPTOR_HANDLE &CPU,
                           D3D12_GPU_DESCRIPTOR_HANDLE &GPU);

  void allocateSamplers(uint32_t Count, D3D12_CPU_DESCRIPTOR_HANDLE &CPU,
                        D3D12_GPU_DESCRIPTOR_HANDLE &GPU);

  void reset() override;

  static bool classof(const DescriptorPool *P);
};

class DXDescriptorSetsBuilder : public DescriptorSetsBuilder {
public:
  struct SetState {
    D3D12_CPU_DESCRIPTOR_HANDLE CSUHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE SamplerHandle;
  };

  ComPtr<ID3D12DeviceX> Dev;
  llvm::SmallVector<DXDescriptorSet> Sets;
  llvm::SmallVector<SetState> SetStates;
  uint32_t CSUIncSize;
  uint32_t SamplerIncSize;

  DXDescriptorSetsBuilder(ComPtr<ID3D12DeviceX> Dev,
                          llvm::SmallVector<DXDescriptorSet> Sets,
                          uint32_t CSUIncSize, uint32_t SamplerIncSize);

  DescriptorSetsBuilder &constant(uint32_t SetIndex,
                                  llvm::ArrayRef<const Buffer *> B,
                                  VKBind) override;

  DescriptorSetsBuilder &
  read(uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind) override;
  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const Texture *> T,
                              llvm::ArrayRef<const Sampler *> S,
                              VKBind) override;
  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const AccelerationStructure *> A,
                              VKBind) override;

  DescriptorSetsBuilder &
  write(uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind) override;
  DescriptorSetsBuilder &
  write(uint32_t SetIndex, llvm::ArrayRef<const Texture *> T, VKBind) override;

  DescriptorSetsBuilder &sampler(uint32_t SetIndex,
                                 llvm::ArrayRef<const Sampler *> S,
                                 VKBind) override;

  std::unique_ptr<DescriptorSets> build() override;

  static bool classof(const DescriptorSetsBuilder *B);
};
} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_DESCRIPTORS_H
