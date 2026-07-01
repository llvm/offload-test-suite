//===- VK/Device.h - Offload API VK Device API ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_DEVICE_H
#define OFFLOADTEST_API_VK_DEVICE_H

#include "API/Capabilities.h"
#include "API/Device.h"
#include "API/Encoder.h"
#include "API/Texture.h"
#include "API/VK/AccelerationStructure.h"
#include "API/VK/CommandBuffer.h"
#include "API/VK/Queue.h"
#include "Support/Pipeline.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include <vulkan/vulkan.h>

#include <memory>

namespace offloadtest {

struct SharedInvocationState;

struct VulkanInstance {
  VkInstance Instance;
  VkDebugUtilsMessengerEXT DebugMessenger;

  VulkanInstance(VkInstance Instance, VkDebugUtilsMessengerEXT DebugMessenger)
      : Instance(Instance), DebugMessenger(DebugMessenger) {}
  VulkanInstance(const VulkanInstance &) = delete;
  VulkanInstance(VulkanInstance &&) = delete;
  VulkanInstance &operator=(const VulkanInstance &) = delete;
  VulkanInstance &operator=(VulkanInstance &&) = delete;
  ~VulkanInstance() {
    if (DebugMessenger) {
      auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          Instance, "vkDestroyDebugUtilsMessengerEXT");
      assert(Func != nullptr);
      Func(Instance, DebugMessenger, nullptr);
    }
    vkDestroyInstance(Instance, nullptr);
  }
};

class VulkanDevice : public offloadtest::Device {
  // VKComputeEncoder needs access to the device's RT entry points and the
  // raw VkDevice handle to record acceleration-structure build commands.
  friend class VKComputeEncoder;

private:
  // VKComputeEncoder needs access to the device's RT entry points and the
  // raw VkDevice handle to record acceleration-structure build commands.
  friend class VKComputeEncoder;

private:
  std::shared_ptr<VulkanInstance> Instance;
  VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties Props;
  VkPhysicalDeviceProperties2 Props2;
  VkPhysicalDeviceFloatControlsProperties FloatControlProp;
  VkPhysicalDeviceDriverProperties DriverProps;
  VkDevice Device = VK_NULL_HANDLE;
  VulkanQueue GraphicsQueue;
  Capabilities Caps;
  using LayerVector = llvm::SmallVector<VkLayerProperties, 0>;
  LayerVector InstanceLayers;
  using ExtensionVector = llvm::SmallVector<VkExtensionProperties, 0>;
  ExtensionVector DeviceExtensions;

  // Debug utils function pointers, resolved once per device.
  PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabel = nullptr;
  PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabel = nullptr;
  PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabel = nullptr;
  MeshShaderFunctions MeshShaderFns;

  bool HasASSupport = false;
  bool HasRTPipelineSupport = false;
  struct ASFunctions {
    PFN_vkCreateAccelerationStructureKHR Create = nullptr;
    PFN_vkDestroyAccelerationStructureKHR Destroy = nullptr;
    PFN_vkGetAccelerationStructureBuildSizesKHR GetBuildSizes = nullptr;
    PFN_vkGetAccelerationStructureDeviceAddressKHR GetDeviceAddress = nullptr;
    PFN_vkCmdBuildAccelerationStructuresKHR CmdBuild = nullptr;
  };
  ASFunctions AS;
  struct RTPipelineFunctions {
    PFN_vkCreateRayTracingPipelinesKHR CreatePipelines = nullptr;
    PFN_vkGetRayTracingShaderGroupHandlesKHR GetGroupHandles = nullptr;
    PFN_vkCmdTraceRaysKHR CmdTraceRays = nullptr;
  };
  RTPipelineFunctions RT;
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR RTPipelineProps{};

  struct BufferRef {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
  };

  struct ImageRef {
    VkImage Image;
    VkSampler Sampler;
    VkDeviceMemory Memory;
  };

  struct ResourceRef {
    explicit ResourceRef(BufferRef H, BufferRef D) : Host(H), Device(D) {}
    explicit ResourceRef(BufferRef H, ImageRef I) : Host(H), Image(I) {}
    explicit ResourceRef(VulkanAccelerationStructure *AS) : AS(AS) {}

    BufferRef Host{};
    BufferRef Device{};
    ImageRef Image{};
    // AS-only; mutually exclusive with the buffer/image fields above.
    VulkanAccelerationStructure *AS = nullptr;
  };

  struct ResourceBundle {
    ResourceBundle(VkDescriptorType DescriptorType, uint64_t Size,
                   CPUBuffer *BufferPtr)
        : DescriptorType(DescriptorType), Size(Size), BufferPtr(BufferPtr) {}

    bool isImage() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
             DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
             DescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    }

