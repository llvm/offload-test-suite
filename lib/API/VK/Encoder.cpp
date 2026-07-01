//===- VK/Encoder.cpp - Vulkan Encoder API --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/Encoder.h"
#include "API/FormatConversion.h"
#include "API/VK/AccelerationStructure.h"
#include "API/VK/Buffer.h"
#include "API/VK/CommandBuffer.h"
#include "API/VK/Descriptors.h"
#include "API/VK/Device.h"
#include "API/VK/PipelineState.h"
#include "API/VK/SBT.h"
#include "API/VK/Texture.h"
#include "VKResources.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/FormatVariadic.h"

#include <algorithm>
#include <cassert>
#include <cstring>

using namespace offloadtest;

void VKComputeEncoder::addDstBarrier(VkPipelineStageFlags DstStage,
                                     VkAccessFlags DstAccess) {
  CB.addPendingBarrier(DstStage, DstAccess);
  CB.flushBarrier();
}

void VKComputeEncoder::pushDebugGroup(llvm::StringRef Label) {
  CB.pushDebugGroup(Label);
}
void VKComputeEncoder::popDebugGroup() { CB.popDebugGroup(); }
void VKComputeEncoder::insertDebugSignpost(llvm::StringRef Label) {
  CB.insertDebugSignpost(Label);
}

void VKComputeEncoder::bindDescriptorSets(const PipelineState &PSO,
                                          const DescriptorSets &DSets,
                                          VkPipelineBindPoint BindPoint) {
  const VulkanPipelineState &VulkanPipeline =
      llvm::cast<VulkanPipelineState>(PSO);
  const VulkanDescriptorSets &VulkanDSets =
      llvm::cast<VulkanDescriptorSets>(DSets);

  if (!VulkanDSets.Sets.empty())
    vkCmdBindDescriptorSets(CB.CmdBuffer, BindPoint, VulkanPipeline.Layout, 0,
                            VulkanDSets.Sets.size(), VulkanDSets.Sets.data(), 0,
                            0);
}

void VKComputeEncoder::bindComputeDescriptorSets(const PipelineState &PSO,
                                                 const DescriptorSets &DSets) {
  return bindDescriptorSets(PSO, DSets, VK_PIPELINE_BIND_POINT_COMPUTE);
}

void VKComputeEncoder::bindRayTracingDescriptorSets(
    const PipelineState &PSO, const DescriptorSets &DSets) {
  return bindDescriptorSets(PSO, DSets, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR);
}

llvm::Error VKComputeEncoder::dispatch(const offloadtest::PipelineState &PSO,
                                       uint32_t GroupCountX,
                                       uint32_t GroupCountY,
                                       uint32_t GroupCountZ) {
  const auto &VKPSO = llvm::cast<VulkanPipelineState>(PSO);
  // Include AS_READ so dispatches that issue RayQuery against a TLAS get a
  // proper hand-off from the prior AS-build's AS_WRITE.
  addDstBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT |
                    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);
  insertDebugSignpost(llvm::formatv("Dispatch [{0},{1},{2}]", GroupCountX,
                                    GroupCountY, GroupCountZ)
                          .str());
  vkCmdBindPipeline(CB.CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    VKPSO.Pipeline);
  vkCmdDispatch(CB.CmdBuffer, GroupCountX, GroupCountY, GroupCountZ);
  return llvm::Error::success();
}

llvm::Error VKComputeEncoder::copyBufferToBuffer(offloadtest::Buffer &Src,
                                                 size_t SrcOffset,
                                                 offloadtest::Buffer &Dst,
                                                 size_t DstOffset,
                                                 size_t Size) {
  auto &VKSrc = llvm::cast<VulkanBuffer>(Src);
  auto &VKDst = llvm::cast<VulkanBuffer>(Dst);
  VkBufferCopy Region = {};
  Region.srcOffset = SrcOffset;
  Region.dstOffset = DstOffset;
  Region.size = Size;
  addDstBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  insertDebugSignpost(llvm::formatv("CopyBuffer {0}B", Size).str());
  vkCmdCopyBuffer(CB.CmdBuffer, VKSrc.Buffer, VKDst.Buffer, 1, &Region);
  return llvm::Error::success();
}

