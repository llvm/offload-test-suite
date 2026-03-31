//===- DX/Device.cpp - DirectX Device API ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/Device.h"

#include "Config.h"

#include "llvm/Support/Error.h"

#include <cstdlib>
#include <memory>

using namespace offloadtest;

Queue::~Queue() {}

Device::~Device() {}

llvm::Expected<llvm::SmallVector<std::shared_ptr<Device>>>
offloadtest::initializeDevices(const DeviceConfig Config) {
  llvm::SmallVector<std::shared_ptr<Device>> Devices;

#ifdef OFFLOADTEST_ENABLE_D3D12
  if (auto Err = initializeDX12Devices(Config, Devices))
    return Err;
#endif

#ifdef OFFLOADTEST_ENABLE_VULKAN
  if (auto Err = initializeVulkanDevices(Config, Devices))
    return Err;
#endif

#ifdef OFFLOADTEST_ENABLE_METAL
  if (auto Err = initializeMetalDevices(Config, Devices))
    return Err;
#endif

  return Devices;
}