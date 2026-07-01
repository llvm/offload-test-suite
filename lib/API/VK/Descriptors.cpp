//===- VK/Descriptors.cpp - Vulkan Descriptors API ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/Descriptors.h"
#include "API/VK/AccelerationStructure.h"
#include "API/VK/Buffer.h"
#include "API/VK/Sampler.h"
#include "API/VK/Texture.h"
#include "Support/VkError.h"

#include "llvm/Support/Casting.h"

#include <cassert>

using namespace offloadtest;

VulkanDescriptorPool::~VulkanDescriptorPool() {
  vkDestroyDescriptorPool(Dev, Pool, nullptr);
}

llvm::Expected<std::unique_ptr<DescriptorPool>>
VulkanDescriptorPool::create(VkDevice Dev) {
  constexpr VkDescriptorType DescriptorTypes[] = {
      VK_DESCRIPTOR_TYPE_SAMPLER,
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
      VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
      VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR};

  llvm::SmallVector<VkDescriptorPoolSize> PoolSizes;
  for (const VkDescriptorType Type : DescriptorTypes) {
    VkDescriptorPoolSize PoolSize = {};
    PoolSize.type = Type;
    PoolSize.descriptorCount = 1024; // Just allocate enough
    PoolSizes.push_back(PoolSize);
  }

  VkDescriptorPoolCreateInfo PoolCreateInfo = {};
  PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  PoolCreateInfo.poolSizeCount = PoolSizes.size();
  PoolCreateInfo.pPoolSizes = PoolSizes.data();
  PoolCreateInfo.maxSets = 64; // Just allocate enough

  VkDescriptorPool Pool = VK_NULL_HANDLE;
  if (auto Err = VK::toError(
          vkCreateDescriptorPool(Dev, &PoolCreateInfo, nullptr, &Pool),
          "Failed to create descriptor pool."))
    return Err;

  return std::make_unique<VulkanDescriptorPool>(Dev, Pool);
}

void VulkanDescriptorPool::reset() { vkResetDescriptorPool(Dev, Pool, 0); }

bool VulkanDescriptorPool::classof(const DescriptorPool *P) {
  return P->getAPI() == GPUAPI::Vulkan;
}

DescriptorSetsBuilder &
VulkanDescriptorSetsBuilder::bindBuffers(uint32_t SetIndex,
                                         llvm::ArrayRef<const Buffer *> B,
                                         VKBind Bnd, bool IsRead) {
  const size_t BufferInfosCapacity = BufferInfos.capacity();
  const size_t BufferViewsCapacity = BufferViews.capacity();

  const size_t Offset = BufferInfos.size();
  const size_t OffsetTyped = BufferViews.size();

  uint32_t NumRawBuffers = 0;
  uint32_t NumTypedBuffers = 0;

  for (const Buffer *Buf : B) {
    const VulkanBuffer &BufferVk = llvm::cast<VulkanBuffer>(*Buf);
    if (BufferVk.Desc.AccessType == BufferShaderAccessType::Typed) {
      BufferViews.push_back(BufferVk.View);
      NumTypedBuffers += 1;
    } else {
      const VkDescriptorBufferInfo BI = {BufferVk.Buffer, 0, VK_WHOLE_SIZE};
      BufferInfos.push_back(BI);
      NumRawBuffers += 1;
    }
  }

  assert((NumRawBuffers == 0 || NumTypedBuffers == 0) &&
         "Cannot bind a mix of typed and raw buffers to a single bind point.");

  const size_t CountersOffset = BufferInfos.size();

  uint32_t NumCounters = 0;

  for (const Buffer *Buf : B) {
    const VulkanBuffer &BufferVk = llvm::cast<VulkanBuffer>(*Buf);
    if (BufferVk.Desc.HasCounter) {
      assert(BufferVk.Desc.AccessType != BufferShaderAccessType::Typed);

      const VkDescriptorBufferInfo BI = {BufferVk.CounterBuffer, 0,
                                         VK_WHOLE_SIZE};
      BufferInfos.push_back(BI);
      NumCounters += 1;
    }
  }

  if (NumCounters > 0)
    assert((NumRawBuffers == NumCounters) &&
           "Cannot bind a mix of buffers with and without a counter to the "
           "same bind point.");

  // Builder relies on no reallocations happening
  assert(BufferInfos.size() <= BufferInfosCapacity &&
         "Too many BufferInfos inserted into DescriptorSets");
  assert(BufferViews.size() <= BufferViewsCapacity &&
         "Too many BufferViews inserted into DescriptorSets");

  if (NumRawBuffers > 0) {
    VkWriteDescriptorSet WDS = {};
    WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    WDS.dstSet = Sets[SetIndex];
    WDS.dstBinding = Bnd.Binding;
    WDS.descriptorCount = NumRawBuffers;
    WDS.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    WDS.pBufferInfo = &BufferInfos[Offset];
    WriteDescriptors.push_back(WDS);

    if (NumCounters > 0) {
      VkWriteDescriptorSet WDS = {};
      WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      WDS.dstSet = Sets[SetIndex];
      WDS.dstBinding = Bnd.CounterBinding;
      WDS.descriptorCount = NumCounters;
      WDS.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      WDS.pBufferInfo = &BufferInfos[CountersOffset];
      WriteDescriptors.push_back(WDS);
    }
  } else if (NumTypedBuffers > 0) {
    VkWriteDescriptorSet WDS = {};
    WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    WDS.dstSet = Sets[SetIndex];
    WDS.dstBinding = Bnd.Binding;
    WDS.descriptorCount = NumTypedBuffers;
    WDS.descriptorType = IsRead ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
                                : VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    WDS.pTexelBufferView = &BufferViews[OffsetTyped];
    WriteDescriptors.push_back(WDS);
  }

  return *this;
}

DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::bindTextures(
    uint32_t SetIndex, llvm::ArrayRef<const Texture *> T,
    llvm::ArrayRef<const Sampler *> S, VKBind Bnd, bool IsRead) {
  assert((S.empty() || S.size() == T.size()) &&
         "Sampler list must either be empty or match "
         "texture list when binding descriptors.");

  const size_t ImageInfosCapacity = ImageInfos.capacity();
  const size_t Offset = ImageInfos.size();

  for (size_t I = 0, N = T.size(); I < N; ++I) {
    const VulkanTexture &TextureVk = llvm::cast<VulkanTexture>(*T[I]);
    VkSampler SamplerHandle = VK_NULL_HANDLE;
    if (!S.empty() && S[I] != nullptr) {
      const VulkanSampler &SamplerVk = llvm::cast<VulkanSampler>(*S[I]);
      SamplerHandle = SamplerVk.Sampler;
    }
    const VkDescriptorImageInfo ImageInfo = {SamplerHandle, TextureVk.View,
                                             TextureVk.PreferredLayout};
    ImageInfos.push_back(ImageInfo);
  }

  assert(ImageInfos.size() <= ImageInfosCapacity &&
         "Too many ImageInfos inserted into DescriptorSets");

  VkWriteDescriptorSet WDS = {};
  WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WDS.dstSet = Sets[SetIndex];
  WDS.dstBinding = Bnd.Binding;
  WDS.descriptorCount = T.size();
  WDS.descriptorType = IsRead ? VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
                              : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  WDS.pImageInfo = &ImageInfos[Offset];
  WriteDescriptors.push_back(WDS);

  return *this;
}

DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::constant(
    uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind Bnd) {
  const size_t BufferInfosCapacity = BufferInfos.capacity();
  const size_t Offset = BufferInfos.size();
  for (const Buffer *Buf : B) {
    const VulkanBuffer &BufferVk = llvm::cast<VulkanBuffer>(*Buf);
    const VkDescriptorBufferInfo BI = {BufferVk.Buffer, 0, VK_WHOLE_SIZE};
    BufferInfos.push_back(BI);
  }

  assert(BufferInfos.size() <= BufferInfosCapacity &&
         "Too many BufferInfos inserted into DescriptorSets");

  VkWriteDescriptorSet WDS = {};
  WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WDS.dstSet = Sets[SetIndex];
  WDS.dstBinding = Bnd.Binding;
  WDS.descriptorCount = B.size();
  WDS.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  WDS.pBufferInfo = &BufferInfos[Offset];
  WriteDescriptors.push_back(WDS);

  return *this;
}

DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::read(
    uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind Bnd) {
  return bindBuffers(SetIndex, B, Bnd, /*isRead=*/true);
}
DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::read(
    uint32_t SetIndex, llvm::ArrayRef<const Texture *> T,
    llvm::ArrayRef<const Sampler *> S, VKBind Bnd) {
  return bindTextures(SetIndex, T, S, Bnd, /*IsRead=*/true);
}
DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::read(
    uint32_t SetIndex, llvm::ArrayRef<const AccelerationStructure *> A,
    VKBind Bnd) {
  const size_t ASHandlesCapacity = ASHandles.capacity();
  const size_t ASInfosCapacity = ASInfos.capacity();
  const size_t HandleStart = ASHandles.size();

  for (const AccelerationStructure *AS : A) {
    const VulkanAccelerationStructure &AccelStructVulkan =
        llvm::cast<VulkanAccelerationStructure>(*AS);
    ASHandles.push_back(AccelStructVulkan.AccelStruct);
  }

  VkWriteDescriptorSetAccelerationStructureKHR ASWrite = {};
  ASWrite.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  ASWrite.accelerationStructureCount = A.size();
  ASWrite.pAccelerationStructures = &ASHandles[HandleStart];
  ASInfos.push_back(ASWrite);

  // Builder relies on no reallocations happening
  assert(
      ASHandles.size() <= ASHandlesCapacity &&
      "Too many Acceleration Structure Handles inserted into DescriptorSets");
  assert(ASInfos.size() <= ASInfosCapacity &&
         "Too many Acceleration Structure Writes inserted into DescriptorSets");

  VkWriteDescriptorSet WDS = {};
  WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WDS.pNext = &ASInfos.back();
  WDS.dstSet = Sets[SetIndex];
  WDS.dstBinding = Bnd.Binding;
  WDS.descriptorCount = A.size();
  WDS.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  WriteDescriptors.push_back(WDS);

  return *this;
}

DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::write(
    uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind Bnd) {
  return bindBuffers(SetIndex, B, Bnd, /*isRead=*/false);
}
DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::write(
    uint32_t SetIndex, llvm::ArrayRef<const Texture *> T, VKBind Bnd) {
  return bindTextures(SetIndex, T, {}, Bnd, /*IsRead=*/false);
}

DescriptorSetsBuilder &VulkanDescriptorSetsBuilder::sampler(
    uint32_t SetIndex, llvm::ArrayRef<const Sampler *> S, VKBind Bnd) {
  const size_t ImageInfosCapacity = ImageInfos.capacity();
  const size_t Offset = ImageInfos.size();

  for (const Sampler *Sampl : S) {
    const VulkanSampler &SamplerVk = llvm::cast<VulkanSampler>(*Sampl);
    const VkDescriptorImageInfo ImageInfo = {
        SamplerVk.Sampler, VK_NULL_HANDLE,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    ImageInfos.push_back(ImageInfo);
  }

  assert(ImageInfos.size() <= ImageInfosCapacity &&
         "Too many ImageInfos inserted into DescriptorSets");

  VkWriteDescriptorSet WDS = {};
  WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  WDS.dstSet = Sets[SetIndex];
  WDS.dstBinding = Bnd.Binding;
  WDS.descriptorCount = S.size();
  WDS.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
  WDS.pImageInfo = &ImageInfos[Offset];
  WriteDescriptors.push_back(WDS);

  return *this;
}

std::unique_ptr<DescriptorSets> VulkanDescriptorSetsBuilder::build() {
  vkUpdateDescriptorSets(Dev, WriteDescriptors.size(), WriteDescriptors.data(),
                         0, nullptr);
  return std::make_unique<VulkanDescriptorSets>(std::move(Sets));
}

bool VulkanDescriptorSetsBuilder::classof(const DescriptorSetsBuilder *B) {
  return B->getAPI() == GPUAPI::Vulkan;
}
