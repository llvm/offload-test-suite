//===- VK/Encoder.h - Offload API VK Encoder API --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_ENCODER_H
#define OFFLOADTEST_API_VK_ENCODER_H

#include "API/CommandBuffer.h"
#include "API/Encoder.h"

#include <vulkan/vulkan.h>

namespace offloadtest {

class VulkanCommandBuffer;

class VKComputeEncoder : public offloadtest::ComputeEncoder {
  VulkanCommandBuffer &CB;

  void addDstBarrier(VkPipelineStageFlags DstStage, VkAccessFlags DstAccess);

public:
  VKComputeEncoder(VulkanCommandBuffer &CB)
      : ComputeEncoder(GPUAPI::Vulkan), CB(CB) {}

  ~VKComputeEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::Vulkan;
  }

  void pushDebugGroup(llvm::StringRef Label) override;
  void popDebugGroup() override;
  void insertDebugSignpost(llvm::StringRef Label) override;

  void bindDescriptorSets(const PipelineState &PSO, const DescriptorSets &DSets,
                          VkPipelineBindPoint BindPoint);

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

  llvm::Error copyBufferToTexture(offloadtest::Buffer &Src,
                                  offloadtest::Texture &Dst) override;

  llvm::Error copyCounterToBuffer(offloadtest::Buffer &Src,
                                  offloadtest::Buffer &Dst) override;

  llvm::Error copyTextureToBuffer(offloadtest::Texture &Src,
                                  offloadtest::Buffer &Dst) override;

  // Defined out-of-line below — needs VulkanDevice's full type for access to
  // the device-loaded ray-tracing entry points and helpers.
  llvm::Error batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) override;

  // Defined out-of-line below — needs VulkanDevice for the RT pipeline
  // entry points and VKRayTracingPipelineState's full type.
  llvm::Error dispatchRays(const PipelineState &PSO,
                           const ShaderBindingTable &SBT, uint32_t Width,
                           uint32_t Height, uint32_t Depth) override;

  void endEncodingImpl() override;
};

class VulkanRenderEncoder : public offloadtest::RenderEncoder {
  VulkanCommandBuffer &CB;
  offloadtest::RenderPassBeginDesc Desc;

  // Encoder contract: viewport and scissor must both be set before draw().
  bool ViewportSet = false;
  bool ScissorSet = false;

public:
  VulkanRenderEncoder(VulkanCommandBuffer &CB,
                      const offloadtest::RenderPassBeginDesc &Desc);
  VulkanRenderEncoder(const VulkanRenderEncoder &CB) = delete;
  VulkanRenderEncoder(VulkanRenderEncoder &&CB) = delete;
  VulkanRenderEncoder &operator=(VulkanRenderEncoder &CB) = delete;
  VulkanRenderEncoder &operator=(const VulkanRenderEncoder &&CB) = delete;

  ~VulkanRenderEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::Vulkan;
  }

  void pushDebugGroup(llvm::StringRef Label) override;
  void popDebugGroup() override;
  void insertDebugSignpost(llvm::StringRef Label) override;

  void setViewport(const offloadtest::Viewport &VP) override;

  void setScissor(const offloadtest::ScissorRect &Rect) override;

  void setVertexBuffer(uint32_t Slot, offloadtest::Buffer *VB, size_t Offset,
                       uint32_t /*Stride*/) override;

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

#endif // OFFLOADTEST_API_VK_ENCODER_H
