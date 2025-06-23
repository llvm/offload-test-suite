//===- VX/Device.cpp - Vulkan Device API ----------------------------------===//
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
#include "Support/Pipeline.h"
#include "llvm/Support/Error.h"

#include <memory>
#include <vulkan/vulkan.h>

using namespace offloadtest;

#define VKFormats(FMT)                                                         \
  if (Channels == 1)                                                           \
    return VK_FORMAT_R32_##FMT;                                                \
  if (Channels == 2)                                                           \
    return VK_FORMAT_R32G32_##FMT;                                             \
  if (Channels == 3)                                                           \
    return VK_FORMAT_R32G32B32_##FMT;                                          \
  if (Channels == 4)                                                           \
    return VK_FORMAT_R32G32B32A32_##FMT;

static VkFormat getVKFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int32:
    VKFormats(SINT) break;
  case DataFormat::Float32:
    VKFormats(SFLOAT) break;
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return VK_FORMAT_UNDEFINED;
}

static VkDescriptorType getDescriptorType(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
  case ResourceKind::RWBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  case ResourceKind::ConstantBuffer:
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  }
  llvm_unreachable("All cases handled");
}

static VkBufferUsageFlagBits getFlagBits(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
    return VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
  case ResourceKind::RWBuffer:
    return VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  case ResourceKind::ConstantBuffer:
    return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  llvm_unreachable("All cases handled");
}

static bool isUniform(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
    return false;
  case ResourceKind::ConstantBuffer:
    return true;
  }
  llvm_unreachable("All cases handled");
}

