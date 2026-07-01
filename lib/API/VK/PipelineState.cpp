//===- VK/PipelineState.cpp - Vulkan PipelineState API --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/PipelineState.h"

using namespace offloadtest;

VulkanPipelineState::~VulkanPipelineState() {
  vkDestroyPipeline(Dev, Pipeline, nullptr);
  vkDestroyPipelineLayout(Dev, Layout, nullptr);
  for (VkDescriptorSetLayout L : SetLayouts)
    vkDestroyDescriptorSetLayout(Dev, L, nullptr);
}

bool VulkanPipelineState::classof(const offloadtest::PipelineState *B) {
  return B->getAPI() == GPUAPI::Vulkan;
}

bool VKRayTracingPipelineState::classof(const offloadtest::PipelineState *B) {
  if (B->getAPI() != GPUAPI::Vulkan)
    return false;
  return static_cast<const VulkanPipelineState *>(B)->IsRayTracing;
}
