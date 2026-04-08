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
#include "API/Capabilities.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"

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

enum class MemoryLocation {
  GpuOnly,
  CpuToGpu,
  GpuToCpu,
};

struct BufferCreateDesc {
  MemoryLocation Location;
};

class Buffer {
public:
  virtual ~Buffer() = default;

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

protected:
  Buffer() = default;
};

class Fence {
public:
  virtual ~Fence() = default;
  virtual uint64_t getFenceValue() = 0;
  virtual llvm::Error waitForCompletion(uint64_t SignalValue) = 0;
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
  virtual void printExtra(llvm::raw_ostream &OS) {}

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

} // namespace offloadtest

#endif // OFFLOADTEST_API_DEVICE_H
