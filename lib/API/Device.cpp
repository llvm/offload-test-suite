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

namespace {
class DeviceContext {
public:
  using DeviceArray = Device::DeviceArray;
  using DeviceIterator = Device::DeviceIterator;

private:
  DeviceArray Devices;

  DeviceContext() = default;
  ~DeviceContext() = default;
  DeviceContext(const DeviceContext &) = delete;

public:
  static DeviceContext &instance() {
    static DeviceContext Ctx;
    return Ctx;
  }

  void registerDevice(std::shared_ptr<Device> D) { Devices.push_back(D); }
  void unregisterDevices() { Devices.clear(); }

  DeviceIterator begin() { return Devices.begin(); }

  DeviceIterator end() { return Devices.end(); }
};
} // namespace

Device::~Device() {}

void Device::registerDevice(std::shared_ptr<Device> D) {
  DeviceContext::instance().registerDevice(D);
}

llvm::Error Device::initialize() {
#ifdef OFFLOADTEST_ENABLE_D3D12
  if (auto Err = initializeDXDevices())
    return Err;
#endif
#ifdef OFFLOADTEST_ENABLE_VULKAN
  if (auto Err = initializeVKDevices())
    return Err;
  // Validation layers have internal state which require a specific destruction
  // ordering. Relying on the global dtor call for this is unreliable and can
  // cause a null-deref in the validation layers during the final
  // vkDestroyInstance. This is a known limitation of the validation layers
  // which explicitely requires using atexit.
  atexit(Device::cleanupVKDevices);
#endif
#ifdef OFFLOADTEST_ENABLE_METAL
  if (auto Err = initializeMtlDevices())
    return Err;
#endif
  return llvm::Error::success();
}

void Device::uninitialize() { DeviceContext::instance().unregisterDevices(); }

Device::DeviceIterator Device::begin() {
  return DeviceContext::instance().begin();
}

Device::DeviceIterator Device::end() { return DeviceContext::instance().end(); }
