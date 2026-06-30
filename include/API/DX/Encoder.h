//===- DX/Encoder.h - Offload API DX Encoder API --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_ENCODER_H
#define OFFLOADTEST_API_DX_ENCODER_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/CommandBuffer.h"
#include "API/Encoder.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXCommandBuffer;

class DXComputeEncoder : public offloadtest::ComputeEncoder {
  DXCommandBuffer &CB;

  void addUAVBarrier();

public:
  DXComputeEncoder(DXCommandBuffer &CB)
      : ComputeEncoder(GPUAPI::DirectX), CB(CB) {}

  ~DXComputeEncoder() override;

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::DirectX;
  }

  // D3D12 debug labels require WinPixEventRuntime for the proper event
  // encoding.  Without it, BeginEvent/EndEvent/SetMarker with metadata type 0
  // crash the D3D12 debug layer, so leave these as no-ops for now.
  void pushDebugGroup(llvm::StringRef Label) override {}
  void popDebugGroup() override {}
  void insertDebugSignpost(llvm::StringRef Label) override {}

  void bindDescriptorSets(const PipelineState &PSO,
                          const DescriptorSets &DSets);

  void bindComputeDescriptorSets(const PipelineState &PSO,
                                 const DescriptorSets &DSets) override;

  void bindRayTracingDescriptorSets(const PipelineState &PSO,
                                    const DescriptorSets &DSets) override;

  llvm::Error dispatch(const offloadtest::PipelineState &PSO,
                       uint32_t GroupCountX, uint32_t GroupCountY,
                       uint32_t GroupCountZ) override;

  llvm::Error copyBufferToBuffer(offloadtest::Buffer &Src, size_t SrcOffset,
                                 offloadtest::Buffer &Dst, size_t DstOffset,
                                 size_t Size) override;

  llvm::Error copyBufferToTexture(Buffer &Src, Texture &Dst) override;

  llvm::Error copyCounterToBuffer(offloadtest::Buffer &Src,
                                  offloadtest::Buffer &Dst) override;

  llvm::Error copyTextureToBuffer(Texture &Src, Buffer &Dst) override;

  // Defined out-of-line below — needs DXDevice's full type for access to the
  // ID3D12Device5 entry point and helper allocators.
  llvm::Error batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) override;

  // Defined out-of-line below — needs DXDevice's full type for access to
  // Device5 and the DXRayTracingPipelineState definition.
  llvm::Error dispatchRays(const PipelineState &PSO,
                           const ShaderBindingTable &SBT, uint32_t Width,
                           uint32_t Height, uint32_t Depth) override;

  void endEncodingImpl() override;
};

class DXRenderEncoder : public offloadtest::RenderEncoder {
  DXCommandBuffer &CB;
  offloadtest::RenderPassBeginDesc Desc;

  // Encoder contract: viewport and scissor must both be set before draw().
  bool ViewportSet = false;
  bool ScissorSet = false;

  llvm::Error bindCommonDrawState(const offloadtest::PipelineState &PSO);

public:
  DXRenderEncoder(DXCommandBuffer &CB,
                  const offloadtest::RenderPassBeginDesc &Desc);
  DXRenderEncoder(const DXRenderEncoder &CB) = delete;
  DXRenderEncoder(DXRenderEncoder &&CB) = delete;
  DXRenderEncoder &operator=(DXRenderEncoder &CB) = delete;
  DXRenderEncoder &operator=(const DXRenderEncoder &&CB) = delete;

  ~DXRenderEncoder() override;

  static bool classof(const CommandEncoder *E);

  // See DXComputeEncoder for why these are no-ops.
  void pushDebugGroup(llvm::StringRef Label) override;
  void popDebugGroup() override;
  void insertDebugSignpost(llvm::StringRef Label) override;

  void setViewport(const offloadtest::Viewport &VP) override;

  void setScissor(const offloadtest::ScissorRect &Rect) override;

  void setVertexBuffer(uint32_t Slot, offloadtest::Buffer *VB, size_t Offset,
                       uint32_t Stride) override;

  void bindDescriptorSets(const PipelineState &PSO,
                          const DescriptorSets &DSets) override;

  llvm::Error drawInstanced(const offloadtest::PipelineState &PSO,
                            uint32_t VertexCount, uint32_t InstanceCount,
                            uint32_t FirstVertex,
                            uint32_t FirstInstance) override;
  llvm::Error dispatchMesh(const offloadtest::PipelineState &PSO,
                           uint32_t GroupCountX, uint32_t GroupCountY,
                           uint32_t GroupCountZ) override;
  void endEncodingImpl() override;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_ENCODER_H
