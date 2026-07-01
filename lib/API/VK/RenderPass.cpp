//===- VK/RenderPass.cpp - Vulkan Render Pass API -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/RenderPass.h"

using namespace offloadtest;

VulkanRenderPass::~VulkanRenderPass() {
  if (Handle != VK_NULL_HANDLE)
    vkDestroyRenderPass(Dev, Handle, nullptr);
}
