//===- VKResources.h - Vulkan Resource Helpers ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VKRESOURCES_H
#define OFFLOADTEST_API_VKRESOURCES_H

#include "API/Device.h"

#include <vulkan/vulkan.h>

namespace offloadtest {

inline VkMemoryPropertyFlags getVulkanMemoryFlags(MemoryLocation Location) {
  switch (Location) {
  case MemoryLocation::GpuOnly:
    return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  case MemoryLocation::CpuToGpu:
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  case MemoryLocation::GpuToCpu:
    return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
           VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
  }
  llvm_unreachable("All MemoryLocation cases handled");
}

inline VkFormat getVulkanFormat(Format Format) {
  switch (Format) {
  case Format::R16Sint:
    return VK_FORMAT_R16_SINT;
  case Format::R16Uint:
    return VK_FORMAT_R16_UINT;
  case Format::RG16Sint:
    return VK_FORMAT_R16G16_SINT;
  case Format::RG16Uint:
    return VK_FORMAT_R16G16_UINT;
  case Format::RGBA16Sint:
    return VK_FORMAT_R16G16B16A16_SINT;
  case Format::RGBA16Uint:
    return VK_FORMAT_R16G16B16A16_UINT;
  case Format::R32Sint:
    return VK_FORMAT_R32_SINT;
  case Format::R32Uint:
    return VK_FORMAT_R32_UINT;
  case Format::R32Float:
    return VK_FORMAT_R32_SFLOAT;
  case Format::RG32Sint:
    return VK_FORMAT_R32G32_SINT;
  case Format::RG32Uint:
    return VK_FORMAT_R32G32_UINT;
  case Format::RG32Float:
    return VK_FORMAT_R32G32_SFLOAT;
  case Format::RGB32Float:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case Format::RGBA32Sint:
    return VK_FORMAT_R32G32B32A32_SINT;
  case Format::RGBA32Uint:
    return VK_FORMAT_R32G32B32A32_UINT;
  case Format::RGBA32Float:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  case Format::D32Float:
    return VK_FORMAT_D32_SFLOAT;
  case Format::D32FloatS8Uint:
    return VK_FORMAT_D32_SFLOAT_S8_UINT;
  }
  llvm_unreachable("All Format cases handled");
}

inline VkImageUsageFlags getVulkanImageUsage(TextureUsage Usage) {
  VkImageUsageFlags Flags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if ((Usage & Sampled) != 0)
    Flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
  if ((Usage & Storage) != 0)
    Flags |= VK_IMAGE_USAGE_STORAGE_BIT;
  if ((Usage & RenderTarget) != 0)
    Flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  if ((Usage & DepthStencil) != 0)
    Flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  return Flags;
}


} // namespace offloadtest

#endif // OFFLOADTEST_API_VKRESOURCES_H