llvm::Error VKComputeEncoder::copyBufferToTexture(offloadtest::Buffer &Src,
                                                  offloadtest::Texture &Dst) {
  auto &VKSrc = llvm::cast<VulkanBuffer>(Src);
  auto &VKDst = llvm::cast<VulkanTexture>(Dst);

  CB.addImageTransition(CB.PendingSrcAccess,                  /*SrcAccessMask*/
                        VK_ACCESS_TRANSFER_WRITE_BIT,         /*DstAccessMask*/
                        VKDst.preferredLayoutOrUndefined(),   /*OldLayout*/
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, /*NewLayout*/
                        VKDst);
  VKDst.IsInUndefinedLayout = false;

  CB.addPendingBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_ACCESS_TRANSFER_READ_BIT |
                           VK_ACCESS_TRANSFER_WRITE_BIT);
  CB.flushBarrier();

  const VkImageAspectFlags AspectMask = isDepthFormat(VKDst.Desc.Fmt)
                                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                                            : VK_IMAGE_ASPECT_COLOR_BIT;
  const uint32_t ElementSize = getFormatSizeInBytes(VKDst.Desc.Fmt);
  llvm::SmallVector<VkBufferImageCopy> Regions;
  uint64_t CurrentOffset = 0;
  for (uint32_t I = 0; I < VKDst.Desc.MipLevels; ++I) {
    const uint32_t MipWidth = std::max(1u, VKDst.Desc.Width >> I);
    const uint32_t MipHeight = std::max(1u, VKDst.Desc.Height >> I);
    VkBufferImageCopy Region = {};
    Region.bufferOffset = CurrentOffset;
    Region.imageSubresource.aspectMask = AspectMask;
    Region.imageSubresource.mipLevel = I;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = 1;
    Region.imageExtent = {MipWidth, MipHeight, 1};
    Regions.push_back(Region);
    CurrentOffset += uint64_t(MipWidth) * MipHeight * ElementSize;
  }

  insertDebugSignpost(
      llvm::formatv("copyBufferToTexture {0} -> {1}", VKSrc.Name, VKDst.Name)
          .str());
  vkCmdCopyBufferToImage(CB.CmdBuffer, VKSrc.Buffer, VKDst.Image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Regions.size(),
                         Regions.data());

  CB.addImageTransition(VK_ACCESS_TRANSFER_WRITE_BIT,         /*SrcAccessMask*/
                        VK_ACCESS_NONE,                       /*DstAccessMask*/
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, /*OldLayout*/
                        VKDst.preferredLayoutOrUndefined(),   /*NewLayout*/
                        VKDst);

  return llvm::Error::success();
}

llvm::Error VKComputeEncoder::copyCounterToBuffer(offloadtest::Buffer &Src,
                                                  offloadtest::Buffer &Dst) {
  auto &VKSrc = llvm::cast<VulkanBuffer>(Src);
  auto &VKDst = llvm::cast<VulkanBuffer>(Dst);

  if (!VKSrc.Desc.HasCounter)
    return llvm::createStringError(
        "Counter resource passed does not hvae a counter.");

  const VkBufferCopy Region{
      0,               /*srcOffset*/
      0,               /*dstOffset*/
      sizeof(uint32_t) /*size*/
  };
  addDstBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT);
  insertDebugSignpost("copyCounterToBuffer 4B");
  assert(VKSrc.CounterBuffer != nullptr && "Counter buffer was nullptr >:(");
  assert(VKDst.Buffer != nullptr && "Dst buffer was nullptr >:(");
  vkCmdCopyBuffer(CB.CmdBuffer, VKSrc.CounterBuffer, VKDst.Buffer, 1, &Region);
  return llvm::Error::success();
}

