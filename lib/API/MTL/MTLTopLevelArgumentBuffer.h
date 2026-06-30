//===- MTLTopLevelArgumentBuffer.h - Metal Top Level Argument Buffer ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_MTL_MTLTOPLEVELARGUMENTBUFFER_H
#define OFFLOADTEST_API_MTL_MTLTOPLEVELARGUMENTBUFFER_H

#include "MTLDescriptorHeap.h"
#include "MetalIRConverter.h"
#include "llvm/ADT/SmallVector.h"

namespace offloadtest {

struct MetalResidencyTracker;
// Manages a Metal buffer that serves as the top-level argument buffer for
// shader resource binding with the explicit root signature layout.
class MTLTopLevelArgumentBuffer {
public:
  llvm::SmallVector<IRResourceLocation> ResourceLocs;
  MTL::Buffer *Buffer = nullptr;
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;

  bool checkIndex(uint32_t Index) const;
  bool checkResourceType(uint32_t Index, IRResourceType ExpectedType) const;
  bool checkResourceSize(uint32_t Index, size_t ExpectedSize) const;

  template <IRResourceType ResourceType, typename T>
  void setResource(uint32_t Index, T Resource) const;

  /// @brief Creates a MTLTopLevelArgumentBuffer based on the given root
  /// signature. Empty root signature (zero resources) is allowed, bind methods
  /// will be no-op in that case.
  /// @param Device Metal device to create the argument buffer on.
  /// @param RootSig Root signature describing the layout of the argument
  /// buffer.
  /// @return Created MTLTopLevelArgumentBuffer or error if creation failed.
  static llvm::Expected<std::unique_ptr<MTLTopLevelArgumentBuffer>>
  create(MTL::Device *Device, IRRootSignature *RootSig,
         std::shared_ptr<MetalResidencyTracker> ResidencyTracker);

  MTLTopLevelArgumentBuffer(
      llvm::SmallVector<IRResourceLocation> ResourceLocs, MTL::Buffer *Buffer,
      std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : ResourceLocs(std::move(ResourceLocs)), Buffer(Buffer),
        ResidencyTracker(std::move(ResidencyTracker)) {}
  ~MTLTopLevelArgumentBuffer();

  // Binds 32-bit root constant(s) to the argument buffer.
  void setRoot32BitConstant(uint32_t Index, uint32_t SrcData,
                            uint32_t DestOffsetIn32BitValues) const;
  void setRoot32BitConstants(uint32_t Index, uint32_t Num32BitValuesToSet,
                             const void *pSrcData,
                             uint32_t DestOffsetIn32BitValues) const;

  // Binds CBV/SRV/UAV resource via GPU address or resource ID to the argument
  // buffer.
  void setRootConstantBufferView(uint32_t Index, uint64_t GPUAddr) const;
  void setRootShaderResourceView(uint32_t Index, uint64_t GPUAddr) const;
  void setRootUnorderedAccessView(uint32_t Index, uint64_t GPUAddr) const;

  // Binds descriptor table to the argument buffer.
  void setRootDescriptorTable(uint32_t Index,
                              MTLGPUDescriptorHandle BaseHandle) const;

  // Bind the argument buffer to the render command encoder.
  void bind(MTL::RenderCommandEncoder *Encoder) const;
  // Bind the argument buffer to the compute command encoder.
  void bind(MTL::ComputeCommandEncoder *Encoder) const;

  // GPU address of the underlying buffer. Returns 0 when the root signature is
  // empty and no buffer was allocated. Needed by the ray-tracing path so the
  // synthesized IRDispatchRaysArgument can point shader-record callees back at
  // the GRS (global root signature top-level argument buffer).
  uint64_t getGPUAddress() const;
};
} // namespace offloadtest

#endif // OFFLOADTEST_API_MTL_MTLTOPLEVELARGUMENTBUFFER_H
