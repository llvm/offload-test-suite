//===- MTLTopLevelArgumentBuffer.h - Metal Top Level Argument Buffer ------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_MTL_MTLTOPLEVELARGUMENTBUFFER_H
#define OFFLOADTEST_API_MTL_MTLTOPLEVELARGUMENTBUFFER_H

#include "MTLDescriptorHeap.h"
#include "Metal/Metal.hpp"
#include "metal_irconverter.h"
#include "llvm/Support/Error.h"
#include <memory>

struct IRDescriptorTableEntry;

namespace offloadtest {
// Manages a Metal buffer that serves as the top-level argument buffer for
// shader resource binding with the explicit root signature layout.
class MTLTopLevelArgumentBuffer {
  std::vector<IRResourceLocation> ResourceLocs;
  MTL::Buffer *Buffer = nullptr;

  bool checkIndex(uint32_t Index) const;
  bool checkResourceType(uint32_t Index, IRResourceType ExpectedType) const;
  bool checkResourceSize(uint32_t Index, size_t ExpectedSize) const;

  template <IRResourceType ResourceType, typename T>
  void setResource(uint32_t Index, T Resource) const;

public:
  /// @brief Creates a MTLTopLevelArgumentBuffer based on the given root
  /// signature. Empty root signature (zero resources) is allowed, bind methods
  /// will be no-op in that case.
  /// @param Device Metal device to create the argument buffer on.
  /// @param RootSig Root signature describing the layout of the argument
  /// buffer.
  /// @return Created MTLTopLevelArgumentBuffer or error if creation failed.
  static llvm::Expected<std::unique_ptr<MTLTopLevelArgumentBuffer>>
  create(MTL::Device *Device, IRRootSignature *RootSig);

  MTLTopLevelArgumentBuffer(std::vector<IRResourceLocation> ResourceLocs,
                            MTL::Buffer *Buffer)
      : ResourceLocs(std::move(ResourceLocs)), Buffer(Buffer) {}
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
                              METAL_GPU_DESCRIPTOR_HANDLE BaseHandle) const;

  // Bind the argument buffer to the render command encoder.
  void bind(MTL::RenderCommandEncoder *Encoder) const;
  // Bind the argument buffer to the compute command encoder.
  void bind(MTL::ComputeCommandEncoder *Encoder) const;
};
} // namespace offloadtest

#endif // OFFLOADTEST_API_MTL_MTLTOPLEVELARGUMENTBUFFER_H