llvm::Error VKComputeEncoder::copyTextureToBuffer(offloadtest::Texture &Src,
                                                  offloadtest::Buffer &Dst) {
  auto &VKSrc = llvm::cast<VulkanTexture>(Src);
  auto &VKDst = llvm::cast<VulkanBuffer>(Dst);

  CB.addImageTransition(CB.PendingSrcAccess,                  /*SrcAccessMask*/
                        VK_ACCESS_TRANSFER_READ_BIT,          /*DstAccessMask*/
                        VKSrc.preferredLayoutOrUndefined(),   /*OldLayout*/
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, /*NewLayout*/
                        VKSrc);
  VKSrc.IsInUndefinedLayout = false;

  CB.addPendingBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_ACCESS_TRANSFER_READ_BIT |
                           VK_ACCESS_TRANSFER_WRITE_BIT);
  CB.flushBarrier();

  insertDebugSignpost(
      llvm::formatv("copyTextureToBuffer {0} -> {1}", VKSrc.Name, VKDst.Name)
          .str());

  VkBufferImageCopy Region = {};
  Region.imageSubresource.aspectMask =
      VKSrc.FullRange.aspectMask &
      ~VK_IMAGE_ASPECT_STENCIL_BIT; // color or depth
  Region.imageSubresource.mipLevel = 0;
  Region.imageSubresource.baseArrayLayer = 0;
  Region.imageSubresource.layerCount = 1;
  Region.imageExtent.width = VKSrc.Desc.Width;
  Region.imageExtent.height = VKSrc.Desc.Height;
  Region.imageExtent.depth = 1;
  vkCmdCopyImageToBuffer(CB.CmdBuffer, VKSrc.Image,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VKDst.Buffer, 1,
                         &Region);

  CB.addImageTransition(VK_ACCESS_TRANSFER_READ_BIT,          /*SrcAccessMask*/
                        VK_ACCESS_NONE,                       /*DstAccessMask*/
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, /*OldLayout*/
                        VKSrc.preferredLayoutOrUndefined(),   /*NewLayout*/
                        VKSrc);

  return llvm::Error::success();
}

void VKComputeEncoder::endEncodingImpl() { popDebugGroup(); }

VulkanRenderEncoder::VulkanRenderEncoder(
    VulkanCommandBuffer &CB, const offloadtest::RenderPassBeginDesc &Desc)
    : RenderEncoder(GPUAPI::Vulkan), CB(CB), Desc(Desc) {
  pushDebugGroup("RenderEncoder");
}

void VulkanRenderEncoder::pushDebugGroup(llvm::StringRef Label) {
  CB.pushDebugGroup(Label);
}
void VulkanRenderEncoder::popDebugGroup() { CB.popDebugGroup(); }
void VulkanRenderEncoder::insertDebugSignpost(llvm::StringRef Label) {
  CB.insertDebugSignpost(Label);
}

void VulkanRenderEncoder::setViewport(const offloadtest::Viewport &VP) {
  // Negative viewport height (with Y origin at the bottom) flips clip->
  // framebuffer Y the same way DX12 and Metal do, so a CCW-in-clip-space
  // triangle is front-facing on every backend.
  VkViewport VKVP = {};
  VKVP.x = VP.X;
  VKVP.y = VP.Y + VP.Height;
  VKVP.width = VP.Width;
  VKVP.height = -VP.Height;
  VKVP.minDepth = VP.MinDepth;
  VKVP.maxDepth = VP.MaxDepth;
  vkCmdSetViewport(CB.CmdBuffer, 0, 1, &VKVP);
  ViewportSet = true;
}

void VulkanRenderEncoder::setScissor(const offloadtest::ScissorRect &Rect) {
  VkRect2D VKRect = {};
  VKRect.offset.x = Rect.X;
  VKRect.offset.y = Rect.Y;
  VKRect.extent.width = Rect.Width;
  VKRect.extent.height = Rect.Height;
  vkCmdSetScissor(CB.CmdBuffer, 0, 1, &VKRect);
  ScissorSet = true;
}

void VulkanRenderEncoder::setVertexBuffer(uint32_t Slot,
                                          offloadtest::Buffer *VB,
                                          size_t Offset, uint32_t /*Stride*/) {
  // Stride is needed in DX12 at binding time, ignore parameter here.
  assert(Slot == 0 && "Pipeline vertex input only describes binding 0");
  if (VB) {
    VkBuffer Handle = llvm::cast<VulkanBuffer>(*VB).Buffer;
    const VkDeviceSize VKOffset = Offset;
    vkCmdBindVertexBuffers(CB.CmdBuffer, Slot, 1, &Handle, &VKOffset);
  } else {
    VkBuffer NullBuf = VK_NULL_HANDLE;
    const VkDeviceSize Zero = 0;
    vkCmdBindVertexBuffers(CB.CmdBuffer, Slot, 1, &NullBuf, &Zero);
  }
}
void VulkanRenderEncoder::bindDescriptorSets(const PipelineState &PSO,
                                             const DescriptorSets &DSets) {
  const VulkanPipelineState &VulkanPipeline =
      llvm::cast<VulkanPipelineState>(PSO);
  const VulkanDescriptorSets &VulkanDSets =
      llvm::cast<VulkanDescriptorSets>(DSets);

  if (!VulkanDSets.Sets.empty())
    vkCmdBindDescriptorSets(CB.CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            VulkanPipeline.Layout, 0, VulkanDSets.Sets.size(),
                            VulkanDSets.Sets.data(), 0, 0);
}

