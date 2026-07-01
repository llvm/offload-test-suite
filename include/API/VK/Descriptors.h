//===- VK/Descriptors.h - Offload API VK Descriptors API ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_DESCRIPTORS_H
#define OFFLOADTEST_API_VK_DESCRIPTORS_H

#include "API/Descriptors.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>

namespace offloadtest {

struct DescriptorCounts {
  // Counters to determine how many infos and counts to allocate in the builder.
  // Must match for builder to function correctly.
  uint32_t ImageInfoCount = 0;
  uint32_t BufferInfoCount = 0;
  uint32_t BufferViewCount = 0;
  uint32_t ASInfoCount = 0;
  uint32_t ASHandleCount = 0;

  // Size hint for how many write commands to allocate in the builder, doesn't
  // need to be correct, but will prevent (re)allocations if guessed correctly
  uint32_t DescriptorWriteHint = 0;
};

class VulkanDescriptorPool : public DescriptorPool {
public:
  VkDevice Dev;
  VkDescriptorPool Pool;

  VulkanDescriptorPool(VkDevice Dev, VkDescriptorPool Pool)
      : DescriptorPool(GPUAPI::Vulkan), Dev(Dev), Pool(Pool) {}

  ~VulkanDescriptorPool() override;

  static llvm::Expected<std::unique_ptr<DescriptorPool>> create(VkDevice Dev);

  void reset() override;

  static bool classof(const DescriptorPool *P);
};

class VulkanDescriptorSets : public DescriptorSets {
public:
  llvm::SmallVector<VkDescriptorSet> Sets;

  VulkanDescriptorSets(llvm::SmallVector<VkDescriptorSet> Sets)
      : DescriptorSets(GPUAPI::Vulkan), Sets(std::move(Sets)) {}

  static bool classof(const DescriptorSets *S) {
    return S->getAPI() == GPUAPI::Vulkan;
  }
};

class VulkanDescriptorSetsBuilder : public DescriptorSetsBuilder {
public:
  VkDevice Dev;
  llvm::SmallVector<VkDescriptorSet> Sets;

  llvm::SmallVector<VkDescriptorImageInfo> ImageInfos;
  llvm::SmallVector<VkDescriptorBufferInfo> BufferInfos;
  llvm::SmallVector<VkBufferView> BufferViews;
  llvm::SmallVector<VkWriteDescriptorSetAccelerationStructureKHR> ASInfos;
  llvm::SmallVector<VkAccelerationStructureKHR> ASHandles;

  llvm::SmallVector<VkWriteDescriptorSet> WriteDescriptors;

  VulkanDescriptorSetsBuilder(VkDevice Dev,
                              llvm::SmallVector<VkDescriptorSet> Sets,
                              const DescriptorCounts &DescCounts)
      : DescriptorSetsBuilder(GPUAPI::Vulkan), Dev(Dev), Sets(std::move(Sets)) {
    ImageInfos.reserve(DescCounts.ImageInfoCount);
    BufferInfos.reserve(DescCounts.BufferInfoCount);
    BufferViews.reserve(DescCounts.BufferViewCount);
    ASInfos.reserve(DescCounts.ASInfoCount);
    ASHandles.reserve(DescCounts.ASHandleCount);
  }

  DescriptorSetsBuilder &bindBuffers(uint32_t SetIndex,
                                     llvm::ArrayRef<const Buffer *> B,
                                     VKBind Bnd, bool IsRead);

  DescriptorSetsBuilder &bindTextures(uint32_t SetIndex,
                                      llvm::ArrayRef<const Texture *> T,
                                      llvm::ArrayRef<const Sampler *> S,
                                      VKBind Bnd, bool IsRead);

  DescriptorSetsBuilder &constant(uint32_t SetIndex,
                                  llvm::ArrayRef<const Buffer *> B,
                                  VKBind Bnd) override;

  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const Buffer *> B,
                              VKBind Bnd) override;
  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const Texture *> T,
                              llvm::ArrayRef<const Sampler *> S,
                              VKBind Bnd) override;
  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const AccelerationStructure *> A,
                              VKBind Bnd) override;

  DescriptorSetsBuilder &write(uint32_t SetIndex,
                               llvm::ArrayRef<const Buffer *> B,
                               VKBind Bnd) override;
  DescriptorSetsBuilder &write(uint32_t SetIndex,
                               llvm::ArrayRef<const Texture *> T,
                               VKBind Bnd) override;

  DescriptorSetsBuilder &sampler(uint32_t SetIndex,
                                 llvm::ArrayRef<const Sampler *> S,
                                 VKBind Bnd) override;

  std::unique_ptr<DescriptorSets> build() override;

  static bool classof(const DescriptorSetsBuilder *B);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_DESCRIPTORS_H
