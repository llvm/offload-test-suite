//===- VK/AccelerationStructure.h - Offload API VK AccelStruct API --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_ACCELERATIONSTRUCTURE_H
#define OFFLOADTEST_API_VK_ACCELERATIONSTRUCTURE_H

#include "API/AccelerationStructure.h"

#include <vulkan/vulkan.h>

namespace offloadtest {

class VulkanAccelerationStructure : public offloadtest::AccelerationStructure {
public:
  VkDevice Dev;
  VkAccelerationStructureKHR AccelStruct;
  VkBuffer Buffer;
  VkDeviceMemory Memory;
  VkDeviceAddress DeviceAddress;
  PFN_vkDestroyAccelerationStructureKHR FnDestroyAS;

  VulkanAccelerationStructure(VkDevice Dev,
                              VkAccelerationStructureKHR AccelStruct,
                              VkBuffer Buffer, VkDeviceMemory Memory,
                              VkDeviceAddress DeviceAddress,
                              PFN_vkDestroyAccelerationStructureKHR FnDestroyAS,
                              const AccelerationStructureSizes &Sizes)
      : offloadtest::AccelerationStructure(GPUAPI::Vulkan, Sizes), Dev(Dev),
        AccelStruct(AccelStruct), Buffer(Buffer), Memory(Memory),
        DeviceAddress(DeviceAddress), FnDestroyAS(FnDestroyAS) {}

  ~VulkanAccelerationStructure() override;

  VkDeviceAddress getDeviceAddress() const { return DeviceAddress; }

  static bool classof(const offloadtest::AccelerationStructure *AS) {
    return AS->getAPI() == GPUAPI::Vulkan;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_ACCELERATIONSTRUCTURE_H
