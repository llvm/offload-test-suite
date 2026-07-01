//===- VK/SBT.h - Offload API VK Shader Binding Table API -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_SBT_H
#define OFFLOADTEST_API_VK_SBT_H

#include "API/Device.h"

#include <vulkan/vulkan.h>

namespace offloadtest {

class VKShaderBindingTable : public offloadtest::ShaderBindingTable {
public:
  VkDevice Dev;
  VkBuffer Buffer;
  VkDeviceMemory Memory;
  VkStridedDeviceAddressRegionKHR RaygenRegion{};
  VkStridedDeviceAddressRegionKHR MissRegion{};
  VkStridedDeviceAddressRegionKHR HitRegion{};
  VkStridedDeviceAddressRegionKHR CallableRegion{};

  VKShaderBindingTable(VkDevice Dev, VkBuffer Buffer, VkDeviceMemory Memory,
                       VkStridedDeviceAddressRegionKHR Raygen,
                       VkStridedDeviceAddressRegionKHR Miss,
                       VkStridedDeviceAddressRegionKHR Hit,
                       VkStridedDeviceAddressRegionKHR Callable)
      : offloadtest::ShaderBindingTable(GPUAPI::Vulkan), Dev(Dev),
        Buffer(Buffer), Memory(Memory), RaygenRegion(Raygen), MissRegion(Miss),
        HitRegion(Hit), CallableRegion(Callable) {}

  ~VKShaderBindingTable() override;

  static bool classof(const offloadtest::ShaderBindingTable *S) {
    return S->getAPI() == GPUAPI::Vulkan;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_SBT_H
