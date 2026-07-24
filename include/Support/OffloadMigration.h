//===- Texture.h - Offload API Texture ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Code that is shared between backends, which should eventually live in th
// offloader tool, but cannot at the moment because the graphics backend layer
// is not yet finished.
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_OFFLOAD_MIGRATION_H
#define OFFLOADTEST_OFFLOAD_MIGRATION_H

#include "Support/Pipeline.h"
#include "llvm/ADT/SmallVector.h"
#include <memory>

namespace offloadtest {

class AccelerationStructure;
class Buffer;
class CommandBuffer;
class ComputeEncoder;
class Device;
class MemoryHeap;
class PipelineState;
class RenderPass;
class ShaderBindingTable;
class Texture;

struct ResourceSet {
  std::unique_ptr<MemoryHeap> BackingMemory;
  std::unique_ptr<Buffer> Buffer;
  std::unique_ptr<Texture> Texture;
  std::unique_ptr<offloadtest::Buffer> Readback;
  std::unique_ptr<offloadtest::Buffer> CounterReadback;

  // AS-only; mutually exclusive with the buffer/texture fields above.
  AccelerationStructure *AS = nullptr;

  ResourceSet(std::unique_ptr<offloadtest::Buffer> Buffer,
              std::unique_ptr<MemoryHeap> BackingMemory,
              std::unique_ptr<offloadtest::Buffer> Readback,
              std::unique_ptr<offloadtest::Buffer> CounterReadback)
      : BackingMemory(std::move(BackingMemory)), Buffer(std::move(Buffer)),
        Readback(std::move(Readback)),
        CounterReadback(std::move(CounterReadback)) {}
  ResourceSet(std::unique_ptr<offloadtest::Texture> Texture,
              std::unique_ptr<MemoryHeap> BackingMemory,
              std::unique_ptr<offloadtest::Buffer> Readback)
      : BackingMemory(std::move(BackingMemory)), Texture(std::move(Texture)),
        Readback(std::move(Readback)) {}
  explicit ResourceSet(AccelerationStructure *AS) : AS(AS) {}

  ResourceSet(const ResourceSet &) = delete;
  ResourceSet &operator=(const ResourceSet &) = delete;

  ResourceSet(ResourceSet &&A)
      : BackingMemory(std::move(A.BackingMemory)), Buffer(std::move(A.Buffer)),
        Texture(std::move(A.Texture)), Readback(std::move(A.Readback)),
        CounterReadback(std::move(A.CounterReadback)), AS(A.AS) {}
  ResourceSet &operator=(ResourceSet &&A) {
    BackingMemory = std::move(A.BackingMemory);
    Buffer = std::move(A.Buffer);
    Texture = std::move(A.Texture);
    Readback = std::move(A.Readback);
    CounterReadback = std::move(A.CounterReadback);
    AS = A.AS;
    return *this;
  }
};

// ResourceBundle will contain one ResourceSet for a singular resource
// or multiple ResourceSets for resource array.
using ResourceBundle = llvm::SmallVector<ResourceSet>;
using ResourcePair = std::pair<offloadtest::Resource *, ResourceBundle>;

struct DescriptorTable {
  llvm::SmallVector<ResourcePair> Resources;
};

struct SharedInvocationState {
  std::unique_ptr<CommandBuffer> CB;
  std::unique_ptr<PipelineState> Pipeline;
  // Lifetime-tied to the pipeline; only set for RT pipelines.
  std::unique_ptr<offloadtest::ShaderBindingTable> SBT;

  // Resources for graphics pipelines.
  std::unique_ptr<offloadtest::RenderPass> RenderPass;
  std::unique_ptr<offloadtest::Texture> RenderTarget;
  std::unique_ptr<offloadtest::Buffer> RTReadback;
  std::unique_ptr<offloadtest::Texture> DepthStencil;
  std::unique_ptr<offloadtest::Buffer> VB;

  llvm::SmallVector<std::unique_ptr<Buffer>> KeepAliveBuffers;

  llvm::SmallVector<DescriptorTable> DescTables;
  llvm::SmallVector<ResourcePair> RootResources;

  // Parallel-indexed to `P.AccelStructs.BLAS`.
  llvm::SmallVector<std::unique_ptr<offloadtest::AccelerationStructure>> BLASes;
  // Keyed by `TLASDesc::Name`.
  llvm::StringMap<std::unique_ptr<offloadtest::AccelerationStructure>> TLASes;
  // Vertex/index buffers consumed during AS builds; must outlive submission.
  llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> ASInputBuffers;
};

llvm::Error copyBackResource(offloadtest::ComputeEncoder &ReadbackEncoder,
                             ResourcePair &R);
llvm::Error readBack(Device &Dev, Pipeline &P, SharedInvocationState &IS);
llvm::Error createResources(Device &Dev, Pipeline &P,
                            SharedInvocationState &IS);
llvm::Error createRenderTarget(Device &Dev, Pipeline &P,
                               SharedInvocationState &IS);
llvm::Error createDepthStencil(Device &Dev, Pipeline &P,
                               SharedInvocationState &IS);

} // namespace offloadtest

#endif // OFFLOADTEST_OFFLOAD_MIGRATION_H
