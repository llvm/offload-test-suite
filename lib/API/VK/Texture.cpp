//===- VK/Texture.cpp - Vulkan Texture API --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/Texture.h"

#include "llvm/ADT/SmallVector.h"

using namespace offloadtest;

VulkanTexture::~VulkanTexture() {
  if (View)
    vkDestroyImageView(Dev, View, nullptr);
  vkDestroyImage(Dev, Image, nullptr);
  vkFreeMemory(Dev, Memory, nullptr);
}

TileShape VulkanTexture::querySparseTileShape(const Device & /*Dev*/) const {
  uint32_t Count = 0;
  vkGetImageSparseMemoryRequirements(Dev, Image, &Count, nullptr);
  if (Count == 0)
    return TileShape{};
  llvm::SmallVector<VkSparseImageMemoryRequirements> Reqs(Count);
  vkGetImageSparseMemoryRequirements(Dev, Image, &Count, Reqs.data());
  const VkExtent3D G = Reqs[0].formatProperties.imageGranularity;
  return TileShape{G.width, G.height, G.depth};
}

const TextureCreateDesc &VulkanTexture::getDesc() const { return Desc; }

bool VulkanTexture::classof(const offloadtest::Texture *T) {
  return T->getAPI() == GPUAPI::Vulkan;
}
