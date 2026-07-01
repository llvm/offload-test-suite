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

VkShaderStageFlagBits offloadtest::getShaderStageFlag(Stages Stage) {
  switch (Stage) {
  case Stages::Compute:
    return VK_SHADER_STAGE_COMPUTE_BIT;
  case Stages::Vertex:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case Stages::Hull:
    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  case Stages::Domain:
    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  case Stages::Geometry:
    return VK_SHADER_STAGE_GEOMETRY_BIT;
  case Stages::Pixel:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  case Stages::Amplification:
    return VK_SHADER_STAGE_TASK_BIT_EXT;
  case Stages::Mesh:
    return VK_SHADER_STAGE_MESH_BIT_EXT;
  case Stages::RayGeneration:
    return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  case Stages::Miss:
    return VK_SHADER_STAGE_MISS_BIT_KHR;
  case Stages::ClosestHit:
    return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  case Stages::AnyHit:
    return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
  case Stages::Intersection:
    return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
  case Stages::Callable:
    return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
  }
  llvm_unreachable("All cases handled");
}

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
