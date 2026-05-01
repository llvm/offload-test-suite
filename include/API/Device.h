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

#include "Support/Pipeline.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/MemoryBuffer.h"

#include <cstdint>
#include <memory>
#include <optional>
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

struct InputLayoutDesc {
  std::string Name;
  Format Fmt;
  uint32_t OffsetInBytes;
  std::optional<uint32_t> InstanceStepRate;
};

struct DXBinding {
  uint32_t Register;
  uint32_t Space;
};

struct ResourceBindingDesc {
  ResourceKind Kind;
  DXBinding DXBinding;
  std::optional<VulkanBinding> VKBinding;
  uint32_t DescriptorCount;
};

struct DescriptorSetLayoutDesc {
  llvm::SmallVector<ResourceBindingDesc> ResourceBindings;
};

struct PushConstantsRange {
  uint32_t OffsetInBytes; // Must be multiple of 4
  uint32_t SizeInBytes;   // Must be multiple of 4
};

struct BindingsDesc {
  llvm::SmallVector<DescriptorSetLayoutDesc> DescriptorSetDescs;
  llvm::SmallVector<PushConstantsRange> PushConstantRanges;
};

struct ShaderContainer {
  std::string EntryPoint;
  const llvm::MemoryBuffer *Shader;
  llvm::SmallVector<SpecializationConstant> SpecializationConstants;
};

class PipelineState {
public:
  GPUAPI API;

  virtual ~PipelineState() = default;

  PipelineState(const Buffer &) = delete;
  PipelineState &operator=(const Buffer &) = delete;

  GPUAPI getAPI() const { return API; }

protected:
  explicit PipelineState(GPUAPI API) : API(API) {}
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

/// Returned by Queue::submit() so that callers can decide when to block.
struct SubmitResult {
  Fence *F;
  uint64_t Value;

  llvm::Error waitForCompletion() const { return F->waitForCompletion(Value); }
};

class Queue {
public:
  virtual ~Queue() = 0;
  Queue(const Queue &) = delete;
  Queue &operator=(const Queue &) = delete;
  Queue(Queue &&) = default;
  Queue &operator=(Queue &&) = default;

  /// Submit command buffers for GPU execution.  Returns a fence + value that
  /// the caller can wait on; the call itself does not block.
  virtual llvm::Expected<SubmitResult>
  submit(llvm::SmallVector<std::unique_ptr<CommandBuffer>> CBs) = 0;

  /// Convenience overload for submitting a single command buffer.
  llvm::Expected<SubmitResult> submit(std::unique_ptr<CommandBuffer> CB) {
    llvm::SmallVector<std::unique_ptr<CommandBuffer>> CBs;
    CBs.push_back(std::move(CB));
    return submit(std::move(CBs));
  }

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

  virtual llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                   ShaderContainer CS) = 0;

  virtual llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineVsPs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                     llvm::ArrayRef<InputLayoutDesc> InputLayout,
                     llvm::ArrayRef<Format> RTFormats,
                     std::optional<Format> DSFormat, ShaderContainer VS,
                     ShaderContainer PS) = 0;

  virtual llvm::Expected<std::unique_ptr<Fence>>
  createFence(llvm::StringRef Name) = 0;

  virtual llvm::Expected<std::unique_ptr<Buffer>>
  createBuffer(std::string Name, BufferCreateDesc &Desc,
               size_t SizeInBytes) = 0;

  virtual llvm::Expected<std::unique_ptr<Texture>>
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
llvm::Expected<std::unique_ptr<Texture>>
createRenderTargetFromCPUBuffer(Device &Dev, const CPUBuffer &Buf);

// Creates a depth/stencil texture matching the dimensions of a render target.
llvm::Expected<std::unique_ptr<Texture>>
createDefaultDepthStencilTarget(Device &Dev, uint32_t Width, uint32_t Height);

} // namespace offloadtest

#endif // OFFLOADTEST_API_DEVICE_H
