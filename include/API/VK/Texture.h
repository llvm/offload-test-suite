//===- VK/Texture.h - Offload API VK Texture API --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_TEXTURE_H
#define OFFLOADTEST_API_VK_TEXTURE_H

#include "API/Texture.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace offloadtest {

class VulkanTexture : public offloadtest::Texture {
public:
  VkDevice Dev;
  VkImage Image;
  VkDeviceMemory Memory;
  // TODO:
  // RenderTarget and DepthStencil views are created at texture creation time.
  // Ideally Sampled/Storage image views would also live here, but they are
  // currently created during descriptor set setup, which determines their
  // binding layout.
  VkImageView View = VK_NULL_HANDLE;
  std::string Name;
  TextureCreateDesc Desc;
  VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;

  VkImageLayout PreferredLayout = VK_IMAGE_LAYOUT_GENERAL;
  VkImageSubresourceRange FullRange;
  bool IsInUndefinedLayout = true;
  uint64_t SizeInBytes;

  VulkanTexture(VkDevice Dev, VkImage Image, VkDeviceMemory Memory,
                VkImageView View, llvm::StringRef Name, TextureCreateDesc Desc,
                VkImageLayout PreferredLayout,
                VkImageSubresourceRange FullRange, VkImageTiling Tiling,
                uint64_t SizeInBytes)
      : offloadtest::Texture(GPUAPI::Vulkan), Dev(Dev), Image(Image),
        Memory(Memory), View(View), Name(Name), Desc(Desc), Tiling(Tiling),
        PreferredLayout(PreferredLayout), FullRange(FullRange),
        SizeInBytes(SizeInBytes) {}

  ~VulkanTexture() override;

  VkImageLayout preferredLayoutOrUndefined() {
    return IsInUndefinedLayout ? VK_IMAGE_LAYOUT_UNDEFINED : PreferredLayout;
  }

  TileShape querySparseTileShape(const Device & /*Dev*/) const override;

  const TextureCreateDesc &getDesc() const override;

  static bool classof(const offloadtest::Texture *T);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_TEXTURE_H
