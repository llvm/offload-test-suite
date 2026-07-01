//===- VK/Sampler.h - Offload API VK Sampler API --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_SAMPLER_H
#define OFFLOADTEST_API_VK_SAMPLER_H

#include "API/Sampler.h"

#include <vulkan/vulkan.h>

#include <string>

namespace offloadtest {

class VulkanSampler : public offloadtest::Sampler {
public:
  VkSampler Sampler;
  VkDevice Device;
  std::string Name;
  SamplerCreateDesc Desc;

  VulkanSampler(std::string Name, const SamplerCreateDesc &Desc,
                VkSampler Sampler, VkDevice Device)
      : offloadtest::Sampler(GPUAPI::Vulkan), Sampler(Sampler), Device(Device),
        Name(std::move(Name)), Desc(Desc) {}
  ~VulkanSampler() override;

  const SamplerCreateDesc &getDesc() const override;

  static bool classof(const offloadtest::Sampler *S);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_SAMPLER_H
