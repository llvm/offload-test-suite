//===- VK/RenderPass.h - Offload API VK Render Pass API -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_RENDERPASS_H
#define OFFLOADTEST_API_VK_RENDERPASS_H

#include "API/RenderPass.h"

#include <vulkan/vulkan.h>

namespace offloadtest {

class VulkanRenderPass final : public offloadtest::RenderPass {
public:
  VkDevice Dev;
  VkRenderPass Handle;
  offloadtest::RenderPassDesc Desc;

  VulkanRenderPass(VkDevice Dev, VkRenderPass Handle,
                   offloadtest::RenderPassDesc Desc)
      : RenderPass(GPUAPI::Vulkan), Dev(Dev), Handle(Handle),
        Desc(std::move(Desc)) {}

  ~VulkanRenderPass() override;

  static bool classof(const offloadtest::RenderPass *RP) {
    return RP->getAPI() == GPUAPI::Vulkan;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_RENDERPASS_H