llvm::Error VulkanRenderEncoder::drawInstanced(
    const offloadtest::PipelineState &PSO, uint32_t VertexCount,
    uint32_t InstanceCount, uint32_t FirstVertex, uint32_t FirstInstance) {
  if (!ViewportSet)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Viewport must be set before drawing.");
  if (!ScissorSet)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Scissor must be set before drawing.");

  const auto &VKPSO = llvm::cast<VulkanPipelineState>(PSO);
  vkCmdBindPipeline(CB.CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    VKPSO.Pipeline);
  vkCmdDraw(CB.CmdBuffer, VertexCount, InstanceCount, FirstVertex,
            FirstInstance);
  return llvm::Error::success();
}

llvm::Error
VulkanRenderEncoder::dispatchMesh(const offloadtest::PipelineState &PSO,
                                  uint32_t GroupCountX, uint32_t GroupCountY,
                                  uint32_t GroupCountZ) {
  if (!ViewportSet)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Viewport must be set before drawing.");
  if (!ScissorSet)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Scissor must be set before drawing.");

  const auto &VKPSO = llvm::cast<VulkanPipelineState>(PSO);
  vkCmdBindPipeline(CB.CmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    VKPSO.Pipeline);
  CB.MeshShaderFns.VkCmdDrawMeshTasksEXT(CB.CmdBuffer, GroupCountX, GroupCountY,
                                         GroupCountZ);
  return llvm::Error::success();
}

void VulkanRenderEncoder::endEncodingImpl() {
  vkCmdEndRenderPass(CB.CmdBuffer);

  for (size_t I = 0; I < Desc.ColorAttachments.size(); ++I) {
    auto &Tex = llvm::cast<VulkanTexture>(*Desc.ColorAttachments[I]);
    CB.addImageTransition(
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        /*SrcAccessMask*/
        CB.PendingSrcAccess,                      /*DstAccessMask*/
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, /*OldLayout*/
        Tex.PreferredLayout,                      /*NewLayout*/
        Tex);
  }

  if (Desc.DepthStencil) {
    auto &Tex = llvm::cast<VulkanTexture>(*Desc.DepthStencil);
    CB.addImageTransition(
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, /*SrcAccessMask*/
        CB.PendingSrcAccess,                              /*DstAccessMask*/
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /*OldLayout*/
        Tex.PreferredLayout,                              /*NewLayout*/
        Tex);
  }

  popDebugGroup();
}

