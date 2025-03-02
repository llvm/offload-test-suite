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

using namespace offloadtest;

#ifdef OFFLOADTEST_ENABLE_D3D12
llvm::Error InitializeDXDevices();
#endif

#ifdef OFFLOADTEST_ENABLE_VULKAN
llvm::Error InitializeVXDevices();
#endif

#ifdef OFFLOADTEST_ENABLE_METAL
llvm::Error InitializeMTLDevices();
#endif

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
  static DeviceContext &Instance() {
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
  DeviceContext::Instance().registerDevice(D);
}

llvm::Error Device::initialize() {
#ifdef OFFLOADTEST_ENABLE_D3D12
  if (auto Err = InitializeDXDevices())
    return Err;
#endif
#ifdef OFFLOADTEST_ENABLE_VULKAN
  if (auto Err = InitializeVXDevices())
    return Err;
#endif
#ifdef OFFLOADTEST_ENABLE_METAL
  if (auto Err = InitializeMTLDevices())
    return Err;
#endif
  return llvm::Error::success();
}

void Device::uninitialize() {
  DeviceContext::Instance().unregisterDevices();
}

Device::DeviceIterator Device::begin() {
  return DeviceContext::Instance().begin();
}

Device::DeviceIterator Device::end() { return DeviceContext::Instance().end(); }
