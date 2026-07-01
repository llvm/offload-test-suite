//===- VK/Sampler.cpp - Vulkan Sampler API --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/Sampler.h"

using namespace offloadtest;

VulkanSampler::~VulkanSampler() {
  if (Sampler)
    vkDestroySampler(Device, Sampler, nullptr);
}

const SamplerCreateDesc &VulkanSampler::getDesc() const { return Desc; }

bool VulkanSampler::classof(const offloadtest::Sampler *S) {
  return S->getAPI() == GPUAPI::Vulkan;
}
