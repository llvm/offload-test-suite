//===- VK/AccelerationStructure.cpp - Vulkan AccelStruct API --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/AccelerationStructure.h"

using namespace offloadtest;

VulkanAccelerationStructure::~VulkanAccelerationStructure() {
  if (AccelStruct != VK_NULL_HANDLE)
    FnDestroyAS(Dev, AccelStruct, nullptr);
  if (Buffer != VK_NULL_HANDLE)
    vkDestroyBuffer(Dev, Buffer, nullptr);
  if (Memory != VK_NULL_HANDLE)
    vkFreeMemory(Dev, Memory, nullptr);
}
