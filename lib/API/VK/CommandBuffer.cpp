//===- VK/CommandBuffer.cpp - Vulkan CommandBuffer API --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/CommandBuffer.h"
#include "API/Util.h"
#include "API/VK/Encoder.h"
#include "API/VK/RenderPass.h"
#include "API/VK/Texture.h"
#include "Support/VkError.h"

#include "llvm/Support/Casting.h"

#include <cassert>

using namespace offloadtest;

llvm::Expected<std::unique_ptr<VulkanCommandBuffer>>
VulkanCommandBuffer::create(
    VkDevice Device, uint32_t QueueFamilyIdx,
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabel,
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabel,
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabel,
    MeshShaderFunctions MeshShaderFns) {
  auto CB = std::unique_ptr<VulkanCommandBuffer>(new VulkanCommandBuffer());
  CB->Device = Device;

  VkCommandPoolCreateInfo CmdPoolInfo = {};
  CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  CmdPoolInfo.queueFamilyIndex = QueueFamilyIdx;
  CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (auto Err = VK::toError(
          vkCreateCommandPool(Device, &CmdPoolInfo, nullptr, &CB->CmdPool),
          "Could not create command pool."))
    return Err;

  VkCommandBufferAllocateInfo CBufAllocInfo = {};
  CBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  CBufAllocInfo.commandPool = CB->CmdPool;
  CBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CBufAllocInfo.commandBufferCount = 1;
  if (auto Err = VK::toError(
          vkAllocateCommandBuffers(Device, &CBufAllocInfo, &CB->CmdBuffer),
          "Could not create command buffer."))
    return Err;

  VkCommandBufferBeginInfo BufferInfo = {};
  BufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (auto Err = VK::toError(vkBeginCommandBuffer(CB->CmdBuffer, &BufferInfo),
                             "Could not begin command buffer."))
    return Err;

  CB->CmdBeginDebugUtilsLabel = CmdBeginDebugUtilsLabel;
  CB->CmdEndDebugUtilsLabel = CmdEndDebugUtilsLabel;
  CB->CmdInsertDebugUtilsLabel = CmdInsertDebugUtilsLabel;
  CB->MeshShaderFns = MeshShaderFns;

  return CB;
}

void VulkanCommandBuffer::addImageTransition(VkAccessFlags SrcAccessMask,
                                             VkAccessFlags DstAccessMask,
                                             VkImageLayout OldLayout,
                                             VkImageLayout NewLayout,
                                             VulkanTexture &Texture) {
  assert(
      NewLayout != VK_IMAGE_LAYOUT_UNDEFINED &&
      "There should be no reason to ever transition to an undefined layout.");

  PendingImageTransitions.push_back(VkImageMemoryBarrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, /*sType*/
      nullptr,                                /*pNext*/
      SrcAccessMask,
      DstAccessMask,
      OldLayout,
      NewLayout,
      0, /*srcQueueFamilyIndex*/
      0, /*dstQueueFamilyIndex*/
      Texture.Image,
      Texture.FullRange,
  });

  Texture.IsInUndefinedLayout = false;
}

llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
VulkanCommandBuffer::createComputeEncoder() {
  auto Enc = std::make_unique<VKComputeEncoder>(*this);
  Enc->pushDebugGroup("ComputeEncoder");
  return Enc;
}

llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
VulkanCommandBuffer::createRenderEncoder(
    const offloadtest::RenderPassBeginDesc &Desc) {
  // The pass carries the VkRenderPass and the format / load / store policy.
  // The begin desc supplies the textures that get bound for this encoder.
  if (!Desc.Pass)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc is missing its RenderPass.");
  auto &VKPass = llvm::cast<VulkanRenderPass>(*Desc.Pass);
  const offloadtest::RenderPassDesc &PassDesc = VKPass.Desc;
  if (Desc.ColorAttachments.size() != PassDesc.ColorAttachments.size())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc color attachment count does not match its "
        "RenderPass.");
  if (PassDesc.DepthStencil.has_value() != (Desc.DepthStencil != nullptr))
    return llvm::createStringError(std::errc::invalid_argument,
                                   "RenderPassBeginDesc depth-stencil "
                                   "presence does not match its RenderPass.");

  uint32_t Width = 0, Height = 0;
  if (auto Err = findAndValidateRenderPassTextureSize(Desc, &Width, &Height))
    return Err;

  llvm::SmallVector<VkImageView, 9> Views;
  llvm::SmallVector<VkClearValue, 9> ClearValues;

  for (size_t I = 0; I < Desc.ColorAttachments.size(); ++I) {
    if (!Desc.ColorAttachments[I])
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RenderPassBeginDesc has a null color attachment texture.");
    auto &Tex = llvm::cast<VulkanTexture>(*Desc.ColorAttachments[I]);
    if (Tex.View == VK_NULL_HANDLE)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Color attachment texture has no image view.");
    Views.push_back(Tex.View);

    VkClearValue CV = {};
    if (PassDesc.ColorAttachments[I].Load == offloadtest::LoadAction::Clear) {
      if (!Tex.Desc.OptimizedClearValue)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "LoadAction::Clear requires the render target to have been "
            "created with an OptimizedClearValue.");
      const auto *ColorCV =
          std::get_if<ClearColor>(&*Tex.Desc.OptimizedClearValue);
      assert(ColorCV &&
             "RenderTarget OptimizedClearValue must be a ClearColor");
      CV.color = {{ColorCV->R, ColorCV->G, ColorCV->B, ColorCV->A}};
    }
    ClearValues.push_back(CV);
  }

  if (Desc.DepthStencil) {
    auto &Tex = llvm::cast<VulkanTexture>(*Desc.DepthStencil);
    if (Tex.View == VK_NULL_HANDLE)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Depth-stencil attachment texture has no image view.");
    Views.push_back(Tex.View);

    const auto &DS = *PassDesc.DepthStencil;
    VkClearValue CV = {};
    if (DS.DepthLoad == offloadtest::LoadAction::Clear ||
        DS.StencilLoad == offloadtest::LoadAction::Clear) {
      if (!Tex.Desc.OptimizedClearValue)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "LoadAction::Clear requires the depth-stencil texture to have "
            "been created with an OptimizedClearValue.");
      const auto *DepthCV =
          std::get_if<ClearDepthStencil>(&*Tex.Desc.OptimizedClearValue);
      assert(DepthCV &&
             "DepthStencil OptimizedClearValue must be a ClearDepthStencil");
      CV.depthStencil = {DepthCV->Depth, DepthCV->Stencil};
    }
    ClearValues.push_back(CV);
  }

  for (size_t I = 0; I < Desc.ColorAttachments.size(); ++I) {
    auto &Tex = llvm::cast<VulkanTexture>(*Desc.ColorAttachments[I]);
    if (PassDesc.ColorAttachments[I].Load == LoadAction::Load) {
      this->addImageTransition(
          this->PendingSrcAccess, /*SrcAccessMask*/
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
              VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, /*DstAccessMask*/
          Tex.preferredLayoutOrUndefined(),         /*OldLayout*/
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /*NewLayout*/
          Tex);
    }
  }

  if (Desc.DepthStencil) {
    auto &Tex = llvm::cast<VulkanTexture>(*Desc.DepthStencil);

    if (PassDesc.DepthStencil->DepthLoad == LoadAction::Load ||
        PassDesc.DepthStencil->StencilLoad == LoadAction::Load) {
      this->addImageTransition(
          this->PendingSrcAccess, /*SrcAccessMask*/
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
              VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, /*DstAccessMask*/
          Tex.preferredLayoutOrUndefined(),                 /*OldLayout*/
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /*NewLayout*/
          Tex);
    }
  }

  VkFramebufferCreateInfo FBCI = {};
  FBCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  FBCI.renderPass = VKPass.Handle;
  FBCI.attachmentCount = static_cast<uint32_t>(Views.size());
  FBCI.pAttachments = Views.data();
  FBCI.width = Width;
  FBCI.height = Height;
  FBCI.layers = 1;

  VkFramebuffer Framebuffer = VK_NULL_HANDLE;
  if (auto Err =
          VK::toError(vkCreateFramebuffer(Device, &FBCI, nullptr, &Framebuffer),
                      "Failed to create framebuffer for RenderEncoder."))
    return Err;

  // The framebuffer must outlive this encoder and remain valid through GPU
  // execution; the command buffer destroys it on teardown. The render pass
  // is owned by the user-supplied VulkanRenderPass.
  OwnedFramebuffers.push_back(Framebuffer);

  VkRenderPassBeginInfo BeginInfo = {};
  BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  BeginInfo.renderPass = VKPass.Handle;
  BeginInfo.framebuffer = Framebuffer;
  BeginInfo.renderArea.extent.width = Width;
  BeginInfo.renderArea.extent.height = Height;
  BeginInfo.clearValueCount = static_cast<uint32_t>(ClearValues.size());
  BeginInfo.pClearValues = ClearValues.data();

  this->flushBarrier();

  vkCmdBeginRenderPass(CmdBuffer, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  return std::make_unique<VulkanRenderEncoder>(*this, Desc);
}
