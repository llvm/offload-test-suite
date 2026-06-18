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
#include "API/AccelerationStructure.h"
#include "API/Buffer.h"
#include "API/Capabilities.h"
#include "API/CommandBuffer.h"
#include "API/RenderPass.h"
#include "API/ShaderBindingTable.h"
#include "API/Texture.h"

#include "Support/Pipeline.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ErrorHandling.h"
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

struct ResourceBindingDesc {
  ResourceKind Kind;
  DirectXBinding DXBinding;
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

struct RayTracingShader {
  Stages Stage;
  std::string EntryPoint;
};

struct RayTracingPipelineCreateDesc {
  // All RT shaders are compiled into a single shader library.
  // Every entry in `Shaders` references this same blob via the backend's
  // library-loading path.
  const llvm::MemoryBuffer *Library = nullptr;
  llvm::SmallVector<RayTracingShader> Shaders;
  llvm::SmallVector<HitGroup> HitGroups;
  RayTracingPipelineConfig Config;
};

struct TraditionalRasterPipelineCreateDesc {
  llvm::SmallVector<InputLayoutDesc> InputLayout;
  llvm::SmallVector<Format> RTFormats;
  std::optional<Format> DSFormat;
  uint32_t SampleCount = 1;
  PrimitiveTopology Topology;
  // Set if Topology == PatchList. Validated in
  // Pipeline.cpp::validatePipelineKind.
  std::optional<uint32_t> PatchControlPoints;

  ShaderContainer VS;
  // Hull and Domain are independent optionals here; Pipeline.cpp enforces that
  // they must be set as a pair (and only with PatchList topology).
  std::optional<ShaderContainer> HS;
  std::optional<ShaderContainer> DS;
  std::optional<ShaderContainer> GS;
  ShaderContainer PS;

  void setShader(Stages Stage, ShaderContainer &&SC) {
    switch (Stage) {
    case Stages::Vertex:
      VS = std::move(SC);
      break;
    case Stages::Hull:
      HS = std::move(SC);
      break;
    case Stages::Domain:
      DS = std::move(SC);
      break;
    case Stages::Geometry:
      GS = std::move(SC);
      break;
    case Stages::Pixel:
      PS = std::move(SC);
      break;
    case Stages::Compute:
    case Stages::Amplification:
    case Stages::Mesh:
    case Stages::RayGeneration:
    case Stages::Miss:
    case Stages::ClosestHit:
    case Stages::AnyHit:
    case Stages::Intersection:
    case Stages::Callable:
      llvm_unreachable("Not a traditional raster pipeline stage.");
    }
  }
};

struct MeshShaderRasterPipelineCreateDesc {
  llvm::SmallVector<Format> RTFormats;
  std::optional<Format> DSFormat;
  PrimitiveTopology Topology;

  ShaderContainer MS;
  std::optional<ShaderContainer> AS;
  std::optional<ShaderContainer> PS;

  void setShader(Stages Stage, ShaderContainer &&SC) {
    switch (Stage) {
    case Stages::Amplification:
      AS = std::move(SC);
      break;
    case Stages::Mesh:
      MS = std::move(SC);
      break;
    case Stages::Pixel:
      PS = std::move(SC);
      break;
    case Stages::Vertex:
    case Stages::Hull:
    case Stages::Domain:
    case Stages::Geometry:
    case Stages::Compute:
    case Stages::RayGeneration:
    case Stages::Miss:
    case Stages::ClosestHit:
    case Stages::AnyHit:
    case Stages::Intersection:
    case Stages::Callable:
      llvm_unreachable("Not a mesh raster pipeline stage.");
    }
  }
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
  std::string DriverVersion;
  std::string GPUGeneration;
  uint16_t FamilyPrefix = 0;

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
  createTraditionalRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const TraditionalRasterPipelineCreateDesc &Desc) = 0;

