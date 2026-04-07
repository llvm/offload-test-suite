//===- MTLDescriptorHeap.h - Metal Descriptor Heap ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_MTL_MTLDESCRIPTORHEAP_H
#define OFFLOADTEST_API_MTL_MTLDESCRIPTORHEAP_H

#include "Metal/Metal.hpp"
#include "llvm/Support/Error.h"
#include <memory>

struct IRDescriptorTableEntry;

namespace offloadtest {
struct MTLGPUDescriptorHandle {
  MTLGPUDescriptorHandle &Offset(int32_t OffsetInDescriptors);

  MTL::GPUAddress Ptr;
};

enum class MTLDescriptorHeapType {
  CBV_SRV_UAV,
  Sampler,
};

struct MTLDescriptorHeapDesc {
  MTLDescriptorHeapType Type;
  uint32_t NumDescriptors;
};

// MTLDescriptorHeap mimics the D3D12 descriptor heap concept, except
// MTLDescriptorHeap is always shader visible and meant to be used
// by the argument buffer for shader resource binding with the explicit root
// signature layout.
class MTLDescriptorHeap {
  MTLDescriptorHeapDesc Desc;
  MTL::Buffer *Buffer;

public:
  static llvm::Expected<std::unique_ptr<MTLDescriptorHeap>>
  create(MTL::Device *Device, const MTLDescriptorHeapDesc &Desc);

  MTLDescriptorHeap(const MTLDescriptorHeapDesc &Desc, MTL::Buffer *Buffer)
      : Desc(Desc), Buffer(Buffer) {}
  ~MTLDescriptorHeap() {
    if (Buffer)
      Buffer->release();
  }

  MTLGPUDescriptorHandle getGPUDescriptorHandleForHeapStart() const;

  IRDescriptorTableEntry *getEntryHandle(uint32_t Index) const;

  void bind(MTL::RenderCommandEncoder *Encoder);
  void bind(MTL::ComputeCommandEncoder *Encoder);
};
} // namespace offloadtest

#endif // OFFLOADTEST_API_MTL_MTLDESCRIPTORHEAP_H
