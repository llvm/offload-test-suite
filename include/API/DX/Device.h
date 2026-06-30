//===- DX/Device.h - Offload API DX Device API ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_DEVICE_H
#define OFFLOADTEST_API_DX_DEVICE_H

#include "API/DX/Common.h"
#include "API/DX/Queue.h"
#include "API/Device.h"

#include <cstdint>
#include <string>

namespace offloadtest {

struct DescriptorSetsLayout;

struct DescriptorAllocator {
  ComPtr<ID3D12DescriptorHeap> Heap;
  std::atomic<uint32_t> NextIndex{0};
  uint32_t DescIncSize;
  uint32_t Capacity;

  static llvm::Expected<DescriptorAllocator>
  create(ID3D12DeviceX *Device, D3D12_DESCRIPTOR_HEAP_TYPE Type,
         uint32_t Capacity);

  llvm::Expected<D3D12_CPU_DESCRIPTOR_HANDLE> allocate();

  DescriptorAllocator(DescriptorAllocator &&Other)
      : Heap(std::move(Other.Heap)),
        NextIndex(Other.NextIndex.load(std::memory_order_relaxed)),
        DescIncSize(Other.DescIncSize), Capacity(Other.Capacity) {}
  DescriptorAllocator(const DescriptorAllocator &) = delete;
  DescriptorAllocator &operator=(const DescriptorAllocator &) = delete;
  DescriptorAllocator &operator=(DescriptorAllocator &&) = delete;

  DescriptorAllocator(ComPtr<ID3D12DescriptorHeap> Heap, uint32_t DescIncSize,
                      uint32_t Capacity)
      : Heap(Heap), DescIncSize(DescIncSize), Capacity(Capacity) {}
};

class DXDevice : public offloadtest::Device {
public:
  ComPtr<IDXCoreAdapter> Adapter;
  ComPtr<ID3D12DeviceX> Device;
  DXQueue GraphicsQueue;
  Capabilities Caps;
  DescriptorAllocator RTVAllocator;
  DescriptorAllocator DSVAllocator;
  DescriptorAllocator CSUAllocator;
  DescriptorAllocator SamplerAllocator;

  DXDevice(ComPtr<IDXCoreAdapter> A, ComPtr<ID3D12DeviceX> D, DXQueue Q,
           DescriptorAllocator RTVAllocator, DescriptorAllocator DSVAllocator,
           DescriptorAllocator CSUAllocator,
           DescriptorAllocator SamplerAllocator, std::string Desc,
           std::string DriverVer);
  DXDevice(const DXDevice &) = delete;
  DXDevice &operator=(const DXDevice &) = delete;

  ~DXDevice() override;

  llvm::StringRef getAPIName() const override;
  GPUAPI getAPI() const override;

  static bool classof(const offloadtest::Device *D);

  Queue &getGraphicsQueue() override;

  llvm::Error
  createRootSignatureFromShader(llvm::StringRef Name,
                                const ShaderContainer &Shader,
                                ComPtr<ID3D12RootSignature> &OutRootSignature,
                                DescriptorSetsLayout &Layout);

  llvm::Error createRootSignatureFromBindingsDesc(
      llvm::StringRef Name, const BindingsDesc &BndDesc, bool IsGraphics,
      ComPtr<ID3D12RootSignature> &OutRootSignature,
      DescriptorSetsLayout &Layout);

  llvm::Error createRootSignature(llvm::StringRef Name,
                                  const BindingsDesc &BndDesc,
                                  const ShaderContainer &Shader,
                                  bool IsGraphics,
                                  ComPtr<ID3D12RootSignature> &OutRootSignature,
                                  DescriptorSetsLayout &Layout);

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BndDesc,
                   ShaderContainer CS) override;

  llvm::Expected<std::unique_ptr<PipelineState>>
  createTraditionalRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BndDesc,
      const TraditionalRasterPipelineCreateDesc &Desc) override;

  llvm::Expected<std::unique_ptr<PipelineState>> createMeshShaderRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const MeshShaderRasterPipelineCreateDesc &Desc) override;

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineRT(llvm::StringRef Name, const BindingsDesc &BndDesc,
                   const RayTracingPipelineCreateDesc &Desc) override;

  llvm::Expected<std::unique_ptr<ShaderBindingTable>>
  createShaderBindingTable(const PipelineState &PSO,
                           const ShaderBindingTableDesc &Desc) override;

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override;

  llvm::Expected<std::unique_ptr<offloadtest::MemoryHeap>>
  createMemoryHeap(std::string Name, size_t SizeInBytes) override;

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, const BufferCreateDesc &Desc,
               size_t SizeInBytes) override;

  llvm::Expected<std::unique_ptr<offloadtest::Texture>>
  createTexture(std::string Name, const TextureCreateDesc &Desc) override;

  llvm::Expected<std::unique_ptr<Sampler>>
  createSampler(std::string Name, const SamplerCreateDesc &Desc) override;

  uint32_t getTextureUploadRowStrideInBytes(
      const TextureCreateDesc &Desc) const override;

  TextureUploadLayout
  getTextureUploadLayout(const TextureCreateDesc &Desc) const override;

  static llvm::Expected<std::unique_ptr<offloadtest::Device>>
  create(ComPtr<IDXCoreAdapter> Adapter, const DeviceConfig &Config);

  const Capabilities &getCapabilities() override;

  void queryCapabilities();

  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override;

  llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
  createRenderPass(const offloadtest::RenderPassDesc &Desc) override;

  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<TriangleGeometryDesc> Triangles) override;

  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) override;

  AccelerationStructureSizes queryBLASPrebuildSize(
      llvm::ArrayRef<D3D12_RAYTRACING_GEOMETRY_DESC> GeomDescs);

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  allocateAS(const AccelerationStructureSizes &Sizes, const char *Kind);

  llvm::Expected<AccelerationStructureSizes>
  getTLASBuildSizes(uint32_t InstanceCount) override;

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createBLAS(const AccelerationStructureSizes &Sizes) override;

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createTLAS(const AccelerationStructureSizes &Sizes,
             uint32_t /*InstanceCount*/) override;

  llvm::Expected<std::unique_ptr<DescriptorPool>>
  createDescriptorPool() override;

  llvm::Expected<std::unique_ptr<DescriptorSetsBuilder>>
  createDescriptorSetsBuilder(DescriptorPool &Pool,
                              const PipelineState &Pipeline) override;

  llvm::Error executeProgram(Pipeline &P) override;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_DEVICE_H
