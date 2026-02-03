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
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/Error.h"

#include <limits>
#include <memory>
#include <numeric>
#include <system_error>
#include <vulkan/vulkan.h>

using namespace offloadtest;

// We use 64KB tile size because DX has a fixed tile size, and the offload test
// suite must work for all APIs.
constexpr uint32_t SparseBufferTileSize = 65536;

static uint32_t getNumTiles(std::optional<uint32_t> NumTiles, size_t Size) {
  if (NumTiles)
    return *NumTiles;
  return (Size + SparseBufferTileSize - 1) / SparseBufferTileSize;
}

#define VKFormats(FMT, BITS)                                                   \
  if (Channels == 1)                                                           \
    return VK_FORMAT_R##BITS##_##FMT;                                          \
  if (Channels == 2)                                                           \
    return VK_FORMAT_R##BITS##G##BITS##_##FMT;                                 \
  if (Channels == 3)                                                           \
    return VK_FORMAT_R##BITS##G##BITS##B##BITS##_##FMT;                        \
  if (Channels == 4)                                                           \
    return VK_FORMAT_R##BITS##G##BITS##B##BITS##A##BITS##_##FMT;

static VkFormat getVKFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int16:
    VKFormats(SINT, 16) break;
  case DataFormat::UInt16:
    VKFormats(UINT, 16) break;
  case DataFormat::Int32:
    VKFormats(SINT, 32) break;
  case DataFormat::UInt32:
    VKFormats(UINT, 32) break;
  case DataFormat::Float32:
    VKFormats(SFLOAT, 32) break;
  case DataFormat::Int64:
    VKFormats(SINT, 64) break;
  case DataFormat::UInt64:
    VKFormats(UINT, 64) break;
  case DataFormat::Float64:
    VKFormats(SFLOAT, 64) break;
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return VK_FORMAT_UNDEFINED;
}

static VkDescriptorType getDescriptorType(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
    return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
  case ResourceKind::RWBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
  case ResourceKind::Texture2D:
    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  case ResourceKind::RWTexture2D:
    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  case ResourceKind::ConstantBuffer:
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case ResourceKind::Sampler:
  case ResourceKind::SamplerComparison:
    return VK_DESCRIPTOR_TYPE_SAMPLER;
  }
  llvm_unreachable("All cases handled");
}

static VkFilter getVKFilter(FilterMode Mode) {
  switch (Mode) {
  case FilterMode::Nearest:
    return VK_FILTER_NEAREST;
  case FilterMode::Linear:
    return VK_FILTER_LINEAR;
  }
  llvm_unreachable("All filter cases handled");
}

static VkSamplerAddressMode getVKAddressMode(AddressMode Mode) {
  switch (Mode) {
  case AddressMode::Clamp:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case AddressMode::Repeat:
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  case AddressMode::Mirror:
    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  case AddressMode::Border:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  case AddressMode::MirrorOnce:
    return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
  }
  llvm_unreachable("All address mode cases handled");
}

static VkCompareOp getVKCompareOp(CompareFunction Func) {
  switch (Func) {
  case CompareFunction::Never:
    return VK_COMPARE_OP_NEVER;
  case CompareFunction::Less:
    return VK_COMPARE_OP_LESS;
  case CompareFunction::Equal:
    return VK_COMPARE_OP_EQUAL;
  case CompareFunction::LessEqual:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case CompareFunction::Greater:
    return VK_COMPARE_OP_GREATER;
  case CompareFunction::NotEqual:
    return VK_COMPARE_OP_NOT_EQUAL;
  case CompareFunction::GreaterEqual:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case CompareFunction::Always:
    return VK_COMPARE_OP_ALWAYS;
  }
  llvm_unreachable("All compare op cases handled");
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
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
  case ResourceKind::Sampler:
  case ResourceKind::SamplerComparison:
    llvm_unreachable("Textures and samplers don't have buffer usage bits!");
  }
  llvm_unreachable("All cases handled");
}

static VkImageViewType getImageViewType(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
    return VK_IMAGE_VIEW_TYPE_2D;
  case ResourceKind::Buffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::ConstantBuffer:
  case ResourceKind::Sampler:
  case ResourceKind::SamplerComparison:
    llvm_unreachable("Not an image view!");
  }
  llvm_unreachable("All cases handled");
}