llvm::Error VKComputeEncoder::batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) {
  if (Items.empty())
    return llvm::Error::success();
  if (!CB.Dev || !CB.Dev->AS.CmdBuild)
    return llvm::createStringError(
        std::errc::not_supported,
        "Ray tracing not supported on this command buffer's device.");
  VulkanDevice *Dev = CB.Dev;

  // Pre-call barrier: ensure prior writes complete before AS-build reads
  // (vertex/index/instance input buffers and, for TLAS, sibling BLASes built
  // in a previous batchBuildAS() call). Including the READ bit is what makes a
  // back-to-back BLAS-then-TLAS sequence safe: the second call's barrier
  // flushes AS-build-write from the first into AS-build-read.
  addDstBarrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
                    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

  const size_t N = Items.size();
  // Per-item arrays must outlive the AS.CmdBuild() call.
  llvm::SmallVector<llvm::SmallVector<VkAccelerationStructureGeometryKHR>>
      Geoms(N);
  llvm::SmallVector<llvm::SmallVector<VkAccelerationStructureBuildRangeInfoKHR>>
      Ranges(N);
  llvm::SmallVector<VkAccelerationStructureBuildGeometryInfoKHR> BuildInfos(N);
  llvm::SmallVector<const VkAccelerationStructureBuildRangeInfoKHR *> RangePtrs(
      N);

  for (size_t I = 0; I < N; ++I) {
    const auto &Item = Items[I];

    auto &BI = BuildInfos[I];
    BI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    BI.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

    uint64_t ScratchSize = 0;
    if (const auto *BLAS = llvm::dyn_cast<const BLASBuildRequest *>(Item)) {
      auto *VkAS = llvm::cast<VulkanAccelerationStructure>(BLAS->AS);
      BI.dstAccelerationStructure = VkAS->AccelStruct;
      BI.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

      if (const auto *Tris =
              std::get_if<llvm::SmallVector<TriangleGeometryDesc>>(
                  &BLAS->Geometry)) {
        Geoms[I].reserve(Tris->size());
        Ranges[I].reserve(Tris->size());
        for (const auto &T : *Tris) {
          VkAccelerationStructureGeometryKHR G = {};
          G.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
          G.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
          if (T.Opaque)
            G.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
          auto &Tri = G.geometry.triangles;
          Tri.sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
          auto *VB = llvm::cast<VulkanBuffer>(T.VertexBuffer);
          Tri.vertexFormat = getVulkanFormat(T.VertexFormat);
          Tri.vertexData.deviceAddress =
              VB->getDeviceAddress() + T.VertexBufferOffset;
          Tri.vertexStride = T.VertexStride;
          Tri.maxVertex = T.VertexCount - 1;
          if (T.IndexBuffer) {
            auto *IB = llvm::cast<VulkanBuffer>(T.IndexBuffer);
            Tri.indexType = getVulkanIndexType(T.IdxFormat);
            Tri.indexData.deviceAddress =
                IB->getDeviceAddress() + T.IndexBufferOffset;
          } else {
            Tri.indexType = VK_INDEX_TYPE_NONE_KHR;
          }
          Geoms[I].push_back(G);

          VkAccelerationStructureBuildRangeInfoKHR R = {};
          R.primitiveCount =
              T.IndexBuffer ? T.IndexCount / 3 : T.VertexCount / 3;
          Ranges[I].push_back(R);
        }
      } else {
        const auto &AABBs =
            std::get<llvm::SmallVector<AABBGeometryDesc>>(BLAS->Geometry);
        Geoms[I].reserve(AABBs.size());
        Ranges[I].reserve(AABBs.size());
        for (const auto &A : AABBs) {
          VkAccelerationStructureGeometryKHR G = {};
          G.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
          G.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
          if (A.Opaque)
            G.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
          auto &Ab = G.geometry.aabbs;
          Ab.sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
          Ab.stride = A.AABBStride;
          auto *AB = llvm::cast<VulkanBuffer>(A.AABBBuffer);
          Ab.data.deviceAddress = AB->getDeviceAddress() + A.AABBBufferOffset;
          Geoms[I].push_back(G);

          VkAccelerationStructureBuildRangeInfoKHR R = {};
          R.primitiveCount = A.AABBCount;
          Ranges[I].push_back(R);
        }
      }

      BI.geometryCount = Geoms[I].size();
      BI.pGeometries = Geoms[I].data();
      ScratchSize = BLAS->AS->getSizes().ScratchDataSizeInBytes;
    } else {
      const auto *TLAS = llvm::cast<const TLASBuildRequest *>(Item);
      auto *VkAS = llvm::cast<VulkanAccelerationStructure>(TLAS->AS);
      BI.dstAccelerationStructure = VkAS->AccelStruct;
      BI.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

      // Serialize instances into the Vulkan-native struct.
      llvm::SmallVector<VkAccelerationStructureInstanceKHR> Native;
      Native.reserve(TLAS->Instances.size());
      for (const auto &Inst : TLAS->Instances) {
        VkAccelerationStructureInstanceKHR NI = {};
        static_assert(sizeof(NI.transform.matrix) == sizeof(Inst.Transform),
                      "Transform layout mismatch");
        memcpy(&NI.transform.matrix, Inst.Transform, sizeof(Inst.Transform));
        NI.instanceCustomIndex = Inst.InstanceID & 0xFFFFFFu;
        NI.mask = Inst.InstanceMask;
        NI.instanceShaderBindingTableRecordOffset =
            Inst.InstanceContributionToHitGroupIndex & 0xFFFFFFu;
        // Bits in AccelerationStructureInstanceFlags match
        // VkGeometryInstanceFlagBitsKHR by design.
        NI.flags = static_cast<VkGeometryInstanceFlagsKHR>(Inst.Flags);
        auto *BLASPtr = llvm::cast<VulkanAccelerationStructure>(Inst.BLAS);
        NI.accelerationStructureReference = BLASPtr->getDeviceAddress();
        Native.push_back(NI);
      }
      const size_t Bytes =
          Native.size() * sizeof(VkAccelerationStructureInstanceKHR);

      // Upload the instance array. Storage + CpuToGpu now carries device
      // address and AS-build-input flags (because RT is supported).
      const BufferCreateDesc Desc = BufferCreateDesc::uploadBuffer();
      auto InstBufOrErr = offloadtest::createBufferWithData(
          *Dev, "TLAS-Instances", Desc, Native.data(), Bytes, nullptr, nullptr);
      if (!InstBufOrErr)
        return InstBufOrErr.takeError();
      auto *VkInstBuf = llvm::cast<VulkanBuffer>(InstBufOrErr->get());

      VkAccelerationStructureGeometryKHR G = {};
      G.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      G.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
      auto &Inst = G.geometry.instances;
      Inst.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      Inst.arrayOfPointers = VK_FALSE;
      Inst.data.deviceAddress = VkInstBuf->getDeviceAddress();
      Geoms[I].push_back(G);

      VkAccelerationStructureBuildRangeInfoKHR R = {};
      R.primitiveCount = static_cast<uint32_t>(TLAS->Instances.size());
      Ranges[I].push_back(R);

      BI.geometryCount = 1;
      BI.pGeometries = Geoms[I].data();
      ScratchSize = TLAS->AS->getSizes().ScratchDataSizeInBytes;

      // Keep the instance buffer alive across submission.
      CB.KeepAliveOwned.push_back(std::move(*InstBufOrErr));
    }

    const BufferCreateDesc ScratchDesc = BufferCreateDesc::scratchBuffer();
    auto ScratchOrErr =
        Dev->createBuffer("AS-Scratch", ScratchDesc, ScratchSize);
    if (!ScratchOrErr)
      return ScratchOrErr.takeError();
    auto *VkScratchBuf = llvm::cast<VulkanBuffer>(ScratchOrErr->get());
    BI.scratchData.deviceAddress = VkScratchBuf->getDeviceAddress();
    CB.KeepAliveOwned.push_back(std::move(*ScratchOrErr));

    RangePtrs[I] = Ranges[I].data();
  }

  insertDebugSignpost(
      llvm::formatv("BuildAccelerationStructures x{0}", N).str());
  Dev->AS.CmdBuild(CB.CmdBuffer, static_cast<uint32_t>(N), BuildInfos.data(),
                   RangePtrs.data());
  return llvm::Error::success();
}

