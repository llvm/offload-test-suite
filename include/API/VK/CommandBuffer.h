//===- VK/CommandBuffer.h - Offload API VK CommandBuffer API --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_COMMANDBUFFER_H
#define OFFLOADTEST_API_VK_COMMANDBUFFER_H

#include "API/Buffer.h"
#include "API/CommandBuffer.h"
#include "API/Encoder.h"

#include "llvm/ADT/SmallVector.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace offloadtest {

class VulkanDevice;
class VulkanTexture;

struct MeshShaderFunctions {
  PFN_vkCmdDrawMeshTasksEXT VkCmdDrawMeshTasksEXT = nullptr;

  static MeshShaderFunctions create(VkDevice Device) {
    MeshShaderFunctions Result;
    Result.VkCmdDrawMeshTasksEXT =
        (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(Device,
                                                       "vkCmdDrawMeshTasksEXT");
    return Result;
  }
};

class VulkanCommandBuffer : public offloadtest::CommandBuffer {
public:
  VkDevice Device = VK_NULL_HANDLE;
  MeshShaderFunctions MeshShaderFns;
  // Back-pointer to the owning device. Used by encoders that need access to
  // device-loaded function pointers (e.g. ray-tracing entry points) and
  // helper allocators.
  VulkanDevice *Dev = nullptr;
  // Owned per command buffer so that recording, submission, and lifetime
  // management of each command buffer are independently safe without external
  // synchronization.
  VkCommandPool CmdPool = VK_NULL_HANDLE;
  VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;

  PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabel = nullptr;
  PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabel = nullptr;
  PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabel = nullptr;

  /// Keep-alive list for Framebuffers constructed by RencderEncoders.
  llvm::SmallVector<VkFramebuffer, 4> OwnedFramebuffers;

  static llvm::Expected<std::unique_ptr<VulkanCommandBuffer>>
  create(VkDevice Device, uint32_t QueueFamilyIdx,
         PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabel,
         PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabel,
         PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabel,
         MeshShaderFunctions MeshShaderFns);

  /// Pending pipeline barrier state accumulated by encoders. Lives on the
  /// command buffer because in Vulkan all encoders record into the same
  /// VkCommandBuffer.  Src tracks what prior commands produced; Dst tracks
  /// what the next command will consume.
  VkPipelineStageFlags PendingSrcStage = VK_PIPELINE_STAGE_HOST_BIT;
  VkAccessFlags PendingSrcAccess = VK_ACCESS_HOST_WRITE_BIT;
  VkPipelineStageFlags PendingDstStage = 0;
  VkAccessFlags PendingDstAccess = 0;
  llvm::SmallVector<VkImageMemoryBarrier> PendingImageTransitions;

  void addImageTransition(VkAccessFlags SrcAccessMask,
                          VkAccessFlags DstAccessMask, VkImageLayout OldLayout,
                          VkImageLayout NewLayout, VulkanTexture &Texture);

  void addPendingBarrier(VkPipelineStageFlags Stage, VkAccessFlags Access) {
    PendingDstStage |= Stage;
    PendingDstAccess |= Access;
  }

  void flushBarrier() {
    if (PendingSrcStage != 0 || !PendingImageTransitions.empty()) {
      VkMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
      Barrier.srcAccessMask = PendingSrcAccess;
      Barrier.dstAccessMask = PendingDstAccess;
      vkCmdPipelineBarrier(CmdBuffer, PendingSrcStage, PendingDstStage, 0, 1,
                           &Barrier, 0, nullptr, PendingImageTransitions.size(),
                           PendingImageTransitions.size() == 0
                               ? nullptr
                               : PendingImageTransitions.data());

      PendingImageTransitions.clear();
    }

    PendingSrcStage = PendingDstStage;
    PendingSrcAccess = PendingDstAccess;
    PendingDstStage = 0;
    PendingDstAccess = 0;
  }

  void pushDebugGroup(llvm::StringRef Label) {
    if (!CmdBeginDebugUtilsLabel)
      return;
    VkDebugUtilsLabelEXT LabelInfo = {};
    LabelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    LabelInfo.pLabelName = Label.data();
    CmdBeginDebugUtilsLabel(CmdBuffer, &LabelInfo);
  }

  void popDebugGroup() {
    if (CmdEndDebugUtilsLabel)
      CmdEndDebugUtilsLabel(CmdBuffer);
  }

  void insertDebugSignpost(llvm::StringRef Label) {
    if (!CmdInsertDebugUtilsLabel)
      return;
    VkDebugUtilsLabelEXT LabelInfo = {};
    LabelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    LabelInfo.pLabelName = Label.data();
    CmdInsertDebugUtilsLabel(CmdBuffer, &LabelInfo);
  }

  /// Abstract-API Buffer wrappers (e.g. AS scratch and TLAS instance array
  /// buffers) that must outlive submission. Held as unique_ptr so the wrapper
  /// destructor frees both the VkBuffer and VkDeviceMemory.
  llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> KeepAliveOwned;

  ~VulkanCommandBuffer() override {
    for (VkFramebuffer FB : OwnedFramebuffers)
      vkDestroyFramebuffer(Device, FB, nullptr);
    if (CmdPool != VK_NULL_HANDLE)
      vkDestroyCommandPool(Device, CmdPool, nullptr);
  }

  static bool classof(const CommandBuffer *CB) {
    return CB->getKind() == GPUAPI::Vulkan;
  }

  llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
  createComputeEncoder() override;

  llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
  createRenderEncoder(const offloadtest::RenderPassBeginDesc &Desc) override;

private:
  VulkanCommandBuffer() : CommandBuffer(GPUAPI::Vulkan) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_COMMANDBUFFER_H