    bool isSampler() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLER;
    }

    bool isAccelerationStructure() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }

    bool isBuffer() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
             DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
             DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER ||
             DescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    }

    bool isReadWrite() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
             DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
             DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    }

    uint32_t size() const { return BufferPtr->size(); }

    VkDescriptorType DescriptorType;
    uint64_t Size;
    CPUBuffer *BufferPtr;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    llvm::SmallVector<ResourceRef> ResourceRefs;
    llvm::SmallVector<ResourceRef> CounterResourceRefs;
  };


public:
  static llvm::Expected<std::unique_ptr<VulkanDevice>>
  create(std::shared_ptr<VulkanInstance> Instance,
         VkPhysicalDevice PhysicalDevice,
         llvm::SmallVector<VkLayerProperties, 0> InstanceLayers);
  VulkanDevice(std::shared_ptr<VulkanInstance> I, VkPhysicalDevice P,
               VkPhysicalDeviceProperties Props, VkDevice D, VulkanQueue Q,
               llvm::SmallVector<VkLayerProperties, 0> InstanceLayers,
               ExtensionVector DeviceExtensions);

  VulkanDevice(const VulkanDevice &) = delete;
  ~VulkanDevice() override;
  llvm::StringRef getAPIName() const override;
  GPUAPI getAPI() const override;
  static bool classof(const offloadtest::Device *D);
  Queue &getGraphicsQueue() override;
  llvm::Error createPipelineLayout(
      const BindingsDesc &BindingsDesc, VkShaderStageFlags StageFlags,
      llvm::SmallVectorImpl<VkDescriptorSetLayout> &SetLayouts,
      VkPipelineLayout &PipelineLayout, DescriptorCounts &DescCounts);
  llvm::Expected<VkShaderModule>
  createShaderModule(const llvm::MemoryBuffer *Shader, const char *Kind);
  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                   ShaderContainer CS) override;
  llvm::Expected<std::unique_ptr<PipelineState>>
  createTraditionalRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const TraditionalRasterPipelineCreateDesc &Desc) override;
  llvm::Expected<std::unique_ptr<PipelineState>> createMeshShaderRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const MeshShaderRasterPipelineCreateDesc &Desc) override;

  // Defined out-of-line below — needs VKRayTracingPipelineState's full type
  // and the device-loaded ray-tracing pipeline entry points.
  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineRT(llvm::StringRef Name, const BindingsDesc &BndDesc,
                   const RayTracingPipelineCreateDesc &Desc) override;

  llvm::Expected<std::unique_ptr<ShaderBindingTable>>
  createShaderBindingTable(const PipelineState &PSO,
                           const ShaderBindingTableDesc &Desc) override;
  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override;
  llvm::Expected<std::unique_ptr<offloadtest::MemoryHeap>>
  createMemoryHeap(std::string /*Name*/, size_t /*SizeInBytes*/) override;
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
  const Capabilities &getCapabilities() override;
  void printExtra(llvm::raw_ostream &OS) override;
  const VkPhysicalDeviceProperties &getProps() const;

private:
  void queryCapabilities();

public:
  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override;
  llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
  createRenderPass(const offloadtest::RenderPassDesc &Desc) override;

  // Helper: create a buffer with device address support.
  llvm::Expected<BufferRef>
  createBufferWithDeviceAddress(VkDeviceSize Size,
                                VkBufferUsageFlags ExtraUsage);
  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<TriangleGeometryDesc> Triangles) override;
  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) override;

private:
  AccelerationStructureSizes queryBLASPrebuildSize(
      llvm::ArrayRef<VkAccelerationStructureGeometryKHR> Geoms,
      llvm::ArrayRef<uint32_t> MaxPrimCounts);
  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  allocateAS(const AccelerationStructureSizes &Sizes,
             VkAccelerationStructureTypeKHR Type, const char *Kind);

public:
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
                              const PipelineState &PSO) override;
  llvm::Expected<BufferRef> createBuffer(VkBufferUsageFlags Usage,
                                         VkMemoryPropertyFlags MemoryFlags,
                                         size_t Size, void *Data = nullptr,
                                         VkMemoryAllocateFlags AllocFlags = 0);
  static llvm::Expected<VkSpecializationMapEntry>
  parseSpecializationConstant(const SpecializationConstant &SpecConst,
                              llvm::SmallVectorImpl<char> &SpecData);
  static llvm::Error parseSpecializationConstants(
      llvm::ArrayRef<SpecializationConstant> SpecializationConstants,
      llvm::SmallVectorImpl<VkSpecializationMapEntry> &SpecEntries,
      llvm::SmallVectorImpl<char> &SpecData, VkSpecializationInfo &SpecInfo);
  llvm::Error createCommands(Pipeline &P, SharedInvocationState &IS,
                             DescriptorPool &Pool);
  llvm::Error executeProgram(Pipeline &P) override;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_DEVICE_H
