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

Queue::~Queue() {}

Device::~Device() {}

void Device::registerDevice(std::shared_ptr<Device> D) {
  DeviceContext::instance().registerDevice(D);
}

llvm::Error Device::initialize(const DeviceConfig Config) {
#ifdef OFFLOADTEST_ENABLE_D3D12
  if (auto Err = initializeDXDevices(Config))
    return Err;
#endif
#ifdef OFFLOADTEST_ENABLE_VULKAN
  if (auto Err = initializeVulkanDevices(Config))
    return Err;
  // Validation layers have internal state which require a specific destruction
  // ordering. Relying on the global dtor call for this is unreliable and can
  // cause a null-deref in the validation layers during the final
  // vkDestroyInstance. This is a known limitation of the validation layers
  // which explicitely requires using atexit.
  atexit(Device::cleanupVulkanDevices);
#endif
#ifdef OFFLOADTEST_ENABLE_METAL
  if (auto Err = initializeMtlDevices(Config))
    return Err;
#endif
  return llvm::Error::success();
}

void Device::uninitialize() { DeviceContext::instance().unregisterDevices(); }

Device::DeviceIterator Device::begin() {
  return DeviceContext::instance().begin();
}

Device::DeviceIterator Device::end() { return DeviceContext::instance().end(); }

llvm::Expected<std::shared_ptr<Texture>>
offloadtest::createRenderTarget(Device &Dev, const CPUBuffer &Buf) {
  auto TexFmtOrErr = toTextureFormat(Buf.Format, Buf.Channels);
  if (!TexFmtOrErr)
    return TexFmtOrErr.takeError();

  TextureCreateDesc Desc = {};
  Desc.Location = MemoryLocation::GpuOnly;
  Desc.Usage = TextureUsage::RenderTarget;
  Desc.Format = *TexFmtOrErr;
  Desc.Width = Buf.OutputProps.Width;
  Desc.Height = Buf.OutputProps.Height;
  Desc.MipLevels = 1;
  Desc.OptimizedClearValue = ClearColor{};

  if (auto Err = validateTextureDescMatchesCPUBuffer(Desc, Buf))
    return Err;

  return Dev.createTexture("RenderTarget", Desc);
}

llvm::Expected<std::shared_ptr<Texture>>
createDepthStencil(Device &Dev, uint32_t Width, uint32_t Height) {
  TextureCreateDesc Desc = {};
  Desc.Location = MemoryLocation::GpuOnly;
  Desc.Usage = TextureUsage::DepthStencil;
  Desc.Format = TextureFormat::D32FloatS8Uint;
  Desc.Width = Width;
  Desc.Height = Height;
  Desc.MipLevels = 1;
  Desc.OptimizedClearValue = ClearDepthStencil{1.0f, 0};

  return Dev.createTexture("DepthStencil", Desc);
}