llvm::Error VKComputeEncoder::dispatchRays(const PipelineState &PSO,
                                           const ShaderBindingTable &SBT,
                                           uint32_t Width, uint32_t Height,
                                           uint32_t Depth) {
  if (!CB.Dev || !CB.Dev->RT.CmdTraceRays)
    return llvm::createStringError(
        std::errc::not_supported,
        "vkCmdTraceRaysKHR entry point not loaded on this device.");
  if (!llvm::isa<VKRayTracingPipelineState>(&PSO))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a RayTracing PipelineState.");
  if (!llvm::isa<VKShaderBindingTable>(&SBT))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a Vulkan ShaderBindingTable.");
  const auto &VKPSO = llvm::cast<VKRayTracingPipelineState>(PSO);
  const auto &VKSBT = llvm::cast<VKShaderBindingTable>(SBT);

  // Outgoing access from prior pipeline stages must complete before the RT
  // pipeline reads the AS, SBT, and bound resources.
  addDstBarrier(VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |
                    VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

  insertDebugSignpost(
      llvm::formatv("TraceRays {0}x{1}x{2}", Width, Height, Depth).str());
  vkCmdBindPipeline(CB.CmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    VKPSO.Pipeline);
  CB.Dev->RT.CmdTraceRays(CB.CmdBuffer, &VKSBT.RaygenRegion, &VKSBT.MissRegion,
                          &VKSBT.HitRegion, &VKSBT.CallableRegion, Width,
                          Height, Depth);
  return llvm::Error::success();
}
