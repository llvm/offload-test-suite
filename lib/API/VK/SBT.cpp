//===- VK/SBT.cpp - Vulkan Shader Binding Table API -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/SBT.h"

using namespace offloadtest;

VKShaderBindingTable::~VKShaderBindingTable() {
  vkDestroyBuffer(Dev, Buffer, nullptr);
  vkFreeMemory(Dev, Memory, nullptr);
}