static VkShaderStageFlagBits getShaderStageFlag(Stages Stage) {
  switch (Stage) {
  case Stages::Compute:
    return VK_SHADER_STAGE_COMPUTE_BIT;
  case Stages::Vertex:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case Stages::Pixel:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
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

  // Return true to turn the validation error or warning into an error in the
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

static llvm::Expected<uint32_t>
getMemoryIndex(VkPhysicalDevice Device, uint32_t MemoryTypeBits,
               VkMemoryPropertyFlags MemoryFlags) {
  VkPhysicalDeviceMemoryProperties MemProperties;
  vkGetPhysicalDeviceMemoryProperties(Device, &MemProperties);
  for (uint32_t I = 0; I < MemProperties.memoryTypeCount; ++I) {
    const uint32_t Bit = (1u << I);
    if ((MemoryTypeBits & Bit) == 0)
      continue;
    if ((MemProperties.memoryTypes[I].propertyFlags & MemoryFlags) ==
        MemoryFlags)
      return I;
  }
  return llvm::createStringError(std::errc::not_enough_memory,
                                 "Could not identify appropriate memory.");
}

namespace {

class VKDevice : public offloadtest::Device {
private:
  VkPhysicalDevice Device;
  VkPhysicalDeviceProperties Props;
  VkPhysicalDeviceProperties2 Props2;
  VkPhysicalDeviceDriverProperties DriverProps;
  Capabilities Caps;
  using LayerVector = std::vector<VkLayerProperties>;
  LayerVector Layers;
  using ExtensionVector = std::vector<VkExtensionProperties>;
  ExtensionVector Extensions;

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
    ResourceRef(BufferRef H, BufferRef D) : Host(H), Device(D) {}
    ResourceRef(BufferRef H, ImageRef I) : Host(H), Image(I) {}

    BufferRef Host;
    BufferRef Device;
    ImageRef Image;
  };

  struct ResourceBundle {
    ResourceBundle(VkDescriptorType DescriptorType, uint64_t Size,
                   Buffer *BufferPtr)
        : DescriptorType(DescriptorType), Size(Size), BufferPtr(BufferPtr) {}

    bool isImage() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
             DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }

    bool isSampler() const {
      return DescriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
             DescriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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
    Buffer *BufferPtr;
    VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    llvm::SmallVector<ResourceRef> ResourceRefs;
    llvm::SmallVector<ResourceRef> CounterResourceRefs;
  };

  struct CompiledShader {
    Stages Stage;
    std::string Entry;
    VkShaderModule Shader;
  };

  struct InvocationState {
    VkDevice Device;
    VkQueue Queue;
    VkQueue SparseQueue = VK_NULL_HANDLE;
    VkCommandPool CmdPool;
    VkCommandBuffer CmdBuffer;
    VkPipelineLayout PipelineLayout;
    VkDescriptorPool Pool = nullptr;
    VkPipelineCache PipelineCache;
    VkPipeline Pipeline;

    // FrameBuffer associated data for offscreen rendering.
    VkFramebuffer FrameBuffer;
    ResourceBundle FrameBufferResource = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0,
                                          nullptr};
    ImageRef DepthStencil = {0, 0, 0};
    std::optional<ResourceRef> VertexBuffer = std::nullopt;

    VkRenderPass RenderPass;
    uint32_t ShaderStageMask = 0;

    llvm::SmallVector<CompiledShader> Shaders;
    llvm::SmallVector<VkDescriptorSetLayout> DescriptorSetLayouts;
    llvm::SmallVector<ResourceBundle> Resources;
    llvm::SmallVector<VkDescriptorSet> DescriptorSets;
    llvm::SmallVector<VkBufferView> BufferViews;
    llvm::SmallVector<VkImageView> ImageViews;

    uint32_t getFullShaderStageMask() {
      if (0 != ShaderStageMask)
        return ShaderStageMask;
      for (const auto &S : Shaders)
        ShaderStageMask |= getShaderStageFlag(S.Stage);
      return ShaderStageMask;
    }
  };

public:
  VKDevice(VkPhysicalDevice D) : Device(D) {
    vkGetPhysicalDeviceProperties(Device, &Props);
    const uint64_t DeviceNameSz =
        strnlen(Props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    Description = std::string(Props.deviceName, DeviceNameSz);
    DriverProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    DriverProps.pNext = nullptr;
    Props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    Props2.pNext = &DriverProps;
    vkGetPhysicalDeviceProperties2(Device, &Props2);
    const uint64_t DriverNameSz =
        strnlen(DriverProps.driverName, VK_MAX_DRIVER_NAME_SIZE);
    DriverName = std::string(DriverProps.driverName, DriverNameSz);
#if defined(__APPLE__) && defined(__aarch64__)
    // Apple silicon Macs may have multiple Vulkan drivers sharing one device
    // name. Include the driver name in the description to enable
    // adapter-regex matching.
    Description += " (" + DriverName + ")";
#endif
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

  const ExtensionVector &getExtensions() {
    if (Extensions.empty())
      queryExtensions();
    return Extensions;
  }

  bool isExtensionSupported(llvm::StringRef QueryName) {
    for (const auto &Ext : getExtensions()) {
      if (Ext.extensionName == QueryName)
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

    OS << "  Extensions:\n";
    for (const auto &Ext : getExtensions()) {
      OS << "  - ExtensionName: " << llvm::StringRef(Ext.extensionName) << "\n";
      OS << "    SpecVersion: " << Ext.specVersion << "\n";
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
#ifdef VK_VERSION_1_4
    VkPhysicalDeviceVulkan14Features Features14{};
    Features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
#endif

    Features.pNext = &Features11;
    if (Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0))
      Features11.pNext = &Features12;
    if (Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0))
      Features12.pNext = &Features13;
#ifdef VK_VERSION_1_4
    if (Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0))
      Features13.pNext = &Features14;
#endif
    vkGetPhysicalDeviceFeatures2(Device, &Features);

    Caps.insert(std::make_pair(
        "APIMajorVersion",
        make_capability<uint32_t>("APIMajorVersion",
                                  VK_API_VERSION_MAJOR(Props.apiVersion))));

    Caps.insert(std::make_pair(
        "APIMinorVersion",
        make_capability<uint32_t>("APIMinorVersion",
                                  VK_API_VERSION_MINOR(Props.apiVersion))));

    Caps.insert(std::make_pair(
        "residencyStandard2DBlockShape",
        make_capability<bool>(
            "residencyStandard2DBlockShape",
            Props.sparseProperties.residencyStandard2DBlockShape)));
    Caps.insert(std::make_pair(
        "residencyStandard2DMultisampleBlockShape",
        make_capability<bool>(
            "residencyStandard2DMultisampleBlockShape",
            Props.sparseProperties.residencyStandard2DMultisampleBlockShape)));
    Caps.insert(std::make_pair(
        "residencyStandard3DBlockShape",
        make_capability<bool>(
            "residencyStandard3DBlockShape",
            Props.sparseProperties.residencyStandard3DBlockShape)));
    Caps.insert(std::make_pair(
        "residencyAlignedMipSize",
        make_capability<bool>("residencyAlignedMipSize",
                              Props.sparseProperties.residencyAlignedMipSize)));
    Caps.insert(
        std::make_pair("residencyNonResidentStrict",
                       make_capability<bool>(
                           "residencyNonResidentStrict",
                           Props.sparseProperties.residencyNonResidentStrict)));

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
#ifdef VK_VERSION_1_4
#define VULKAN14_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, make_capability<bool>(#Name, Features14.Name)));
#endif
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

  void queryExtensions() {
    assert(Extensions.empty() && "Should not be called twice!");
    uint32_t ExtCount;
    vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtCount, nullptr);

    if (ExtCount == 0)
      return;

    Extensions.insert(Extensions.begin(), ExtCount, VkExtensionProperties());
    vkEnumerateDeviceExtensionProperties(Device, nullptr, &ExtCount,
                                         Extensions.data());
  }

  static constexpr uint32_t InvalidQueueIndex =
      std::numeric_limits<uint32_t>::max();

  uint32_t findQueue(const VkQueueFamilyProperties *Props, uint32_t Count,
                     VkQueueFlags Flags) {
    for (uint32_t I = 0; I < Count; ++I) {
      if ((Props[I].queueFlags & Flags) == Flags)
        return I;
    }
    return InvalidQueueIndex;
  }

public:
  llvm::Error createDevice(InvocationState &IS) {

    // Find a queue family that supports both graphics and compute.
    uint32_t QueueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueCount, nullptr);
    if (QueueCount == 0)
      return llvm::createStringError(std::errc::no_such_device,
                                     "No queue families reported.");

    const std::unique_ptr<VkQueueFamilyProperties[]> QueueFamilyProps(
        new VkQueueFamilyProperties[QueueCount]);
    vkGetPhysicalDeviceQueueFamilyProperties(Device, &QueueCount,
                                             QueueFamilyProps.get());

    uint32_t MainQueueIdx =
        findQueue(QueueFamilyProps.get(), QueueCount,
                  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                      VK_QUEUE_SPARSE_BINDING_BIT);
    uint32_t SparseQueueIdx = MainQueueIdx;

    // If not found, find separate queues
    if (MainQueueIdx == InvalidQueueIndex) {
      MainQueueIdx = findQueue(QueueFamilyProps.get(), QueueCount,
                               VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
      SparseQueueIdx = findQueue(QueueFamilyProps.get(), QueueCount,
                                 VK_QUEUE_SPARSE_BINDING_BIT);
    }

    if (MainQueueIdx == InvalidQueueIndex)
      return llvm::createStringError(
          std::errc::no_such_device,
          "No suitable queue family found for graphics and compute.");

    if (SparseQueueIdx == InvalidQueueIndex)
      return llvm::createStringError(
          std::errc::no_such_device,
          "No suitable queue family found for sparse "
          "binding.");

    const float QueuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;

    auto AddQueueCreateInfo = [&](uint32_t QFamilyIndex) {
      VkDeviceQueueCreateInfo QueueCreateInfo = {};
      QueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      QueueCreateInfo.queueFamilyIndex = QFamilyIndex;
      QueueCreateInfo.queueCount = 1;
      QueueCreateInfo.pQueuePriorities = &QueuePriority;
      QueueCreateInfos.push_back(QueueCreateInfo);
    };

    AddQueueCreateInfo(MainQueueIdx);
    if (MainQueueIdx != SparseQueueIdx)
      AddQueueCreateInfo(SparseQueueIdx);

    VkDeviceCreateInfo DeviceInfo = {};
    DeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    DeviceInfo.queueCreateInfoCount =
        static_cast<uint32_t>(QueueCreateInfos.size());
    DeviceInfo.pQueueCreateInfos = QueueCreateInfos.data();

    VkPhysicalDeviceFeatures2 Features{};
    Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    VkPhysicalDeviceVulkan11Features Features11{};
    Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    VkPhysicalDeviceVulkan12Features Features12{};
    Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    VkPhysicalDeviceVulkan13Features Features13{};
    Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
#ifdef VK_VERSION_1_4
    VkPhysicalDeviceVulkan14Features Features14{};
    Features14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
#endif

    Features.pNext = &Features11;
    if (Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0))
      Features11.pNext = &Features12;
    if (Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0))
      Features12.pNext = &Features13;
#ifdef VK_VERSION_1_4
    if (Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0))
      Features13.pNext = &Features14;
#endif
    vkGetPhysicalDeviceFeatures2(Device, &Features);

    DeviceInfo.pEnabledFeatures = &Features.features;
    DeviceInfo.pNext = Features.pNext;

    if (vkCreateDevice(Device, &DeviceInfo, nullptr, &IS.Device))
      return llvm::createStringError(std::errc::no_such_device,
                                     "Could not create Vulkan logical device.");
    vkGetDeviceQueue(IS.Device, MainQueueIdx, 0, &IS.Queue);
    vkGetDeviceQueue(IS.Device, SparseQueueIdx, 0, &IS.SparseQueue);

    VkCommandPoolCreateInfo CmdPoolInfo = {};
    CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolInfo.queueFamilyIndex = MainQueueIdx;
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

  llvm::Expected<VkBuffer> createVkBuffer(VkDevice Device, size_t Size,
                                          VkBufferUsageFlags Usage,
                                          VkBufferCreateFlags Flags = 0) {
    VkBuffer Buffer;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = Size;
    BufferInfo.usage = Usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    BufferInfo.flags = Flags;

    if (vkCreateBuffer(Device, &BufferInfo, nullptr, &Buffer))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Could not create buffer.");
    return Buffer;
  }

  llvm::Expected<VkDeviceMemory>
  allocateMemory(VkDevice Device, VkDeviceSize Size, uint32_t MemoryTypeIndex) {
    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = Size;
    AllocInfo.memoryTypeIndex = MemoryTypeIndex;
    VkDeviceMemory Memory;
    if (vkAllocateMemory(Device, &AllocInfo, nullptr, &Memory))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Memory allocation failed.");
    return Memory;
  }

  llvm::Expected<BufferRef> createBuffer(InvocationState &IS,
                                         VkBufferUsageFlags Usage,
                                         VkMemoryPropertyFlags MemoryFlags,
                                         size_t Size, void *Data = nullptr) {
    auto ExBuffer = createVkBuffer(IS.Device, Size, Usage);
    if (!ExBuffer)
      return ExBuffer.takeError();
    VkBuffer Buffer = *ExBuffer;

    VkMemoryRequirements MemReqs;
    vkGetBufferMemoryRequirements(IS.Device, Buffer, &MemReqs);

    llvm::Expected<uint32_t> MemIdx =
        getMemoryIndex(Device, MemReqs.memoryTypeBits, MemoryFlags);
    if (!MemIdx)
      return MemIdx.takeError();

    auto ExMemory = allocateMemory(IS.Device, MemReqs.size, *MemIdx);
    if (!ExMemory)
      return ExMemory.takeError();
    VkDeviceMemory Memory = *ExMemory;

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

  llvm::Expected<ResourceRef> createImage(InvocationState &IS, Resource &R,
                                          BufferRef &Host,
                                          int UsageOverride = 0) {
    const offloadtest::Buffer &B = *R.BufferPtr;
    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.format = getVKFormat(B.Format, B.Channels);
    ImageCreateInfo.mipLevels = 1;
    ImageCreateInfo.arrayLayers = 1;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Set initial layout of the image to undefined
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.extent = {static_cast<uint32_t>(B.OutputProps.Width),
                              static_cast<uint32_t>(B.OutputProps.Height), 1};
    if (UsageOverride == 0) {
      ImageCreateInfo.usage =
          VK_IMAGE_USAGE_TRANSFER_DST_BIT |
          (R.isReadWrite()
               ? (VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
               : VK_IMAGE_USAGE_SAMPLED_BIT);
    } else {
      ImageCreateInfo.usage = UsageOverride;
    }

    VkImage Image;
    if (vkCreateImage(IS.Device, &ImageCreateInfo, nullptr, &Image))
      return llvm::createStringError(std::errc::io_error,
                                     "Failed to create image.");

    VkSampler Sampler = 0;

    VkMemoryRequirements MemReqs;
    vkGetImageMemoryRequirements(IS.Device, Image, &MemReqs);
    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;

    VkDeviceMemory Memory;
    if (vkAllocateMemory(IS.Device, &AllocInfo, nullptr, &Memory))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Image memory allocation failed.");
    if (vkBindImageMemory(IS.Device, Image, Memory, 0))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Image memory binding failed.");

    return ResourceRef(Host, ImageRef{Image, Sampler, Memory});
  }

  llvm::Expected<ResourceRef> createSampler(InvocationState &IS, Resource &R,
                                            BufferRef &Host) {
    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    const Sampler &S = *R.SamplerPtr;
    SamplerInfo.magFilter = getVKFilter(S.MagFilter);
    SamplerInfo.minFilter = getVKFilter(S.MinFilter);
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.addressModeU = getVKAddressMode(S.Address);
    SamplerInfo.addressModeV = getVKAddressMode(S.Address);
    SamplerInfo.addressModeW = getVKAddressMode(S.Address);
    SamplerInfo.mipLodBias = S.MipLODBias;
    SamplerInfo.anisotropyEnable = VK_FALSE;
    SamplerInfo.maxAnisotropy = 1.0f;
    SamplerInfo.compareEnable =
        R.Kind == ResourceKind::SamplerComparison ? VK_TRUE : VK_FALSE;
    SamplerInfo.compareOp = getVKCompareOp(S.ComparisonOp);
    SamplerInfo.minLod = S.MinLOD;
    SamplerInfo.maxLod = S.MaxLOD;
    SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;

    VkSampler Sampler;
    if (vkCreateSampler(IS.Device, &SamplerInfo, nullptr, &Sampler))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create sampler.");

    return ResourceRef(Host, ImageRef{0, Sampler, 0});
  }

  VkDeviceSize getCopySize(InvocationState &IS, Resource &R, VkBuffer Buffer) {
    VkDeviceSize CopySize = R.size();
    if (R.IsReserved) {
      VkDeviceSize MappedSize =
          static_cast<VkDeviceSize>(getNumTiles(R.TilesMapped, R.size())) *
          SparseBufferTileSize;
      if (CopySize > MappedSize)
        CopySize = MappedSize;
    }
    return CopySize;
  }

  llvm::Error createResource(Resource &R, InvocationState &IS) {
    // Samplers don't have backing data buffers, so handle them separately
    if (R.isSampler()) {
      ResourceBundle Bundle{getDescriptorType(R.Kind), 0, nullptr};
      BufferRef HostBuf = {0, 0};
      auto ExSamplerRef = createSampler(IS, R, HostBuf);
      if (!ExSamplerRef)
        return ExSamplerRef.takeError();
      Bundle.ResourceRefs.push_back(*ExSamplerRef);
      IS.Resources.push_back(Bundle);
      return llvm::Error::success();
    }

    if (!R.BufferPtr)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Resource '%s' has no backing buffer.",
                                     R.Name.c_str());

    ResourceBundle Bundle{getDescriptorType(R.Kind), R.size(), R.BufferPtr};
    for (auto &ResData : R.BufferPtr->Data) {
      auto ExHostBuf = createBuffer(
          IS,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, R.size(), ResData.get());
      if (!ExHostBuf)
        return ExHostBuf.takeError();

      if (R.isTexture()) {
        auto ExImageRef = createImage(IS, R, *ExHostBuf);
        if (!ExImageRef)
          return ExImageRef.takeError();
        Bundle.ResourceRefs.push_back(*ExImageRef);
      } else {
        llvm::Expected<BufferRef> ExDeviceBuf =
            R.IsReserved
                ? createSparseBuffer(
                      IS,
                      getFlagBits(R.Kind) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, R.size(),
                      getNumTiles(R.TilesMapped, R.size()))
                : createBuffer(IS,
                               getFlagBits(R.Kind) |
                                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, R.size());

        if (!ExDeviceBuf)
          return ExDeviceBuf.takeError();

        VkBufferCopy Copy = {};
        Copy.size = getCopySize(IS, R, ExDeviceBuf->Buffer);
        vkCmdCopyBuffer(IS.CmdBuffer, ExHostBuf->Buffer, ExDeviceBuf->Buffer, 1,
                        &Copy);
        Bundle.ResourceRefs.emplace_back(*ExHostBuf, *ExDeviceBuf);
      }
    }
    if (R.HasCounter) {
      for (uint32_t I = 0; I < R.getArraySize(); ++I) {
        uint32_t CounterValue = 0;
        auto ExHostBuf = createBuffer(IS,
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      sizeof(uint32_t), &CounterValue);
        if (!ExHostBuf)
          return ExHostBuf.takeError();

        auto ExDeviceBuf = createBuffer(
            IS,
            getFlagBits(R.Kind) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint32_t));
        if (!ExDeviceBuf)
          return ExDeviceBuf.takeError();
        VkBufferCopy Copy = {};
        Copy.size = sizeof(uint32_t);
        vkCmdCopyBuffer(IS.CmdBuffer, ExHostBuf->Buffer, ExDeviceBuf->Buffer, 1,
                        &Copy);
        Bundle.CounterResourceRefs.emplace_back(*ExHostBuf, *ExDeviceBuf);
      }
    }
    IS.Resources.push_back(Bundle);
    return llvm::Error::success();
  }

  llvm::Expected<BufferRef>
  createSparseBuffer(InvocationState &IS, VkBufferUsageFlags Usage,
                     VkMemoryPropertyFlags MemoryFlags, size_t Size,
                     uint32_t TilesMapped) {
    auto ExBuffer = createVkBuffer(IS.Device, Size, Usage,
                                   VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
                                       VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT);
    if (!ExBuffer)
      return ExBuffer.takeError();
    VkBuffer Buffer = *ExBuffer;

    VkMemoryRequirements MemReqs;
    vkGetBufferMemoryRequirements(IS.Device, Buffer, &MemReqs);

    if (SparseBufferTileSize % MemReqs.alignment != 0)
      return llvm::createStringError(
          std::errc::not_supported,
          "Sparse buffer alignment must be a factor of 64KB.");

    // Calculate size for the mapped region (TilesMapped * SparseBufferTileSize)
    VkDeviceSize MappedSize = TilesMapped * SparseBufferTileSize;

    if (MappedSize > MemReqs.size)
      MappedSize = MemReqs.size;

    llvm::Expected<uint32_t> MemIdx =
        getMemoryIndex(Device, MemReqs.memoryTypeBits, MemoryFlags);
    if (!MemIdx)
      return MemIdx.takeError();

    auto ExMemory = allocateMemory(IS.Device, MappedSize, *MemIdx);
    if (!ExMemory)
      return ExMemory.takeError();
    VkDeviceMemory Memory = *ExMemory;

    // Bind the allocated memory to the start of the buffer
    VkSparseMemoryBind Bind = {};
    Bind.resourceOffset = 0;
    Bind.size = MappedSize;
    Bind.memory = Memory;
    Bind.memoryOffset = 0;
    Bind.flags = 0;

    VkSparseBufferMemoryBindInfo BufferBindInfo = {};
    BufferBindInfo.buffer = Buffer;
    BufferBindInfo.bindCount = 1;
    BufferBindInfo.pBinds = &Bind;

    VkBindSparseInfo BindInfo = {};
    BindInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;
    BindInfo.bufferBindCount = 1;
    BindInfo.pBufferBinds = &BufferBindInfo;

    // Use a fence to ensure binding is complete before use, though for simple
    // cases strict ordering might suffice if on same queue. Ideally we should
    // wait, but here we just submit. IS.Queue is used for commands. Note:
    // vkQueueBindSparse requires the queue to support SPARSI_BINDING. We assume
    // the main queue supports it.
    VkFence Fence;
    VkFenceCreateInfo FenceInfo = {};
    FenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (vkCreateFence(IS.Device, &FenceInfo, nullptr, &Fence))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create fence for sparse bind");

    if (vkQueueBindSparse(IS.SparseQueue, 1, &BindInfo, Fence) != VK_SUCCESS)
      return llvm::createStringError(std::errc::io_error,
                                     "vkQueueBindSparse failed");

    if (vkWaitForFences(IS.Device, 1, &Fence, VK_TRUE, UINT64_MAX) !=
        VK_SUCCESS)
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to wait for sparse bind fence");

    vkDestroyFence(IS.Device, Fence, nullptr);

    return BufferRef{Buffer, Memory};
  }

  llvm::Error createDepthStencil(Pipeline &P, InvocationState &IS) {
    // Create an optimal image used as the depth stencil attachment
    VkImageCreateInfo ImageCi = {};
    ImageCi.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCi.imageType = VK_IMAGE_TYPE_2D;
    ImageCi.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    // Use example's height and width
    ImageCi.extent = {
        static_cast<uint32_t>(P.Bindings.RTargetBufferPtr->OutputProps.Width),
        static_cast<uint32_t>(P.Bindings.RTargetBufferPtr->OutputProps.Height),
        1};
    ImageCi.mipLevels = 1;
    ImageCi.arrayLayers = 1;
    ImageCi.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageCi.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageCi.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ImageCi.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (vkCreateImage(IS.Device, &ImageCi, nullptr, &IS.DepthStencil.Image))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Depth stencil creation failed.");

    // Allocate memory for the image (device local) and bind it to our image
    VkMemoryAllocateInfo MemAlloc{};
    MemAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements MemReqs;
    vkGetImageMemoryRequirements(IS.Device, IS.DepthStencil.Image, &MemReqs);
    MemAlloc.allocationSize = MemReqs.size;
    llvm::Expected<uint32_t> MemIdx = getMemoryIndex(
        Device, MemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!MemIdx)
      return MemIdx.takeError();

    MemAlloc.memoryTypeIndex = *MemIdx;
    if (vkAllocateMemory(IS.Device, &MemAlloc, nullptr,
                         &IS.DepthStencil.Memory))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Depth stencil memory allocation failed.");
    if (vkBindImageMemory(IS.Device, IS.DepthStencil.Image,
                          IS.DepthStencil.Memory, 0))
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Depth stencil memory binding failed.");
    return llvm::Error::success();
  }

  llvm::Error createResources(Pipeline &P, InvocationState &IS) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        if (auto Err = createResource(R, IS))
          return Err;
      }
    }

    if (P.isGraphics()) {
      if (!P.Bindings.RTargetBufferPtr)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "No RenderTarget buffer specified for graphics pipeline.");
      Resource FrameBuffer = {ResourceKind::Texture2D,
                              "RenderTarget",
                              {},
                              {},
                              P.Bindings.RTargetBufferPtr,
                              nullptr,
                              false,
                              std::nullopt,
                              false};
      IS.FrameBufferResource.Size = P.Bindings.RTargetBufferPtr->size();
      IS.FrameBufferResource.BufferPtr = P.Bindings.RTargetBufferPtr;
      IS.FrameBufferResource.ImageLayout =
          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      auto ExHostBuf = createBuffer(
          IS,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, FrameBuffer.size(),
          FrameBuffer.BufferPtr->Data[0].get());
      if (!ExHostBuf)
        return ExHostBuf.takeError();
      auto ExImageRef = createImage(IS, FrameBuffer, *ExHostBuf,
                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                        VK_IMAGE_USAGE_SAMPLED_BIT |
                                        VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
      if (!ExImageRef)
        return ExImageRef.takeError();
      IS.FrameBufferResource.ResourceRefs.push_back(*ExImageRef);
      if (auto Err = createDepthStencil(P, IS))
        return Err;

      if (P.Bindings.VertexBufferPtr == nullptr)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "No Vertex buffer specified for graphics pipeline.");
      const Resource VertexBuffer = {ResourceKind::StructuredBuffer,
                                     "VertexBuffer",
                                     {},
                                     {},
                                     P.Bindings.VertexBufferPtr,
                                     nullptr,
                                     false,
                                     std::nullopt,
                                     false};
      auto ExVHostBuf =
          createBuffer(IS, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VertexBuffer.size(),
                       VertexBuffer.BufferPtr->Data[0].get());
      if (!ExVHostBuf)
        return ExVHostBuf.takeError();
      auto ExDeviceBuf = createBuffer(
          IS,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer.size());
      if (!ExDeviceBuf)
        return ExDeviceBuf.takeError();
      VkBufferCopy Copy = {};
      Copy.size = VertexBuffer.size();
      vkCmdCopyBuffer(IS.CmdBuffer, ExVHostBuf->Buffer, ExDeviceBuf->Buffer, 1,
                      &Copy);
      IS.VertexBuffer = ResourceRef(*ExVHostBuf, *ExDeviceBuf);
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
                                     "Failed to submit command buffer.");
    if (vkWaitForFences(IS.Device, 1, &Fence, VK_TRUE, UINT64_MAX))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed waiting for fence.");

    vkDestroyFence(IS.Device, Fence, nullptr);
    vkFreeCommandBuffers(IS.Device, IS.CmdPool, 1, &IS.CmdBuffer);
    return llvm::Error::success();
  }

  llvm::Error createDescriptorPool(Pipeline &P, InvocationState &IS) {

    constexpr VkDescriptorType DescriptorTypes[] = {
        VK_DESCRIPTOR_TYPE_SAMPLER,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER};
    constexpr size_t DescriptorTypesSize =
        sizeof(DescriptorTypes) / sizeof(VkDescriptorType);
    uint32_t DescriptorCounts[DescriptorTypesSize] = {0};
    for (const auto &S : P.Sets) {
      for (const auto &R : S.Resources) {
        DescriptorCounts[getDescriptorType(R.Kind)] += R.getArraySize();
        if (R.HasCounter)
          DescriptorCounts[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER] +=
              R.getArraySize();
      }
    }
    llvm::SmallVector<VkDescriptorPoolSize> PoolSizes;
    for (const VkDescriptorType Type : DescriptorTypes) {
      if (DescriptorCounts[Type] > 0) {
        llvm::outs() << "Descriptors: { type = " << Type
                     << ", count = " << DescriptorCounts[Type] << " }\n";
        VkDescriptorPoolSize PoolSize = {};
        PoolSize.type = Type;
        PoolSize.descriptorCount = DescriptorCounts[Type];
        PoolSizes.push_back(PoolSize);
      }
    }

    if (P.Sets.size() > 0) {
      VkDescriptorPoolCreateInfo PoolCreateInfo = {};
      PoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      PoolCreateInfo.poolSizeCount = PoolSizes.size();
      PoolCreateInfo.pPoolSizes = PoolSizes.data();
      PoolCreateInfo.maxSets = P.Sets.size();
      if (vkCreateDescriptorPool(IS.Device, &PoolCreateInfo, nullptr, &IS.Pool))
        return llvm::createStringError(std::errc::device_or_resource_busy,
                                       "Failed to create descriptor pool.");
    }
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
        if (R.HasCounter && !R.VKBinding->CounterBinding)
          return llvm::createStringError(
              std::errc::invalid_argument,
              "No CounterBinding provided for resource '%s' with a counter",
              R.Name.c_str());
        Binding.binding = R.VKBinding->Binding;
        Binding.descriptorType = getDescriptorType(R.Kind);
        Binding.descriptorCount = R.getArraySize();
        Binding.stageFlags = IS.getFullShaderStageMask();
        Bindings.push_back(Binding);
        if (R.HasCounter) {
          VkDescriptorSetLayoutBinding CounterBinding = {};
          CounterBinding.binding = *R.VKBinding->CounterBinding;
          CounterBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          CounterBinding.descriptorCount = R.getArraySize();
          CounterBinding.stageFlags = IS.getFullShaderStageMask();
          Bindings.push_back(CounterBinding);
        }
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

    llvm::SmallVector<VkPushConstantRange, 1> Ranges;
    for (const auto &PCB : P.PushConstants) {
      const VkPushConstantRange R = {
          static_cast<VkShaderStageFlags>(getShaderStageFlag(PCB.Stage)),
          /* offset= */ 0, static_cast<uint32_t>(PCB.size())};
      Ranges.emplace_back(std::move(R));
    }
    PipelineCreateInfo.pushConstantRangeCount = Ranges.size();
    PipelineCreateInfo.pPushConstantRanges = Ranges.data();

    if (vkCreatePipelineLayout(IS.Device, &PipelineCreateInfo, nullptr,
                               &IS.PipelineLayout))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline layout.");

    if (P.Sets.size() == 0)
      return llvm::Error::success();

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

    // Calculate the number of infos/views we are going to need for each type
    uint32_t ImageInfoCount = 0;
    uint32_t BufferInfoCount = 0;
    uint32_t BufferViewCount = 0;
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        if (R.isSampler()) {
          ImageInfoCount += 1;
          continue;
        }
        const uint32_t Count = R.getArraySize();
        if (R.isTexture())
          ImageInfoCount += Count;
        else if (R.isRaw())
          BufferInfoCount += Count;
        else
          BufferViewCount += Count;
        if (R.HasCounter)
          BufferInfoCount += Count;
      }
    }

    // reserve enough space for the descriptor infos so it never needs to be
    // resized (we need the memory fixed in place)
    llvm::SmallVector<VkDescriptorImageInfo> ImageInfos;
    llvm::SmallVector<VkDescriptorBufferInfo> BufferInfos;
    llvm::SmallVector<VkBufferView> BufferViews;
    ImageInfos.reserve(ImageInfoCount);
    BufferInfos.reserve(BufferInfoCount);
    BufferViews.reserve(BufferViewCount);

    llvm::SmallVector<VkWriteDescriptorSet> WriteDescriptors;
    WriteDescriptors.reserve(ImageInfoCount + BufferInfoCount +
                             BufferViewCount);
    assert(IS.BufferViews.empty());

    uint32_t OverallResIdx = 0;
    for (uint32_t SetIdx = 0; SetIdx < P.Sets.size(); ++SetIdx) {
      for (uint32_t RIdx = 0; RIdx < P.Sets[SetIdx].Resources.size();
           ++RIdx, ++OverallResIdx) {
        const Resource &R = P.Sets[SetIdx].Resources[RIdx];
        uint32_t IndexOfFirstBufferDataInArray;
        if (R.isSampler()) {
          IndexOfFirstBufferDataInArray = ImageInfos.size();
          for (auto &ResRef : IS.Resources[OverallResIdx].ResourceRefs) {
            const VkDescriptorImageInfo ImageInfo = {ResRef.Image.Sampler, 0,
                                                     VK_IMAGE_LAYOUT_UNDEFINED};
            ImageInfos.push_back(ImageInfo);
          }
        } else if (R.isTexture()) {
          VkImageViewCreateInfo ViewCreateInfo = {};
          ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
          ViewCreateInfo.viewType = getImageViewType(R.Kind);
          ViewCreateInfo.format =
              getVKFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
          ViewCreateInfo.components = {
              VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
              VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
          ViewCreateInfo.subresourceRange.aspectMask =
              VK_IMAGE_ASPECT_COLOR_BIT;
          ViewCreateInfo.subresourceRange.baseMipLevel = 0;
          ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
          ViewCreateInfo.subresourceRange.layerCount = 1;
          ViewCreateInfo.subresourceRange.levelCount = 1;
          IndexOfFirstBufferDataInArray = ImageInfos.size();
          for (auto &ResRef : IS.Resources[OverallResIdx].ResourceRefs) {
            ViewCreateInfo.image = ResRef.Image.Image;
            VkImageView View = {0};
            if (vkCreateImageView(IS.Device, &ViewCreateInfo, nullptr, &View))
              return llvm::createStringError(std::errc::device_or_resource_busy,
                                             "Failed to create image view.");
            const VkDescriptorImageInfo ImageInfo = {ResRef.Image.Sampler, View,
                                                     VK_IMAGE_LAYOUT_GENERAL};
            IS.ImageViews.push_back(View);
            ImageInfos.push_back(ImageInfo);
          }
        } else if (R.isRaw()) {
          IndexOfFirstBufferDataInArray = BufferInfos.size();
          for (auto ResRef : IS.Resources[OverallResIdx].ResourceRefs) {
            const VkDescriptorBufferInfo BI = {ResRef.Device.Buffer, 0,
                                               VK_WHOLE_SIZE};
            BufferInfos.push_back(BI);
          }
        } else {
          VkBufferViewCreateInfo ViewCreateInfo = {};
          const VkFormat Format =
              getVKFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
          ViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
          ViewCreateInfo.format = Format;
          ViewCreateInfo.range = VK_WHOLE_SIZE;
          VkBufferView View = {0};
          IndexOfFirstBufferDataInArray = BufferViews.size();
          for (auto &ResRef : IS.Resources[OverallResIdx].ResourceRefs) {
            ViewCreateInfo.buffer = ResRef.Device.Buffer;
            if (vkCreateBufferView(IS.Device, &ViewCreateInfo, nullptr, &View))
              return llvm::createStringError(std::errc::device_or_resource_busy,
                                             "Failed to create buffer view.");
            IS.BufferViews.push_back(View);
            BufferViews.push_back(View);
          }
        }

        VkWriteDescriptorSet WDS = {};
        WDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        WDS.dstSet = IS.DescriptorSets[SetIdx];
        WDS.dstBinding = R.VKBinding->Binding;
        WDS.descriptorCount = R.getArraySize();
        WDS.descriptorType = getDescriptorType(R.Kind);
        if (R.isTexture() || R.isSampler())
          WDS.pImageInfo = &ImageInfos[IndexOfFirstBufferDataInArray];
        else if (R.isRaw())
          WDS.pBufferInfo = &BufferInfos[IndexOfFirstBufferDataInArray];
        else
          WDS.pTexelBufferView = &BufferViews[IndexOfFirstBufferDataInArray];
        llvm::outs() << "Updating Descriptor [" << OverallResIdx << "] { "
                     << SetIdx << ", " << RIdx << " }\n";
        WriteDescriptors.push_back(WDS);

        if (R.HasCounter) {
          IndexOfFirstBufferDataInArray = BufferInfos.size();
          for (auto ResRef : IS.Resources[OverallResIdx].CounterResourceRefs) {
            const VkDescriptorBufferInfo BI = {ResRef.Device.Buffer, 0,
                                               VK_WHOLE_SIZE};
            BufferInfos.push_back(BI);
          }

          VkWriteDescriptorSet CounterWDS = {};
          CounterWDS.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
          CounterWDS.dstSet = IS.DescriptorSets[SetIdx];
          CounterWDS.dstBinding = *R.VKBinding->CounterBinding;
          CounterWDS.descriptorCount = R.getArraySize();
          CounterWDS.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          CounterWDS.pBufferInfo = &BufferInfos[IndexOfFirstBufferDataInArray];
          llvm::outs() << "Updating Counter Descriptor [" << OverallResIdx
                       << "] { " << SetIdx << ", " << RIdx << " }\n";
          llvm::outs() << "Binding = " << CounterWDS.dstBinding << "\n";
          WriteDescriptors.push_back(CounterWDS);
        }
      }
    }
    assert(ImageInfos.size() == ImageInfoCount &&
           BufferInfos.size() == BufferInfoCount &&
           BufferViews.size() == BufferViewCount &&
           "size of buffer infos does not match expected count");

    llvm::outs() << "WriteDescriptors: " << WriteDescriptors.size() << "\n";
    vkUpdateDescriptorSets(IS.Device, WriteDescriptors.size(),
                           WriteDescriptors.data(), 0, nullptr);
    return llvm::Error::success();
  }

  llvm::Error createShaderModules(Pipeline &P, InvocationState &IS) {
    for (const auto &Shader : P.Shaders) {
      const llvm::StringRef Program = Shader.Shader->getBuffer();
      VkShaderModuleCreateInfo ShaderCreateInfo = {};
      ShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      ShaderCreateInfo.codeSize = Program.size();
      ShaderCreateInfo.pCode =
          reinterpret_cast<const uint32_t *>(Program.data());
      CompiledShader CS = {Shader.Stage, Shader.Entry, 0};
      if (vkCreateShaderModule(IS.Device, &ShaderCreateInfo, nullptr,
                               &CS.Shader))
        return llvm::createStringError(std::errc::not_supported,
                                       "Failed to create shader module.");
      IS.Shaders.emplace_back(CS);
    }
    return llvm::Error::success();
  }

  llvm::Error createRenderPass(Pipeline &P, InvocationState &IS) {
    std::array<VkAttachmentDescription, 2> Attachments = {};

    Attachments[0].format = getVKFormat(P.Bindings.RTargetBufferPtr->Format,
                                        P.Bindings.RTargetBufferPtr->Channels);
    Attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    Attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    Attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    Attachments[1].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    Attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    Attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    Attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    Attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    Attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Attachments[1].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference ColorReference = {};
    ColorReference.attachment = 0;
    ColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference DepthReference = {};
    DepthReference.attachment = 1;
    DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription SubpassDescription = {};
    SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDescription.colorAttachmentCount = 1;
    SubpassDescription.pColorAttachments = &ColorReference;
    SubpassDescription.pDepthStencilAttachment = &DepthReference;
    SubpassDescription.inputAttachmentCount = 0;
    SubpassDescription.pInputAttachments = nullptr;
    SubpassDescription.preserveAttachmentCount = 0;
    SubpassDescription.pPreserveAttachments = nullptr;
    SubpassDescription.pResolveAttachments = nullptr;

    std::array<VkSubpassDependency, 2> Dependencies = {};

    Dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    Dependencies[0].dstSubpass = 0;
    Dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    Dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    Dependencies[0].srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    Dependencies[0].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    Dependencies[0].dependencyFlags = 0;

    Dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    Dependencies[1].dstSubpass = 0;
    Dependencies[1].srcStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependencies[1].dstStageMask =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    Dependencies[1].srcAccessMask = 0;
    Dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    Dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo RPCI = {};
    RPCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RPCI.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RPCI.pAttachments = Attachments.data();
    RPCI.subpassCount = 1;
    RPCI.pSubpasses = &SubpassDescription;
    RPCI.dependencyCount = static_cast<uint32_t>(Dependencies.size());
    RPCI.pDependencies = Dependencies.data();

    if (vkCreateRenderPass(IS.Device, &RPCI, nullptr, &IS.RenderPass))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create render pass.");
    return llvm::Error::success();
  }

  llvm::Error createFrameBuffer(Pipeline &P, InvocationState &IS) {
    std::array<VkImageView, 2> Views = {};
    VkImageViewCreateInfo ViewCreateInfo = {};
    ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ViewCreateInfo.format = getVKFormat(P.Bindings.RTargetBufferPtr->Format,
                                        P.Bindings.RTargetBufferPtr->Channels);
    ViewCreateInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                                 VK_COMPONENT_SWIZZLE_B,
                                 VK_COMPONENT_SWIZZLE_A};
    ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ViewCreateInfo.subresourceRange.layerCount = 1;
    ViewCreateInfo.subresourceRange.levelCount = 1;
    ViewCreateInfo.image = IS.FrameBufferResource.ResourceRefs[0].Image.Image;
    if (vkCreateImageView(IS.Device, &ViewCreateInfo, nullptr, &Views[0]))
      return llvm::createStringError(
          std::errc::device_or_resource_busy,
          "Failed to create frame buffer image view.");
    IS.ImageViews.push_back(Views[0]);

    VkImageViewCreateInfo DepthStencilViewCi = {};
    DepthStencilViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    DepthStencilViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    DepthStencilViewCi.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
    DepthStencilViewCi.subresourceRange = {};
    DepthStencilViewCi.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    DepthStencilViewCi.subresourceRange.baseMipLevel = 0;
    DepthStencilViewCi.subresourceRange.levelCount = 1;
    DepthStencilViewCi.subresourceRange.baseArrayLayer = 0;
    DepthStencilViewCi.subresourceRange.layerCount = 1;
    DepthStencilViewCi.image = IS.DepthStencil.Image;
    if (vkCreateImageView(IS.Device, &DepthStencilViewCi, nullptr, &Views[1]))
      return llvm::createStringError(
          std::errc::device_or_resource_busy,
          "Failed to create depth stencil image view.");
    IS.ImageViews.push_back(Views[1]);

    VkFramebufferCreateInfo FbufCreateInfo = {};
    FbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FbufCreateInfo.renderPass = IS.RenderPass;
    FbufCreateInfo.attachmentCount = Views.size();
    FbufCreateInfo.pAttachments = Views.data();
    FbufCreateInfo.width = P.Bindings.RTargetBufferPtr->OutputProps.Width;
    FbufCreateInfo.height = P.Bindings.RTargetBufferPtr->OutputProps.Height;
    FbufCreateInfo.layers = 1;

    if (vkCreateFramebuffer(IS.Device, &FbufCreateInfo, nullptr,
                            &IS.FrameBuffer))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create frame buffer.");
    return llvm::Error::success();
  }

  static llvm::Error
  parseSpecializationConstant(const SpecializationConstant &SpecConst,
                              VkSpecializationMapEntry &Entry,
                              llvm::SmallVector<char> &SpecData) {
    Entry.constantID = SpecConst.ConstantID;
    Entry.offset = SpecData.size();
    switch (SpecConst.Type) {
    case DataFormat::Float32: {
      float Value = 0.0f;
      double Tmp = 0.0;
      if (llvm::StringRef(SpecConst.Value).getAsDouble(Tmp))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid float value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Value = static_cast<float>(Tmp);
      Entry.size = sizeof(float);
      SpecData.resize(SpecData.size() + sizeof(float));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(float));
      break;
    }
    case DataFormat::Float64: {
      double Value = 0.0;
      if (llvm::StringRef(SpecConst.Value).getAsDouble(Value))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid double value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Entry.size = sizeof(double);
      SpecData.resize(SpecData.size() + sizeof(double));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(double));
      break;
    }
    case DataFormat::Int16: {
      int16_t Value = 0;
      if (llvm::StringRef(SpecConst.Value).getAsInteger(0, Value))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid int16 value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Entry.size = sizeof(int16_t);
      SpecData.resize(SpecData.size() + sizeof(int16_t));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(int16_t));
      break;
    }
    case DataFormat::UInt16: {
      uint16_t Value = 0;
      if (llvm::StringRef(SpecConst.Value).getAsInteger(0, Value))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid uint16 value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Entry.size = sizeof(uint16_t);
      SpecData.resize(SpecData.size() + sizeof(uint16_t));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(uint16_t));
      break;
    }
    case DataFormat::Int32: {
      int32_t Value = 0;
      if (llvm::StringRef(SpecConst.Value).getAsInteger(0, Value))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid int32 value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Entry.size = sizeof(int32_t);
      SpecData.resize(SpecData.size() + sizeof(int32_t));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(int32_t));
      break;
    }
    case DataFormat::UInt32: {
      uint32_t Value = 0;
      if (llvm::StringRef(SpecConst.Value).getAsInteger(0, Value))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid uint32 value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Entry.size = sizeof(uint32_t);
      SpecData.resize(SpecData.size() + sizeof(uint32_t));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(uint32_t));
      break;
    }
    case DataFormat::Bool: {
      bool Value = false;
      if (llvm::StringRef(SpecConst.Value).getAsInteger(0, Value))
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Invalid bool value for specialization constant '%s'",
            SpecConst.Value.c_str());
      Entry.size = sizeof(bool);
      SpecData.resize(SpecData.size() + sizeof(bool));
      memcpy(SpecData.data() + Entry.offset, &Value, sizeof(bool));
      break;
    }
    default:
      llvm_unreachable("Unsupported specialization constant type");
    }
    return llvm::Error::success();
  }

  llvm::Error createPipeline(Pipeline &P, InvocationState &IS) {
    VkPipelineCacheCreateInfo CacheCreateInfo = {};
    CacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (vkCreatePipelineCache(IS.Device, &CacheCreateInfo, nullptr,
                              &IS.PipelineCache))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create pipeline cache.");

    if (P.isCompute()) {
      const offloadtest::Shader &Shader = P.Shaders[0];
      assert(IS.Shaders.size() == 1 &&
             "Currently only support one compute shader");
      const CompiledShader &S = IS.Shaders[0];
      VkPipelineShaderStageCreateInfo StageInfo = {};
      StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      StageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
      StageInfo.module = S.Shader;
      StageInfo.pName = S.Entry.c_str();

      llvm::SmallVector<VkSpecializationMapEntry> SpecEntries;
      llvm::SmallVector<char> SpecData;
      VkSpecializationInfo SpecInfo = {};
      if (!Shader.SpecializationConstants.empty()) {
        llvm::DenseSet<uint32_t> SeenConstantIDs;
        for (const auto &SpecConst : Shader.SpecializationConstants) {
          if (!SeenConstantIDs.insert(SpecConst.ConstantID).second)
            return llvm::createStringError(
                std::errc::invalid_argument,
                "Test configuration contains multiple entries for "
                "specialization constant ID %u.",
                SpecConst.ConstantID);

          VkSpecializationMapEntry Entry;
          if (auto Err =
                  parseSpecializationConstant(SpecConst, Entry, SpecData))
            return Err;
          SpecEntries.push_back(Entry);
        }

        SpecInfo.mapEntryCount = SpecEntries.size();
        SpecInfo.pMapEntries = SpecEntries.data();
        SpecInfo.dataSize = SpecData.size();
        SpecInfo.pData = SpecData.data();
        StageInfo.pSpecializationInfo = &SpecInfo;
      }

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

    llvm::SmallVector<VkPipelineShaderStageCreateInfo> Stages;
    for (const auto &S : IS.Shaders) {
      VkPipelineShaderStageCreateInfo StageInfo = {};
      StageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      StageInfo.stage = getShaderStageFlag(S.Stage);
      StageInfo.module = S.Shader;
      StageInfo.pName = S.Entry.c_str();
      Stages.emplace_back(StageInfo);
    }

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};
    InputAssemblyCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterizationStateCreateInfo RastStateCI = {};
    RastStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RastStateCI.polygonMode = VK_POLYGON_MODE_FILL;
    RastStateCI.cullMode = VK_CULL_MODE_NONE;
    RastStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RastStateCI.depthClampEnable = VK_FALSE;
    RastStateCI.rasterizerDiscardEnable = VK_FALSE;
    RastStateCI.depthBiasEnable = VK_FALSE;
    RastStateCI.lineWidth = 1.0f;

    VkPipelineColorBlendAttachmentState BlendState = {};
    BlendState.colorWriteMask = 0xf;
    BlendState.blendEnable = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo BlendStateCI = {};
    BlendStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    BlendStateCI.attachmentCount = 1;
    BlendStateCI.pAttachments = &BlendState;

    VkPipelineViewportStateCreateInfo ViewStateCI = {};
    ViewStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewStateCI.viewportCount = 1;
    ViewStateCI.scissorCount = 1;

    const VkDynamicState DynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                            VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo DynamicStateCI = {};
    DynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicStateCI.pDynamicStates = &DynamicStates[0];
    DynamicStateCI.dynamicStateCount = 2;

    VkPipelineDepthStencilStateCreateInfo DepthStencilStateCI = {};
    DepthStencilStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilStateCI.depthTestEnable = VK_TRUE;
    DepthStencilStateCI.depthWriteEnable = VK_TRUE;
    DepthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    DepthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
    DepthStencilStateCI.back.failOp = VK_STENCIL_OP_KEEP;
    DepthStencilStateCI.back.passOp = VK_STENCIL_OP_KEEP;
    DepthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
    DepthStencilStateCI.stencilTestEnable = VK_FALSE;
    DepthStencilStateCI.front = DepthStencilStateCI.back;

    VkPipelineMultisampleStateCreateInfo MultisampleStateCI = {};
    MultisampleStateCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    const uint32_t Stride = P.Bindings.getVertexStride();

    VkVertexInputBindingDescription VertexInputBinding{};
    VertexInputBinding.binding = 0;
    VertexInputBinding.stride = Stride;
    VertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    llvm::SmallVector<VkVertexInputAttributeDescription> Attributes;
    for (size_t I = 0; I < P.Bindings.VertexAttributes.size(); ++I) {
      const VertexAttribute &VA = P.Bindings.VertexAttributes[I];
      VkVertexInputAttributeDescription VkVA = {};
      VkVA.location = I;
      VkVA.binding = 0;
      VkVA.format = getVKFormat(VA.Format, VA.Channels);
      VkVA.offset = VA.Offset;
      Attributes.push_back(VkVA);
    }

    VkPipelineVertexInputStateCreateInfo VertexInputStateCi = {};
    VertexInputStateCi.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputStateCi.vertexBindingDescriptionCount = 1;
    VertexInputStateCi.pVertexBindingDescriptions = &VertexInputBinding;
    VertexInputStateCi.vertexAttributeDescriptionCount = Attributes.size();
    VertexInputStateCi.pVertexAttributeDescriptions = Attributes.data();

    VkGraphicsPipelineCreateInfo PipelineCreateInfo = {};
    PipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCreateInfo.stageCount = Stages.size();
    PipelineCreateInfo.pStages = Stages.data();
    PipelineCreateInfo.pVertexInputState = &VertexInputStateCi;
    PipelineCreateInfo.pInputAssemblyState = &InputAssemblyCI;
    PipelineCreateInfo.pRasterizationState = &RastStateCI;
    PipelineCreateInfo.pColorBlendState = &BlendStateCI;
    PipelineCreateInfo.pMultisampleState = &MultisampleStateCI;
    PipelineCreateInfo.pViewportState = &ViewStateCI;
    PipelineCreateInfo.pDepthStencilState = &DepthStencilStateCI;
    PipelineCreateInfo.pDynamicState = &DynamicStateCI;
    PipelineCreateInfo.renderPass = IS.RenderPass;
    PipelineCreateInfo.layout = IS.PipelineLayout;

    if (vkCreateGraphicsPipelines(IS.Device, IS.PipelineCache, 1,
                                  &PipelineCreateInfo, nullptr, &IS.Pipeline))
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create graphics pipeline.");

    return llvm::Error::success();
  }

  void copyResourceDataToDevice(InvocationState &IS, ResourceBundle &R) {
    if (R.isSampler())
      return;
    if (R.isImage()) {
      const offloadtest::Buffer &B = *R.BufferPtr;
      VkBufferImageCopy BufferCopyRegion = {};
      BufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      BufferCopyRegion.imageSubresource.mipLevel = 0;
      BufferCopyRegion.imageSubresource.baseArrayLayer = 0;
      BufferCopyRegion.imageSubresource.layerCount = 1;
      BufferCopyRegion.imageExtent.width = B.OutputProps.Width;
      BufferCopyRegion.imageExtent.height = B.OutputProps.Height;
      BufferCopyRegion.imageExtent.depth = 1;
      BufferCopyRegion.bufferOffset = 0;

      VkImageSubresourceRange SubRange = {};
      SubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      SubRange.baseMipLevel = 0;
      SubRange.levelCount = 1;
      SubRange.layerCount = 1;

      VkImageMemoryBarrier ImageBarrier = {};
      ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

      ImageBarrier.subresourceRange = SubRange;
      ImageBarrier.srcAccessMask = 0;
      ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImageBarrier.oldLayout = R.ImageLayout;
      ImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      R.ImageLayout = VK_IMAGE_LAYOUT_GENERAL;

      for (auto &ResRef : R.ResourceRefs) {
        ImageBarrier.image = ResRef.Image.Image;
        vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &ImageBarrier);

        vkCmdCopyBufferToImage(IS.CmdBuffer, ResRef.Host.Buffer,
                               ResRef.Image.Image, VK_IMAGE_LAYOUT_GENERAL, 1,
                               &BufferCopyRegion);
      }

      ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImageBarrier.dstAccessMask =
          VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      ImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

      for (auto &ResRef : R.ResourceRefs) {
        ImageBarrier.image = ResRef.Image.Image;
        vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &ImageBarrier);
      }
      return;
    }
    VkBufferMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.size = VK_WHOLE_SIZE;
    Barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    Barrier.dstAccessMask = 0;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    for (auto &ResRef : R.ResourceRefs) {
      Barrier.buffer = ResRef.Host.Buffer;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           1, &Barrier, 0, nullptr);
    }
  }

  void copyResourceDataToHost(InvocationState &IS, ResourceBundle &R) {
    if (!R.isReadWrite())
      return;
    if (R.isImage()) {
      VkImageSubresourceRange SubRange = {};
      SubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      SubRange.baseMipLevel = 0;
      SubRange.levelCount = 1;
      SubRange.layerCount = 1;

      VkImageMemoryBarrier ImageBarrier = {};
      ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

      ImageBarrier.subresourceRange = SubRange;
      ImageBarrier.srcAccessMask = 0;
      ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      ImageBarrier.oldLayout = R.ImageLayout;
      ImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
      R.ImageLayout = VK_IMAGE_LAYOUT_GENERAL;

      for (auto &ResRef : R.ResourceRefs) {
        ImageBarrier.image = ResRef.Image.Image;
        vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &ImageBarrier);
      }

      const offloadtest::Buffer &B = *R.BufferPtr;
      VkBufferImageCopy BufferCopyRegion = {};
      BufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      BufferCopyRegion.imageSubresource.mipLevel = 0;
      BufferCopyRegion.imageSubresource.baseArrayLayer = 0;
      BufferCopyRegion.imageSubresource.layerCount = 1;
      BufferCopyRegion.imageExtent.width = B.OutputProps.Width;
      BufferCopyRegion.imageExtent.height = B.OutputProps.Height;
      BufferCopyRegion.imageExtent.depth = 1;
      BufferCopyRegion.bufferOffset = 0;
      for (auto &ResRef : R.ResourceRefs)
        vkCmdCopyImageToBuffer(IS.CmdBuffer, ResRef.Image.Image,
                               VK_IMAGE_LAYOUT_GENERAL, ResRef.Host.Buffer, 1,
                               &BufferCopyRegion);

      VkBufferMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      for (auto &ResRef : R.ResourceRefs) {
        Barrier.buffer = ResRef.Host.Buffer;
        vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                             &Barrier, 0, nullptr);
      }
      return;
    }
    VkBufferMemoryBarrier Barrier = {};
    Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    Barrier.size = VK_WHOLE_SIZE;
    Barrier.srcAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    for (auto &ResRef : R.ResourceRefs) {
      Barrier.buffer = ResRef.Host.Buffer;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
    VkBufferCopy CopyRegion = {};
    CopyRegion.size = R.size();
    for (auto &ResRef : R.ResourceRefs)
      vkCmdCopyBuffer(IS.CmdBuffer, ResRef.Device.Buffer, ResRef.Host.Buffer, 1,
                      &CopyRegion);

    VkBufferCopy CounterCopyRegion = {};
    CounterCopyRegion.size = sizeof(uint32_t);
    for (auto &ResRef : R.CounterResourceRefs)
      vkCmdCopyBuffer(IS.CmdBuffer, ResRef.Device.Buffer, ResRef.Host.Buffer, 1,
                      &CounterCopyRegion);

    Barrier.size = VK_WHOLE_SIZE;
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    for (auto &ResRef : R.ResourceRefs) {
      Barrier.buffer = ResRef.Host.Buffer;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
    for (auto &ResRef : R.CounterResourceRefs) {
      Barrier.buffer = ResRef.Host.Buffer;
      vkCmdPipelineBarrier(IS.CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
  }

  llvm::Error createCommands(Pipeline &P, InvocationState &IS) {
    for (auto &R : IS.Resources)
      copyResourceDataToDevice(IS, R);

    if (P.isGraphics()) {
      VkClearValue ClearValues[2] = {};
      ClearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
      ClearValues[1].depthStencil = {1.0f, 0};

      VkRenderPassBeginInfo RenderPassBeginInfo = {};
      RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      RenderPassBeginInfo.renderPass = IS.RenderPass;
      RenderPassBeginInfo.framebuffer = IS.FrameBuffer;
      RenderPassBeginInfo.renderArea.extent.width =
          P.Bindings.RTargetBufferPtr->OutputProps.Width;
      RenderPassBeginInfo.renderArea.extent.height =
          P.Bindings.RTargetBufferPtr->OutputProps.Height;
      RenderPassBeginInfo.clearValueCount = 2;
      RenderPassBeginInfo.pClearValues = ClearValues;

      vkCmdBeginRenderPass(IS.CmdBuffer, &RenderPassBeginInfo,
                           VK_SUBPASS_CONTENTS_INLINE);

      VkViewport Viewport = {};
      Viewport.x = 0.0f;
      Viewport.y = 0.0f;
      Viewport.width =
          static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Width);
      Viewport.height =
          static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Height);
      Viewport.minDepth = 0.0f;
      Viewport.maxDepth = 1.0f;
      vkCmdSetViewport(IS.CmdBuffer, 0, 1, &Viewport);

      VkRect2D Scissor = {};
      Scissor.offset = {0, 0};
      Scissor.extent.width = P.Bindings.RTargetBufferPtr->OutputProps.Width;
      Scissor.extent.height = P.Bindings.RTargetBufferPtr->OutputProps.Height;
      vkCmdSetScissor(IS.CmdBuffer, 0, 1, &Scissor);
    }

    const VkPipelineBindPoint BindPoint = P.isGraphics()
                                              ? VK_PIPELINE_BIND_POINT_GRAPHICS
                                              : VK_PIPELINE_BIND_POINT_COMPUTE;
    vkCmdBindPipeline(IS.CmdBuffer, BindPoint, IS.Pipeline);
    if (IS.DescriptorSets.size() > 0)
      vkCmdBindDescriptorSets(IS.CmdBuffer, BindPoint, IS.PipelineLayout, 0,
                              IS.DescriptorSets.size(),
                              IS.DescriptorSets.data(), 0, 0);

    for (const auto &PCB : P.PushConstants) {
      llvm::SmallVector<uint8_t, 4> Data;
      PCB.getContent(Data);
      vkCmdPushConstants(IS.CmdBuffer, IS.PipelineLayout,
                         getShaderStageFlag(PCB.Stage), 0, Data.size(),
                         Data.data());
    }

    if (P.isCompute()) {
      const llvm::ArrayRef<int> DispatchSize =
          llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
      vkCmdDispatch(IS.CmdBuffer, DispatchSize[0], DispatchSize[1],
                    DispatchSize[2]);
      llvm::outs() << "Dispatched compute shader: { " << DispatchSize[0] << ", "
                   << DispatchSize[1] << ", " << DispatchSize[2] << " }\n";
    } else {
      VkDeviceSize Offsets[1]{0};
      assert(IS.VertexBuffer.has_value());
      vkCmdBindVertexBuffers(IS.CmdBuffer, 0, 1,
                             &IS.VertexBuffer->Device.Buffer, Offsets);
      // instanceCount must be >=1 to draw; previously was 0 which draws nothing
      vkCmdDraw(IS.CmdBuffer, P.Bindings.getVertexCount(), 1, 0, 0);
      llvm::outs() << "Drew " << P.Bindings.getVertexCount() << " vertices.\n";
      vkCmdEndRenderPass(IS.CmdBuffer);
      copyResourceDataToHost(IS, IS.FrameBufferResource);
    }

    for (auto &R : IS.Resources)
      copyResourceDataToHost(IS, R);
    return llvm::Error::success();
  }

  llvm::Error readBackData(Pipeline &P, InvocationState &IS) {
    uint32_t BufIdx = 0;
    for (auto &S : P.Sets) {
      for (int I = 0, E = S.Resources.size(); I < E; ++I, ++BufIdx) {
        const Resource &R = S.Resources[I];
        if (!R.isReadWrite())
          continue;
        VkMappedMemoryRange Range = {};
        Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        Range.offset = 0;
        Range.size = VK_WHOLE_SIZE;
        auto &ResourceRef = IS.Resources[BufIdx].ResourceRefs;
        auto &DataSet = R.BufferPtr->Data;
        auto *ResRefIt = ResourceRef.begin();
        auto *DataIt = DataSet.begin();
        for (; ResRefIt != ResourceRef.end() && DataIt != DataSet.end();
             ++ResRefIt, ++DataIt) {
          void *Mapped = nullptr; // NOLINT(misc-const-correctness)
          vkMapMemory(IS.Device, ResRefIt->Host.Memory, 0, VK_WHOLE_SIZE, 0,
                      &Mapped);
          Range.memory = ResRefIt->Host.Memory;
          vkInvalidateMappedMemoryRanges(IS.Device, 1, &Range);
          memcpy(DataIt->get(), Mapped, R.size());
          vkUnmapMemory(IS.Device, ResRefIt->Host.Memory);
        }
        if (R.HasCounter) {
          R.BufferPtr->Counters.clear();
          for (uint32_t I = 0; I < R.getArraySize(); ++I) {
            uint32_t *Mapped = nullptr; // NOLINT(misc-const-correctness)
            auto &CounterRef = IS.Resources[BufIdx].CounterResourceRefs[I];
            vkMapMemory(IS.Device, CounterRef.Host.Memory, 0, VK_WHOLE_SIZE, 0,
                        (void **)&Mapped);
            Range.memory = CounterRef.Host.Memory;
            vkInvalidateMappedMemoryRanges(IS.Device, 1, &Range);
            R.BufferPtr->Counters.push_back(*Mapped);
            vkUnmapMemory(IS.Device, CounterRef.Host.Memory);
          }
        }
      }
    }

    // Copy back the frame buffer data if this was a graphics pipeline.
    if (P.isGraphics()) {
      VkMappedMemoryRange Range = {};
      Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      Range.offset = 0;
      Range.size = VK_WHOLE_SIZE;
      const ResourceRef &ResRef = IS.FrameBufferResource.ResourceRefs[0];

      void *Mapped = nullptr; // NOLINT(misc-const-correctness)
      vkMapMemory(IS.Device, ResRef.Host.Memory, 0, VK_WHOLE_SIZE, 0, &Mapped);

      Range.memory = ResRef.Host.Memory;
      vkInvalidateMappedMemoryRanges(IS.Device, 1, &Range);

      const Buffer &B = *P.Bindings.RTargetBufferPtr;
      memcpy(B.Data[0].get(), Mapped, B.size());
      vkUnmapMemory(IS.Device, ResRef.Host.Memory);
    }
    return llvm::Error::success();
  }

  llvm::Error cleanup(InvocationState &IS) {
    vkQueueWaitIdle(IS.Queue);
    for (auto &V : IS.BufferViews)
      vkDestroyBufferView(IS.Device, V, nullptr);

    for (auto &V : IS.ImageViews)
      vkDestroyImageView(IS.Device, V, nullptr);

    for (auto &R : IS.Resources) {
      for (auto &ResRef : R.ResourceRefs) {
        if (R.isBuffer()) {
          vkDestroyBuffer(IS.Device, ResRef.Device.Buffer, nullptr);
          vkFreeMemory(IS.Device, ResRef.Device.Memory, nullptr);
        } else if (R.isSampler()) {
          vkDestroySampler(IS.Device, ResRef.Image.Sampler, nullptr);
        } else {
          assert(R.isImage());
          vkDestroyImage(IS.Device, ResRef.Image.Image, nullptr);
          vkFreeMemory(IS.Device, ResRef.Image.Memory, nullptr);
        }
        vkDestroyBuffer(IS.Device, ResRef.Host.Buffer, nullptr);
        vkFreeMemory(IS.Device, ResRef.Host.Memory, nullptr);
      }
      for (auto &ResRef : R.CounterResourceRefs) {
        vkDestroyBuffer(IS.Device, ResRef.Device.Buffer, nullptr);
        vkFreeMemory(IS.Device, ResRef.Device.Memory, nullptr);
        vkDestroyBuffer(IS.Device, ResRef.Host.Buffer, nullptr);
        vkFreeMemory(IS.Device, ResRef.Host.Memory, nullptr);
      }
    }

    if (IS.getFullShaderStageMask() != VK_SHADER_STAGE_COMPUTE_BIT) {
      if (IS.VertexBuffer.has_value()) {
        vkDestroyBuffer(IS.Device, IS.VertexBuffer->Device.Buffer, nullptr);
        vkFreeMemory(IS.Device, IS.VertexBuffer->Device.Memory, nullptr);
        vkDestroyBuffer(IS.Device, IS.VertexBuffer->Host.Buffer, nullptr);
        vkFreeMemory(IS.Device, IS.VertexBuffer->Host.Memory, nullptr);
      }
      for (auto &ResRef : IS.FrameBufferResource.ResourceRefs) {
        // We know the device resource is an image, so no need to check it.
        vkDestroyImage(IS.Device, ResRef.Image.Image, nullptr);
        vkFreeMemory(IS.Device, ResRef.Image.Memory, nullptr);
        vkDestroyBuffer(IS.Device, ResRef.Host.Buffer, nullptr);
        vkFreeMemory(IS.Device, ResRef.Host.Memory, nullptr);
      }
      vkDestroyImage(IS.Device, IS.DepthStencil.Image, nullptr);
      vkFreeMemory(IS.Device, IS.DepthStencil.Memory, nullptr);
      vkDestroyFramebuffer(IS.Device, IS.FrameBuffer, nullptr);
      vkDestroyRenderPass(IS.Device, IS.RenderPass, nullptr);
    }

    vkDestroyPipeline(IS.Device, IS.Pipeline, nullptr);

    for (auto &S : IS.Shaders)
      vkDestroyShaderModule(IS.Device, S.Shader, nullptr);

    vkDestroyPipelineCache(IS.Device, IS.PipelineCache, nullptr);

    vkDestroyPipelineLayout(IS.Device, IS.PipelineLayout, nullptr);

    for (auto &L : IS.DescriptorSetLayouts)
      vkDestroyDescriptorSetLayout(IS.Device, L, nullptr);

    if (IS.Pool)
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
    if (auto Err = createShaderModules(P, State))
      return Err;
    llvm::outs() << "Shader module created.\n";
    if (auto Err = createCommandBuffer(State))
      return Err;
    llvm::outs() << "Copy command buffer created.\n";
    if (auto Err = createResources(P, State))
      return Err;
    if (P.isGraphics()) {
      if (auto Err = createRenderPass(P, State))
        return Err;
      llvm::outs() << "Render pass created.\n";
      if (auto Err = createFrameBuffer(P, State))
        return Err;
      llvm::outs() << "Frame buffer created.\n";
    }
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
    if (auto Err = createPipeline(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";
    if (auto Err = createCommands(P, State))
      return Err;
    llvm::outs() << "Commands created.\n";
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

  llvm::Error initialize(const DeviceConfig Config) {
    // Create a Vulkan 1.1 instance to determine the API version
    VkApplicationInfo AppInfo = {};
    AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    AppInfo.pApplicationName = "OffloadTest";
    // TODO: We should set this based on a command line flag, and simplify the
    // code below to error if the requested version isn't supported.
    AppInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    CreateInfo.pApplicationInfo = &AppInfo;

    llvm::SmallVector<const char *> Extensions;
    llvm::SmallVector<const char *> Layers;
#if __APPLE__
    // If we build Vulkan support for Apple platforms the VK_KHR_PORTABILITY
    // extension is required, so we can just force this one on. If it fails, the
    // whole device would fail anyways.
    Extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    CreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    CreateInfo.ppEnabledExtensionNames = Extensions.data();
    CreateInfo.enabledExtensionCount = Extensions.size();

    VkResult Res = vkCreateInstance(&CreateInfo, NULL, &Instance);
    if (Res == VK_ERROR_INCOMPATIBLE_DRIVER)
      return llvm::createStringError(std::errc::no_such_device,
                                     "Cannot find a base Vulkan device");
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

      if (Config.EnableValidationLayer) {
        const llvm::StringRef ValidationLayer = "VK_LAYER_KHRONOS_validation";
        if (TmpDev->isLayerSupported(ValidationLayer))
          Layers.push_back(ValidationLayer.data());
      }

      if (Config.EnableDebugLayer) {
        const llvm::StringRef DebugUtilsExtensionName = "VK_EXT_debug_utils";
        if (TmpDev->isExtensionSupported(DebugUtilsExtensionName))
          Extensions.push_back(DebugUtilsExtensionName.data());
      }

      CreateInfo.ppEnabledLayerNames = Layers.data();
      CreateInfo.enabledLayerCount = Layers.size();
      CreateInfo.ppEnabledExtensionNames = Extensions.data();
      CreateInfo.enabledExtensionCount = Extensions.size();
    }
    vkDestroyInstance(Instance, NULL);
    Instance = VK_NULL_HANDLE;

    // This second creation shouldn't ever fail, but it tries to create the
    // highest supported device version.
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

llvm::Error Device::initializeVKDevices(const DeviceConfig Config) {
  return VKContext::instance().initialize(Config);
}

void Device::cleanupVKDevices() { VKContext::instance().cleanup(); }
