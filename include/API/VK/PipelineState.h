//===- VK/PipelineState.h - Offload API VK PipelineState API --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_PIPELINESTATE_H
#define OFFLOADTEST_API_VK_PIPELINESTATE_H

#include "API/Device.h"
#include "API/VK/Descriptors.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace offloadtest {

class VulkanPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  VkDevice Dev;
  VkPipeline Pipeline;
  VkPipelineLayout Layout;
  llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
  DescriptorCounts DescCounts;
  // True for pipelines created via createPipelineRT — used by SBT / dispatch
  // code to safely downcast to VKRayTracingPipelineState.
  bool IsRayTracing = false;

  VulkanPipelineState(llvm::StringRef Name, VkDevice Dev, VkPipeline Pipeline,
                      VkPipelineLayout Layout,
                      llvm::SmallVector<VkDescriptorSetLayout> SetLayouts,
                      const DescriptorCounts &DescCounts, bool IsRT = false)
      : offloadtest::PipelineState(GPUAPI::Vulkan), Name(Name.str()), Dev(Dev),
        Pipeline(Pipeline), Layout(Layout), SetLayouts(std::move(SetLayouts)),
        DescCounts(DescCounts), IsRayTracing(IsRT) {}

  ~VulkanPipelineState() override;

  static bool classof(const offloadtest::PipelineState *B);
};

/// RT pipeline state with the extra metadata needed to build a shader binding
/// table — the group-index mapping resolves SBT-record `ShaderName`s to the
/// shader-group index in this pipeline, and the per-bucket counts allow the
/// SBT builder to slice the contiguous handle blob returned by
/// `vkGetRayTracingShaderGroupHandlesKHR` into raygen / miss / hit / callable
/// regions.
class VKRayTracingPipelineState : public VulkanPipelineState {
public:
  // Maps each raygen / miss / callable shader's `EntryPoint` and each
  // hit group's `Name` to its index in the pipeline's
  // `VkRayTracingShaderGroupCreateInfoKHR[]`.
  llvm::StringMap<uint32_t> ShaderGroupIndices;

  // Group counts laid out in pipeline order: raygen, miss, hit, callable.
  uint32_t NumRaygenGroups = 0;
  uint32_t NumMissGroups = 0;
  uint32_t NumHitGroups = 0;
  uint32_t NumCallableGroups = 0;

  uint32_t totalGroupCount() const {
    return NumRaygenGroups + NumMissGroups + NumHitGroups + NumCallableGroups;
  }

  VKRayTracingPipelineState(llvm::StringRef Name, VkDevice Dev,
                            VkPipeline Pipeline, VkPipelineLayout Layout,
                            llvm::SmallVector<VkDescriptorSetLayout> SetLayouts,
                            const DescriptorCounts &DescCounts)
      : VulkanPipelineState(Name, Dev, Pipeline, Layout, std::move(SetLayouts),
                            DescCounts,
                            /*IsRT=*/true) {}

  static bool classof(const offloadtest::PipelineState *B);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_PIPELINESTATE_H