  virtual llvm::Expected<std::unique_ptr<PipelineState>>
  createMeshShaderRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const MeshShaderRasterPipelineCreateDesc &Desc) = 0;

  virtual llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineRT(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                   const RayTracingPipelineCreateDesc &Desc) = 0;

  virtual llvm::Expected<std::unique_ptr<ShaderBindingTable>>
  createShaderBindingTable(const PipelineState &PSO,
                           const ShaderBindingTableDesc &Desc) = 0;

  virtual llvm::Expected<std::unique_ptr<Fence>>
  createFence(llvm::StringRef Name) = 0;

  virtual llvm::Expected<std::unique_ptr<Buffer>>
  createBuffer(std::string Name, const BufferCreateDesc &Desc,
               size_t SizeInBytes) = 0;

  virtual llvm::Expected<std::unique_ptr<Texture>>
  createTexture(std::string Name, const TextureCreateDesc &Desc) = 0;

  // The row stride required when uploading data to (or reading back from) a
  // texture created with the given description, via an upload buffer.
  virtual uint32_t
  getTextureUploadRowStrideInBytes(const TextureCreateDesc &Desc) const = 0;

  virtual llvm::Expected<std::unique_ptr<RenderPass>>
  createRenderPass(const RenderPassDesc &Desc) = 0;

  virtual void printExtra(llvm::raw_ostream &OS) {}

  virtual llvm::Expected<std::unique_ptr<CommandBuffer>>
  createCommandBuffer() = 0;

  // Sizing queries: return the result/scratch sizes the backend needs to
  // allocate an AS that can hold the given build inputs. The build-input
  // pointers (geometry buffers, BLAS handles) are never consulted — only
  // counts/strides — so these can be called before BLAS handles exist.
  virtual llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<TriangleGeometryDesc> Triangles) = 0;

  virtual llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) = 0;

  virtual llvm::Expected<AccelerationStructureSizes>
  getTLASBuildSizes(uint32_t InstanceCount) = 0;

  // Allocate the AS storage (no GPU build). The build is recorded later via
  // ComputeEncoder::batchBuildAS(), which is the only place that consumes the
  // associated *BuildRequest.
  virtual llvm::Expected<std::unique_ptr<AccelerationStructure>>
  createBLAS(const AccelerationStructureSizes &Sizes) = 0;

  virtual llvm::Expected<std::unique_ptr<AccelerationStructure>>
  createTLAS(const AccelerationStructureSizes &Sizes) = 0;

  virtual ~Device() = 0;

  llvm::StringRef getDescription() const { return Description; }
  llvm::StringRef getDriverName() const { return DriverName; }
  llvm::StringRef getDriverVersion() const { return DriverVersion; }
  llvm::StringRef getGPUGeneration() const { return GPUGeneration; }
  uint16_t getFamilyPrefix() const { return FamilyPrefix; }
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
// SampleCount must match the matching color render target when used in the
// same render pass (default 1, i.e. non-MSAA).
llvm::Expected<std::unique_ptr<Texture>>
createDefaultDepthStencilTarget(Device &Dev, uint32_t Width, uint32_t Height,
                                uint32_t SampleCount = 1);

llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
createBufferWithData(Device &Dev, std::string Name,
                     const BufferCreateDesc &Desc, const void *Data,
                     size_t SizeInBytes, ComputeEncoder *Encoder,
                     std::unique_ptr<offloadtest::Buffer> *OutUploadBuffer);

llvm::Expected<std::unique_ptr<offloadtest::Texture>>
createTextureWithData(Device &Dev, std::string Name,
                      const TextureCreateDesc &Desc, const void *Data,
                      size_t SizeInBytes, ComputeEncoder *Encoder,
                      std::unique_ptr<offloadtest::Buffer> *OutUploadBuffer);

// TLAS handles come in pre-allocated because the caller's binding loop
// stamps the AS pointer into descriptor bundles before this helper runs;
// BLAS handles are allocated inline since BLASes aren't user-bindable.
// BLAS and TLAS builds get separate `Enc.batchBuildAS()` calls so the
// implicit BLAS-write → TLAS-read barrier sits between them. Outputs
// (`OutBLAS`, `OutInputBuffers`) must outlive command-buffer submission.
//
// TODO: `Pipeline` belongs to the test framework, not the rendering backend
// API. This helper lives here only because `executeProgram` is still on
// `Device` — once that moves out, this helper should follow.
llvm::Error buildPipelineAccelerationStructures(
    Device &Dev, ComputeEncoder &Enc, Pipeline &P,
    llvm::SmallVectorImpl<std::unique_ptr<AccelerationStructure>> &OutBLAS,
    const llvm::StringMap<std::unique_ptr<AccelerationStructure>>
        &PreallocatedTLASes,
    llvm::SmallVectorImpl<std::unique_ptr<Buffer>> &OutInputBuffers);

} // namespace offloadtest

#endif // OFFLOADTEST_API_DEVICE_H