static std::string getMessageSeverityString(
    VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity) {
  if (MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    return "Error";
  if (MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    return "Warning";
  if (MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    return "Info";
  if (MessageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    return "Verbose";
  return "Unknown";
}

static VkBool32
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT MessageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT MessageType,
              const VkDebugUtilsMessengerCallbackDataEXT *Data, void *) {
  // Only interested in messages from the validation layers.
  if (!(MessageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT))
    return VK_FALSE;

  llvm::dbgs() << "Validation " << getMessageSeverityString(MessageSeverity);
  llvm::dbgs() << ": [ " << Data->pMessageIdName << " ]\n";
  llvm::dbgs() << Data->pMessage;

  for (uint32_t I = 0; I < Data->objectCount; I++) {
    llvm::dbgs() << '\n';
    if (Data->pObjects[I].pObjectName) {
      llvm::dbgs() << "[" << Data->pObjects[I].pObjectName << "]";
    }
  }
  llvm::dbgs() << '\n';

  // Return true to true the validation error or warning into an error in the
  // vulkan API. This should causes tests to fail.
  const bool IsErrorOrWarning =
      MessageSeverity & (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT);
  if (IsErrorOrWarning)
    return VK_TRUE;

  // Continue to run even with VERBOSE and INFO messages.
  return VK_FALSE;
}

static VkDebugUtilsMessengerEXT registerDebugUtilCallback(VkInstance Instance) {
  VkDebugUtilsMessengerCreateInfoEXT CreateInfo = {};
  CreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  CreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  CreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  CreateInfo.pfnUserCallback = debugCallback;
  CreateInfo.pUserData = nullptr; // Optional
  auto Func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      Instance, "vkCreateDebugUtilsMessengerEXT");
  if (Func == nullptr)
    return VK_NULL_HANDLE;

  VkDebugUtilsMessengerEXT DebugMessenger;
  Func(Instance, &CreateInfo, nullptr, &DebugMessenger);
  return DebugMessenger;
}

namespace {

class VKDevice : public offloadtest::Device {
private:
  VkPhysicalDevice Device;
  VkPhysicalDeviceProperties Props;
  Capabilities Caps;
  using LayerVector = std::vector<VkLayerProperties>;
  LayerVector Layers;

  struct BufferRef {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
  };

  struct ResourceRef {
    BufferRef Host;
    BufferRef Device;
    uint64_t Size;
  };

  struct InvocationState {
    VkDevice Device;
    VkQueue Queue;
    VkCommandPool CmdPool;
    VkCommandBuffer CmdBuffer;
    VkPipelineLayout PipelineLayout;
    VkDescriptorPool Pool;
    VkPipelineCache PipelineCache;
    VkShaderModule Shader;
    VkPipeline Pipeline;

    llvm::SmallVector<VkDescriptorSetLayout> DescriptorSetLayouts;
    llvm::SmallVector<ResourceRef> Buffers;
    llvm::SmallVector<VkDescriptorSet> DescriptorSets;
    llvm::SmallVector<VkBufferView> BufferViews;
  };

public:
  VKDevice(VkPhysicalDevice D) : Device(D) {
    vkGetPhysicalDeviceProperties(Device, &Props);
    const uint64_t StrSz =
        strnlen(Props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    Description = std::string(Props.deviceName, StrSz);
  }
  VKDevice(const VKDevice &) = default;

  ~VKDevice() override = default;

  llvm::StringRef getAPIName() const override { return "Vulkan"; }
  GPUAPI getAPI() const override { return GPUAPI::Vulkan; }

  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      queryCapabilities();
    return Caps;
  }

  const LayerVector &getLayers() {
    if (Layers.empty())
      queryLayers();
    return Layers;
  }

  bool isLayerSupported(llvm::StringRef QueryName) {
    for (auto Layer : getLayers()) {
      if (Layer.layerName == QueryName)
        return true;
    }
    return false;
  }

  void printExtra(llvm::raw_ostream &OS) override {
    OS << "  Layers:\n";
    for (auto Layer : getLayers()) {
      uint64_t Sz = strnlen(Layer.layerName, VK_MAX_EXTENSION_NAME_SIZE);
      OS << "  - LayerName: " << llvm::StringRef(Layer.layerName, Sz) << "\n";
      OS << "    SpecVersion: " << Layer.specVersion << "\n";
      OS << "    ImplVersion: " << Layer.implementationVersion << "\n";
      Sz = strnlen(Layer.description, VK_MAX_DESCRIPTION_SIZE);
      OS << "    LayerDesc: " << llvm::StringRef(Layer.description, Sz) << "\n";
    }
  }

  const VkPhysicalDeviceProperties &getProps() const { return Props; }

private:
  void queryCapabilities() {

    VkPhysicalDeviceFeatures2 Features{};
    Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceVulkan11Features Features11{};
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    VkPhysicalDeviceVulkan12Features Features12{};
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceVulkan14Features Features14{};
    Features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;

    Features.pNext = &Features11;
    Features11.pNext = &Features12;
    Features12.pNext = &Features13;
    Features13.pNext = &Features14;
    Features14.pNext = NULL;
    vkGetPhysicalDeviceFeatures2(Device, &Features);

    Caps.insert(std::make_pair(
        "APIMajorVersion",
        make_capability<uint32_t>("APIMajorVersion",
                                  VK_API_VERSION_MAJOR(Props.apiVersion))));

    Caps.insert(std::make_pair(
        "APIMinorVersion",
        make_capability<uint32_t>("APIMinorVersion",
                                  VK_API_VERSION_MINOR(Props.apiVersion))));

#define VULKAN_FEATURE_BOOL(Name)                                              \
  Caps.insert(std::make_pair(                                                  \
      #Name, make_capability<bool>(#Name, Features.features.Name)));
#define VULKAN11_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features11.Name)));
#define VULKAN12_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features12.Name)));
#define VULKAN13_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features13.Name)));
#define VULKAN14_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features14.Name)));
#include "VKFeatures.def"
  }

  void queryLayers() {
    assert(Layers.empty() && "Should not be called twice!");
    uint32_t LayerCount;
    vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

    if (LayerCount == 0)
      return;

    Layers.insert(Layers.begin(), LayerCount, VkLayerProperties());
    vkEnumerateInstanceLayerProperties(&LayerCount, Layers.data());
  }

public:
  llvm::Error createDevice(InvocationState &IS) {

    // Find a queue that supports compute
    uint32_t QueueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueCount, 0);
    const std::unique_ptr<VkQueueFamilyProperties[]> QueueFamilyProps =
        std::unique_ptr<VkQueueFamilyProperties[]>(
            new VkQueueFamilyProperties[QueueCount]);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueCount,
                                             QueueFamilyProps.get());
    uint32_t QueueIdx = 0;
    for (; QueueIdx < QueueCount; ++QueueIdx)
      if (QueueFamilyProps.get()[QueueIdx].queueFlags & VK_QUEUE_COMPUTE_BIT)
        break;
    if (QueueIdx >= QueueCount)
      return llvm::createStringError(std::errc::no_such_device,
                                     "No compute queue found.");

    VkDeviceQueueCreateInfo QueueInfo = {};
    const float QueuePriority = 0.0f;
    QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex = QueueIdx;
    QueueInfo.queueCount = 1;
    QueueInfo.pQueuePriorities = &QueuePriority;

    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount = 1;
    DeviceInfo.pQueueCreateInfos = &QueueInfo;

    VkPhysicalDeviceFeatures2 Features{};
    Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceVulkan11Features Features11{};
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    VkPhysicalDeviceVulkan12Features Features12{};
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    VkPhysicalDeviceVulkan14Features Features14{};
    Features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;

    Features.pNext = &Features11;
    Features11.pNext = &Features12;
    Features12.pNext = &Features13;
    Features13.pNext = &Features14;
    Features14.pNext = NULL;
    vkGetPhysicalDeviceFeatures2(Device, &Features);

    Features.features.shaderStorageImageReadWithoutFormat = true;
    Features.features.shaderStorageImageWriteWithoutFormat = true;

    DeviceInfo.pEnabledFeatures = &Features.features;
    DeviceInfo.pNext = Features.pNext;

    if (vkCreateDevice(Device, &DeviceInfo, nullptr, &IS.Device))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Could not create Vulkan logical device.");
    vkGetDeviceQueue(IS.Device, QueueIdx, 0, &IS.Queue);

    VkCommandPoolCreateInfo CmdPoolInfo = {};
    CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolInfo.queueFamilyIndex = QueueIdx;
    CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(IS.Device, &CmdPoolInfo, nullptr, &IS.CmdPool))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not create command pool.");
    return llvm::Error::success();
  }

  llvm::Error createCommandBuffer(InvocationState &IS) {
    VkCommandBufferAllocateInfo CBufAllocInfo = {};
    CBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CBufAllocInfo.commandPool = IS.CmdPool;
    CBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CBufAllocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(IS.Device, &CBufAllocInfo, &IS.CmdBuffer))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not create command buffer.");
    VkCommandBufferBeginInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(IS.CmdBuffer, &BufferInfo))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not begin command buffer.");
    return llvm::Error::success();
  }

  llvm::Expected<BufferRef> createBuffer(InvocationState &IS,
                                         VkBufferUsageFlags Usage,
                                         VkMemoryPropertyFlags MemoryFlags,
                                         size_t Size, void *Data = nullptr) {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = Size;
    BufferInfo.usage = Usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(IS.Device, &BufferInfo, nullptr, &Buffer))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Could not create buffer.");

    VkMemoryRequirements MemReqs;
    vkGetBufferMemoryRequirements(IS.Device, Buffer, &MemReqs);
    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;

    VkPhysicalDeviceMemoryProperties MemProperties;
    vkGetPhysicalDeviceMemoryProperties(Device, &MemProperties);
    uint32_t MemIdx = 0;
    for (; MemIdx < MemProperties.memoryTypeCount;
         ++MemIdx, MemReqs.memoryTypeBits >>= 1) {
      if ((MemReqs.memoryTypeBits & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
          ((MemProperties.memoryTypes[MemIdx].propertyFlags & MemoryFlags) ==
           MemoryFlags)) {
        break;
      }
    }
    if (MemIdx >= MemProperties.memoryTypeCount)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Could not identify appropriate memory.");

    AllocInfo.memoryTypeIndex = MemIdx;

    if (vkAllocateMemory(IS.Device, &AllocInfo, nullptr, &Memory))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Memory allocation failed.");
    if (Data) {
      void *Dst = nullptr;
      if (vkMapMemory(IS.Device, Memory, 0, Size, 0, &Dst))
        return llvm::createStringError(std::errc::not_enough_memory,
                                       "Failed to map memory.");
      memcpy(Dst, Data, Size);

      VkMappedMemoryRange Range = {};
      Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      Range.memory = Memory;
      Range.offset = 0;
      Range.size = VK_WHOLE_SIZE;
      vkFlushMappedMemoryRanges(IS.Device, 1, &Range);

      vkUnmapMemory(IS.Device, Memory);
    }

    if (vkBindBufferMemory(IS.Device, Buffer, Memory, 0))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Failed to bind buffer to memory.");

    return BufferRef{Buffer, Memory};
  }

  llvm::Error createBuffer(Resource &R, InvocationState &IS) {
    auto ExHostBuf = createBuffer(
        IS, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, R.size(), R.BufferPtr->Data.get());
    if (!ExHostBuf)
      return ExHostBuf.takeError();

    auto ExDeviceBuf =
        createBuffer(IS,
                     getFlagBits(R.Kind) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, R.size());
    if (!ExDeviceBuf)
      return ExDeviceBuf.takeError();

    VkBufferCopy Copy = {};
    Copy.size = R.size();
    vkCmdCopyBuffer(IS.CmdBuffer, ExHostBuf->Buffer, ExDeviceBuf->Buffer, 1,
                    &Copy);

    IS.Buffers.push_back(ResourceRef{*ExHostBuf, *ExDeviceBuf, R.size()});

    return llvm::Error::success();
  }

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        if (auto Err = createBuffer(R, IS))
          return Err;
      }
    }
    return llvm::Error::success();
  }

  llvm::Error executeCommandBuffer(InvocationState &IS,
                                   VkPipelineStageFlags WaitMask = 0) {
    if (vkEndCommandBuffer(IS.CmdBuffer))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not end command buffer.");

    VkSubmitInfo SubmitInfo = {};
    SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    SubmitInfo.commandBufferCount = 1;
    SubmitInfo.pCommandBuffers = &IS.CmdBuffer;
    SubmitInfo.pWaitDstStageMask = &WaitMask;
    VkFenceCreateInfo FenceInfo = {};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence Fence;
    if (vkCreateFence(IS.Device, &FenceInfo, nullptr, &Fence))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Could not create fence.");

    // Submit to the queue
    if (vkQueueSubmit(IS.Queue, 1, &SubmitInfo, Fence))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to submit to queue.");
    if (vkWaitForFences(IS.Device, 1, &Fence, VK_TRUE, UINT64_MAX))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed waiting for fence.");

    vkDestroyFence(IS.Device, Fence, nullptr);
    vkFreeCommandBuffers(IS.Device, IS.CmdPool, 1, &IS.CmdBuffer);
    return llvm::Error::success();
  }

  llvm::Error createDescriptorPool(Pipeline &P, InvocationState &IS) {
    uint32_t TexelBufferCount = 0;
    uint32_t StorageBufferCount = 0;
    uint32_t UniformBufferCount = 0;
    for (const auto &S : P.Sets) {
      for (const auto &R : S.Resources) {
        if (isUniform(R.Kind))
          UniformBufferCount += 1;
        else if (R.isRaw())
          StorageBufferCount += 1;
        else
          TexelBufferCount += 1;
      }
    }
    assert(TexelBufferCount + StorageBufferCount + UniformBufferCount ==
               P.getDescriptorCount() &&
           "Mismatch in descriptor type identification.");
    llvm::SmallVector<VkDescriptorPoolSize> PoolSizes;
    if (TexelBufferCount > 0) {
      VkDescriptorPoolSize PoolSize = {};
      PoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
      PoolSize.descriptorCount = TexelBufferCount;
      PoolSizes.push_back(PoolSize);
    }

    if (StorageBufferCount > 0) {
      VkDescriptorPoolSize PoolSize = {};
      PoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      PoolSize.descriptorCount = StorageBufferCount;
      PoolSizes.push_back(PoolSize);
    }

    if (UniformBufferCount > 0) {
      VkDescriptorPoolSize PoolSize = {};
      PoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      PoolSize.descriptorCount = UniformBufferCount;
      PoolSizes.push_back(PoolSize);
    }

    VkDescriptorPoolCreateInfo PoolCreateInfo = {};
    PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    PoolCreateInfo.poolSizeCount = PoolSizes.size();
    PoolCreateInfo.pPoolSizes = PoolSizes.data();
    PoolCreateInfo.maxSets = P.Sets.size();
    if (vkCreateDescriptorPool(IS.Device, &PoolCreateInfo, nullptr, &IS.Pool))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create descriptor pool.");
    return llvm::Error::success();
  }

  llvm::Error createDescriptorSets(Pipeline &P, InvocationState &IS) {
    for (const auto &S : P.Sets) {
      std::vector<VkDescriptorSetLayoutBinding> Bindings;
      for (const auto &R : S.Resources) {
        VkDescriptorSetLayoutBinding Binding = {};
        if (!R.VKBinding.has_value())
          return llvm::createStringError(std::errc::invalid_argument,
                                         "No VulkanBinding provided for '%s'",
                                         R.Name.c_str());
        Binding.binding = R.VKBinding->Binding;
        Binding.descriptorType = getDescriptorType(R.Kind);
        Binding.descriptorCount = 1;
        Binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        Bindings.push_back(Binding);
      }
      VkDescriptorSetLayoutCreateInfo LayoutCreateInfo = {};
      LayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      LayoutCreateInfo.bindingCount = Bindings.size();
      LayoutCreateInfo.pBindings = Bindings.data();
      llvm::outs() << "Binding " << Bindings.size() << " descriptors.\n";
      VkDescriptorSetLayout Layout;
      if (vkCreateDescriptorSetLayout(IS.Device, &LayoutCreateInfo, nullptr,
                                      &Layout))
        return llvm::createStringError(
            std::errc::device_or_resource_busy,
            "Failed to create descriptor set layout.");
      IS.DescriptorSetLayouts.push_back(Layout);
    }

    VkPipelineLayoutCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineCreateInfo.setLayoutCount = IS.DescriptorSetLayouts.size();
    PipelineCreateInfo.pSetLayouts = IS.DescriptorSetLayouts.data();
    if (vkCreatePipelineLayout(IS.Device, &PipelineCreateInfo, nullptr,
                               &IS.PipelineLayout))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline layout.");

    VkDescriptorSetAllocateInfo DSAllocInfo = {};
    DSAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DSAllocInfo.descriptorPool = IS.Pool;
    DSAllocInfo.descriptorSetCount = IS.DescriptorSetLayouts.size();
    DSAllocInfo.pSetLayouts = IS.DescriptorSetLayouts.data();
    assert(IS.DescriptorSets.empty());
    IS.DescriptorSets.insert(IS.DescriptorSets.begin(),
                             IS.DescriptorSetLayouts.size(), VkDescriptorSet());
    llvm::outs() << "Num Descriptor sets: " << IS.DescriptorSetLayouts.size()
                 << "\n";
    if (vkAllocateDescriptorSets(IS.Device, &DSAllocInfo,
                                 IS.DescriptorSets.data()))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to allocate descriptor sets.");
    llvm::SmallVector<VkWriteDescriptorSet> WriteDescriptors;
    assert(IS.BufferViews.empty());
    llvm::SmallVector<VkDescriptorBufferInfo> RawBufferInfos;

    uint32_t BufIdx = 0;
    for (uint32_t SetIdx = 0; SetIdx < P.Sets.size(); ++SetIdx) {
      for (uint32_t RIdx = 0; RIdx < P.Sets[SetIdx].Resources.size();
           ++RIdx, ++BufIdx) {
        const Resource &R = P.Sets[SetIdx].Resources[RIdx];
        // This is a hack... need a better way to do this.
        VkBufferViewCreateInfo ViewCreateInfo = {};
        const bool IsRawOrUniform = R.isRaw();
        const VkFormat Format =
            IsRawOrUniform
                ? VK_FORMAT_UNDEFINED
                : getVKFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
        ViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        ViewCreateInfo.buffer = IS.Buffers[BufIdx].Device.Buffer;
        ViewCreateInfo.format = Format;
        ViewCreateInfo.range = VK_WHOLE_SIZE;
        if (IsRawOrUniform) {
          const VkDescriptorBufferInfo BI = {IS.Buffers[BufIdx].Device.Buffer,
                                             0, VK_WHOLE_SIZE};
          RawBufferInfos.push_back(BI);
        } else {
          IS.BufferViews.push_back(VkBufferView{0});
          if (vkCreateBufferView(IS.Device, &ViewCreateInfo, nullptr,
                                 &IS.BufferViews.back()))
            return llvm::createStringError(std::errc::device_or_resource_busy,
                                           "Failed to create buffer view.");
        }
        VkWriteDescriptorSet WDS = {};
        WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        WDS.dstSet = IS.DescriptorSets[SetIdx];
        WDS.dstBinding = RIdx;
        WDS.descriptorCount = 1;
        WDS.descriptorType = getDescriptorType(R.Kind);
        if (IsRawOrUniform)
          WDS.pBufferInfo = &RawBufferInfos.back();
        else
          WDS.pTexelBufferView = &IS.BufferViews.back();
        llvm::outs() << "Updating Descriptor [" << BufIdx << "] { " << SetIdx
                     << ", " << RIdx << " }\n";
        WriteDescriptors.push_back(WDS);
      }
    }
    llvm::outs() << "WriteDescriptors: " << WriteDescriptors.size() << "\n";
    vkUpdateDescriptorSets(IS.Device, WriteDescriptors.size(),
                           WriteDescriptors.data(), 0, nullptr);
    return llvm::Error::success();
  }

  llvm::Error createShaderModule(llvm::StringRef Program, InvocationState &IS) {
    VkShaderModuleCreateInfo ShaderCreateInfo = {};
    ShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderCreateInfo.codeSize = Program.size();
    ShaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(Program.data());
    if (vkCreateShaderModule(IS.Device, &ShaderCreateInfo, nullptr, &IS.Shader))
      return llvm::createStringError(std::errc::not_supported,
                                     "Failed to create shader module.");
    return llvm::Error::success();
  }

  llvm::Error createPipeline(Pipeline &P, InvocationState &IS) {
    VkPipelineCacheCreateInfo CacheCreateInfo = {};
    CacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (vkCreatePipelineCache(IS.Device, &CacheCreateInfo, nullptr,
                              &IS.PipelineCache))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline cache.");

    VkPipelineShaderStageCreateInfo StageInfo = {};
    StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    StageInfo.module = IS.Shader;
    StageInfo.pName = P.Shaders[0].Entry.c_str();

    VkComputePipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stage = StageInfo;
    PipelineCreateInfo.layout = IS.PipelineLayout;
    if (vkCreateComputePipelines(IS.Device, IS.PipelineCache, 1,
                                 &PipelineCreateInfo, nullptr, &IS.Pipeline))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline.");

    return llvm::Error::success();
  }

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    for (auto &UAV : IS.Buffers) {
      VkBufferMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.buffer = UAV.Device.Buffer;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      Barrier.dstAccessMask =
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           1, &Barrier, 0, nullptr);
    }
    vkCmdBindPipeline(IS.CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      IS.Pipeline);
    vkCmdBindDescriptorSets(IS.CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            IS.PipelineLayout, 0, IS.DescriptorSets.size(),
                            IS.DescriptorSets.data(), 0, 0);
    const llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
    vkCmdDispatch(IS.CmdBuffer, DispatchSize[0], DispatchSize[1],
                  DispatchSize[2]);

    for (auto &UAV : IS.Buffers) {
      VkBufferMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.buffer = UAV.Device.Buffer;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask =
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
      VkBufferCopy CopyRegion = {};
      CopyRegion.size = UAV.Size;
      vkCmdCopyBuffer(IS.CmdBuffer, UAV.Device.Buffer, UAV.Host.Buffer, 1,
                      &CopyRegion);

      Barrier.buffer = UAV.Host.Buffer;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
    return llvm::Error::success();
  }

  llvm::Error readBackData(Pipeline &P, InvocationState &IS) {
    uint32_t BufIdx = 0;
    for (auto &S : P.Sets) {
      for (int I = 0, E = S.Resources.size(); I < E; ++I, ++BufIdx) {
        const Resource &R = S.Resources[I];
        if (!R.isReadWrite())
          continue;
        void *Mapped = nullptr;
        vkMapMemory(IS.Device, IS.Buffers[BufIdx].Host.Memory, 0, VK_WHOLE_SIZE,
                    0, &Mapped);
        VkMappedMemoryRange Range = {};
        Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.memory = IS.Buffers[BufIdx].Host.Memory;
        Range.offset = 0;
        Range.size = VK_WHOLE_SIZE;
        vkInvalidateMappedMemoryRanges(IS.Device, 1, &Range);
        memcpy(R.BufferPtr->Data.get(), Mapped, R.size());
        vkUnmapMemory(IS.Device, IS.Buffers[BufIdx].Host.Memory);
      }
    }
    return llvm::Error::success();
  }

  llvm::Error cleanup(InvocationState &IS) {
    vkQueueWaitIdle(IS.Queue);
    for (auto &V : IS.BufferViews)
      vkDestroyBufferView(IS.Device, V, nullptr);

    for (auto &R : IS.Buffers) {
      vkDestroyBuffer(IS.Device, R.Device.Buffer, nullptr);
      vkFreeMemory(IS.Device, R.Device.Memory, nullptr);
      vkDestroyBuffer(IS.Device, R.Host.Buffer, nullptr);
      vkFreeMemory(IS.Device, R.Host.Memory, nullptr);
    }

    vkDestroyPipeline(IS.Device, IS.Pipeline, nullptr);

    vkDestroyShaderModule(IS.Device, IS.Shader, nullptr);

    vkDestroyPipelineCache(IS.Device, IS.PipelineCache, nullptr);

    vkDestroyPipelineLayout(IS.Device, IS.PipelineLayout, nullptr);

    for (auto &L : IS.DescriptorSetLayouts)
      vkDestroyDescriptorSetLayout(IS.Device, L, nullptr);

    vkDestroyDescriptorPool(IS.Device, IS.Pool, nullptr);

    vkDestroyCommandPool(IS.Device, IS.CmdPool, nullptr);
    vkDestroyDevice(IS.Device, nullptr);
    return llvm::Error::success();
  }

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState State;
    if (auto Err = createDevice(State))
      return Err;
    llvm::outs() << "Physical device created.\n";
    if (auto Err = createCommandBuffer(State))
      return Err;
    llvm::outs() << "Copy command buffer created.\n";
    if (auto Err = createBuffers(P, State))
      return Err;
    llvm::outs() << "Memory buffers created.\n";
    if (auto Err = executeCommandBuffer(State))
      return Err;
    llvm::outs() << "Executed copy command buffer.\n";
    if (auto Err = createCommandBuffer(State))
      return Err;
    llvm::outs() << "Execute command buffer created.\n";
    if (auto Err = createDescriptorPool(P, State))
      return Err;
    llvm::outs() << "Descriptor pool created.\n";
    if (auto Err = createDescriptorSets(P, State))
      return Err;
    llvm::outs() << "Descriptor sets created.\n";
    if (auto Err = createShaderModule(P.Shaders[0].Shader->getBuffer(), State))
      return Err;
    llvm::outs() << "Shader module created.\n";
    if (auto Err = createPipeline(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";
    if (auto Err = createComputeCommands(P, State))
      return Err;
    llvm::outs() << "Compute commands created.\n";
    if (auto Err = executeCommandBuffer(State, VK_PIPELINE_STAGE_TRANSFER_BIT))
      return Err;
    llvm::outs() << "Executed compute command buffer.\n";
    if (auto Err = readBackData(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";

    if (auto Err = cleanup(State))
      return Err;
    llvm::outs() << "Cleanup complete.\n";
    return llvm::Error::success();
  }
};

class VKContext {
private:
  VkInstance Instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
  llvm::SmallVector<std::shared_ptr<VKDevice>> Devices;

  VKContext() = default;
  ~VKContext() { cleanup(); }
  VKContext(const VKContext &) = delete;

public:
  static VKContext &instance() {
    static VKContext Ctx;
    return Ctx;
  }

  void cleanup() {
#ifndef NDEBUG
    auto Func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        Instance, "vkDestroyDebugUtilsMessengerEXT");
    if (Func != nullptr) {
      Func(Instance, DebugMessenger, nullptr);
    }
#endif
    vkDestroyInstance(Instance, NULL);
    Instance = VK_NULL_HANDLE;
  }

  llvm::Error initialize() {
    // Create a Vulkan 1.1 instance to determine the API version
    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = "OffloadTest";
    // TODO: We should set this based on a command line flag, and simplify the
    // code below to error if the requested version isn't supported.
    AppInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;

    VkResult Res = vkCreateInstance(&CreateInfo, NULL, &Instance);
    if (Res == VK_ERROR_INCOMPATIBLE_DRIVER)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Cannot find a compatible Vulkan device");
    if (Res)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Unknown Vulkan initialization error: %d",
                                     Res);

    uint32_t DeviceCount = 0;
    if (vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Failed to get device count");
    std::vector<VkPhysicalDevice> PhysicalDevicesTmp(DeviceCount);
    if (vkEnumeratePhysicalDevices(Instance, &DeviceCount,
                                   PhysicalDevicesTmp.data()))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Failed to enumerate devices");
    {
      auto TmpDev = std::make_shared<VKDevice>(PhysicalDevicesTmp[0]);
      AppInfo.apiVersion = TmpDev->getProps().apiVersion;
      llvm::dbgs() << "Using API version "
                   << VK_API_VERSION_MAJOR(AppInfo.apiVersion) << "."
                   << VK_API_VERSION_MINOR(AppInfo.apiVersion) << ".\n";
    }
    vkDestroyInstance(Instance, NULL);
    Instance = VK_NULL_HANDLE;

// TODO: This is a bit hacky but matches what I did in DX.
#ifndef NDEBUG
    const char *ValidationLayer = "VK_LAYER_KHRONOS_validation";
    CreateInfo.ppEnabledLayerNames = &ValidationLayer;
    CreateInfo.enabledLayerCount = 1;

    const char *DebugUtilsExtensionName = "VK_EXT_debug_utils";
    CreateInfo.ppEnabledExtensionNames = &DebugUtilsExtensionName;
    CreateInfo.enabledExtensionCount = 1;
#endif

    Res = vkCreateInstance(&CreateInfo, NULL, &Instance);
    if (Res == VK_ERROR_INCOMPATIBLE_DRIVER)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Cannot find a compatible Vulkan device");
    if (Res)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Unknown Vulkan initialization error %d",
                                     Res);

#ifndef NDEBUG
    DebugMessenger = registerDebugUtilCallback(Instance);
#endif

    DeviceCount = 0;
    if (vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Failed to get device count");
    std::vector<VkPhysicalDevice> PhysicalDevices(DeviceCount);
    if (vkEnumeratePhysicalDevices(Instance, &DeviceCount,
                                   PhysicalDevices.data()))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Failed to enumerate devices");
    for (const auto &Dev : PhysicalDevices) {
      auto NewDev = std::make_shared<VKDevice>(Dev);
      Devices.push_back(NewDev);
      Device::registerDevice(std::static_pointer_cast<Device>(NewDev));
    }

    return llvm::Error::success();
  }
};
} // namespace

llvm::Error Device::initializeVKDevices() {
  return VKContext::instance().initialize();
}

void Device::cleanupVKDevices() { VKContext::instance().cleanup(); }
