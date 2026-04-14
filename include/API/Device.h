//===- Device.h - Offload API Device API ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DEVICE_H
#define OFFLOADTEST_API_DEVICE_H

#include "Config.h"

#include "API/API.h"
#include "API/Buffer.h"
#include "API/Capabilities.h"
#include "API/CommandBuffer.h"
#include "API/Texture.h"
#include "API/VertexBuffer.h"
#include "Support/Pipeline.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/Error.h"

#include <memory>
#include <string>

namespace llvm {
class raw_ostream;
} // namespace llvm

namespace offloadtest {

struct Pipeline;

struct DeviceConfig {
  bool EnableDebugLayer = false;
  bool EnableValidationLayer = false;
};

class Fence {
public:
  virtual ~Fence();

  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;

  virtual uint64_t getFenceValue() = 0;
  virtual llvm::Error waitForCompletion(uint64_t SignalValue) = 0;

protected:
  Fence() = default;
};

class Queue {
public:
  virtual ~Queue() = 0;

protected:
  Queue() = default;
};

class Device {
protected:
  std::string Description;
  std::string DriverName;

public:
  virtual const Capabilities &getCapabilities() = 0;
  virtual llvm::StringRef getAPIName() const = 0;
  virtual GPUAPI getAPI() const = 0;
  virtual llvm::Error executeProgram(Pipeline &P) = 0;

  virtual Queue &getGraphicsQueue() = 0;

  virtual llvm::Expected<std::unique_ptr<Fence>>
  createFence(llvm::StringRef Name) = 0;

  virtual llvm::Expected<std::shared_ptr<Buffer>>
  createBuffer(std::string Name, BufferCreateDesc &Desc,
               size_t SizeInBytes) = 0;

  virtual llvm::Expected<std::shared_ptr<Texture>>
  createTexture(std::string Name, TextureCreateDesc &Desc) = 0;

  virtual void printExtra(llvm::raw_ostream &OS) {}

  virtual llvm::Expected<std::unique_ptr<CommandBuffer>>
  createCommandBuffer() = 0;

  virtual ~Device() = 0;

  llvm::StringRef getDescription() const { return Description; }
  llvm::StringRef getDriverName() const { return DriverName; }
};

llvm::Error
initializeDX12Devices(const DeviceConfig Config,
                      llvm::SmallVectorImpl<std::unique_ptr<Device>> &Devices);
llvm::Error initializeVulkanDevices(
    const DeviceConfig Config,
    llvm::SmallVectorImpl<std::unique_ptr<Device>> &Devices);
llvm::Error
initializeMetalDevices(const DeviceConfig Config,
                       llvm::SmallVectorImpl<std::unique_ptr<Device>> &Devices);
llvm::Expected<llvm::SmallVector<std::unique_ptr<Device>>>
initializeDevices(const DeviceConfig Config);

// Creates a render target texture using the format and dimensions from a
// CPUBuffer. Does not upload the buffer's data — only uses its description to
// configure the texture.
llvm::Expected<std::shared_ptr<Texture>>
createRenderTargetFromCPUBuffer(Device &Dev, const CPUBuffer &Buf);

// Creates a depth/stencil texture matching the dimensions of a render target.
llvm::Expected<std::shared_ptr<Texture>>
createDefaultDepthStencilTarget(Device &Dev, uint32_t Width, uint32_t Height);

// Creates a VertexBuffer from a ParsedVertexBuffer.
llvm::Expected<VertexBuffer> createVertexBuffer(Device &Dev,
                                                const ParsedVertexBuffer &PVB);

} // namespace offloadtest

#endif // OFFLOADTEST_API_DEVICE_H
