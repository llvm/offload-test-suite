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
#include "API/FormatConversion.h"
#include "Support/Pipeline.h"
#include "Support/VkError.h"
#include "VKResources.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/Error.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <numeric>
#include <system_error>

using namespace offloadtest;

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
  case DataFormat::Depth32:
    if (Channels != 1)
      llvm_unreachable("Depth32 format only supports a single channel.");
    return VK_FORMAT_D32_SFLOAT;
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
    return VK_DESCRIPTOR_TYPE_SAMPLER;
  case ResourceKind::SampledTexture2D:
    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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
  case ResourceKind::SampledTexture2D:
    llvm_unreachable("Textures and samplers don't have buffer usage bits!");
  }
  llvm_unreachable("All cases handled");
}

static VkImageViewType getImageViewType(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
  case ResourceKind::SampledTexture2D:
    return VK_IMAGE_VIEW_TYPE_2D;
  case ResourceKind::Buffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::ConstantBuffer:
  case ResourceKind::Sampler:
    llvm_unreachable("Not an image view!");
  }
  llvm_unreachable("All cases handled");
}

static VkImageType getVKImageType(const ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
  case ResourceKind::SampledTexture2D:
    return VK_IMAGE_TYPE_2D;
  default:
    llvm_unreachable("Unsupported image kind");
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

static llvm::SmallVector<VkLayerProperties, 0> queryInstanceLayers() {
  uint32_t LayerCount;
  vkEnumerateInstanceLayerProperties(&LayerCount, nullptr);

  llvm::SmallVector<VkLayerProperties, 0> Layers;
  if (LayerCount == 0)
    return Layers;

  Layers.resize(LayerCount);
  vkEnumerateInstanceLayerProperties(&LayerCount, Layers.data());

  return Layers;
}

static bool
isLayerSupported(const llvm::SmallVector<VkLayerProperties, 0> &Layers,
                 llvm::StringRef QueryName) {
  for (auto &Layer : Layers) {
    if (Layer.layerName == QueryName)
      return true;
  }
  return false;
}

static llvm::SmallVector<VkExtensionProperties, 0>
queryInstanceExtensions(const char *InstanceLayer) {
  uint32_t ExtCount;
  vkEnumerateInstanceExtensionProperties(InstanceLayer, &ExtCount, nullptr);

  llvm::SmallVector<VkExtensionProperties, 0> Extensions;
  if (ExtCount == 0)
    return Extensions;

  Extensions.resize(ExtCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &ExtCount, Extensions.data());

  return Extensions;
}

static llvm::SmallVector<VkExtensionProperties, 0>
queryDeviceExtensions(VkPhysicalDevice PhysicalDevice) {
  uint32_t ExtCount;
  vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtCount,
                                       nullptr);

  llvm::SmallVector<VkExtensionProperties, 0> Extensions;
  if (ExtCount == 0)
    return Extensions;

  Extensions.resize(ExtCount);
  vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtCount,
                                       Extensions.data());

  return Extensions;
}

static bool isExtensionSupported(
    const llvm::SmallVector<VkExtensionProperties, 0> &Extensions,
    llvm::StringRef QueryName) {
  for (const auto &Ext : Extensions) {
    if (Ext.extensionName == QueryName)
      return true;
  }
  return false;
}

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

namespace {

class VulkanBuffer : public offloadtest::Buffer {
public:
  VkDevice Dev; // Needed for clean-up
  VkBuffer Buffer;
  VkDeviceMemory Memory;
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;

  VulkanBuffer(VkDevice Dev, VkBuffer Buffer, VkDeviceMemory Memory,
               llvm::StringRef Name, BufferCreateDesc Desc, size_t SizeInBytes)
      : offloadtest::Buffer(GPUAPI::Vulkan), Dev(Dev), Buffer(Buffer),
        Memory(Memory), Name(Name), Desc(Desc), SizeInBytes(SizeInBytes) {}

  ~VulkanBuffer() override {
    vkDestroyBuffer(Dev, Buffer, nullptr);
    vkFreeMemory(Dev, Memory, nullptr);
  }

  static bool classof(const offloadtest::Buffer *B) {
    return B->getAPI() == GPUAPI::Vulkan;
  }
};

class VulkanTexture : public offloadtest::Texture {
public:
  VkDevice Dev;
  VkImage Image;
  VkDeviceMemory Memory;
  // TODO:
  // RenderTarget and DepthStencil views are created at texture creation time.
  // Ideally Sampled/Storage image views would also live here, but they are
  // currently created during descriptor set setup, which determines their
  // binding layout.
  VkImageView View = VK_NULL_HANDLE;
  std::string Name;
  TextureCreateDesc Desc;

  VulkanTexture(VkDevice Dev, VkImage Image, VkDeviceMemory Memory,
                llvm::StringRef Name, TextureCreateDesc Desc)
      : offloadtest::Texture(GPUAPI::Vulkan), Dev(Dev), Image(Image),
        Memory(Memory), Name(Name), Desc(Desc) {}

  ~VulkanTexture() override {
    if (View)
      vkDestroyImageView(Dev, View, nullptr);
    vkDestroyImage(Dev, Image, nullptr);
    vkFreeMemory(Dev, Memory, nullptr);
  }

  static bool classof(const offloadtest::Texture *T) {
    return T->getAPI() == GPUAPI::Vulkan;
  }
};

class VulkanFence : public offloadtest::Fence {
public:
  VulkanFence(VkDevice Device, VkSemaphore Semaphore, llvm::StringRef Name)
      : Name(Name), Device(Device), Semaphore(Semaphore) {}

  std::string Name;
  VkDevice Device;
  VkSemaphore Semaphore;

  static llvm::Expected<std::unique_ptr<VulkanFence>>
  create(VkDevice Device, llvm::StringRef Name) {
    VkSemaphoreTypeCreateInfo TypeCreateInfo = {};
    TypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    TypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

    VkSemaphoreCreateInfo CreateInfo = {};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    CreateInfo.pNext = &TypeCreateInfo;

    VkSemaphore Semaphore = VK_NULL_HANDLE;
    if (auto Err = VK::toError(
            vkCreateSemaphore(Device, &CreateInfo, nullptr, &Semaphore),
            "Failed to create Semaphore."))
      return Err;

    return std::make_unique<VulkanFence>(Device, Semaphore, Name);
  }

  ~VulkanFence() { vkDestroySemaphore(Device, Semaphore, nullptr); }

  uint64_t getFenceValue() override {
    uint64_t Value = 0;
    [[maybe_unused]] const VkResult Ret =
        vkGetSemaphoreCounterValue(Device, Semaphore, &Value);
    assert(!Ret && "vkGetSemaphoreCounterValue failed but should never fail.");
    return Value;
  }

  llvm::Error waitForCompletion(uint64_t SignalValue) override {
    VkSemaphoreWaitInfo WaitInfo = {};
    WaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    WaitInfo.semaphoreCount = 1;
    WaitInfo.pSemaphores = &Semaphore;
    WaitInfo.pValues = &SignalValue;

    if (auto Err = VK::toError(vkWaitSemaphores(Device, &WaitInfo, UINT64_MAX),
                               "Failed to wait on Semaphore."))
      return Err;

    return llvm::Error::success();
  }
};

class VulkanQueue : public offloadtest::Queue {
public:
  using Queue::submit;

  VkQueue Queue = VK_NULL_HANDLE;
  uint32_t QueueFamilyIdx = 0;
  // TODO: Ensure device lifetime is managed (e.g. via shared_ptr).
  VkDevice Device = VK_NULL_HANDLE;
  std::unique_ptr<VulkanFence> SubmitFence;
  uint64_t FenceCounter = 0;
  // Batches of command buffers submitted to the GPU that may still be
  // in-flight.  VulkanCommandBuffer's destructor destroys the VkCommandPool,
  // which would invalidate any still-pending command buffers.  Each batch
  // records the fence value it signals so we can non-blockingly query
  // progress and release completed batches.
  struct InFlightBatch {
    uint64_t FenceValue;
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs;
  };
  llvm::SmallVector<InFlightBatch> InFlightBatches;

  VulkanQueue(VkQueue Q, uint32_t QueueFamilyIdx, VkDevice Device,
              std::unique_ptr<VulkanFence> SubmitFence)
      : Queue(Q), QueueFamilyIdx(QueueFamilyIdx), Device(Device),
        SubmitFence(std::move(SubmitFence)) {}

  llvm::Expected<offloadtest::SubmitResult>
  submit(llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs)
      override;
};

class VulkanCommandBuffer : public offloadtest::CommandBuffer {
public:
  VkDevice Device = VK_NULL_HANDLE;
  // Owned per command buffer so that recording, submission, and lifetime
  // management of each command buffer are independently safe without external
  // synchronization.
  VkCommandPool CmdPool = VK_NULL_HANDLE;
  VkCommandBuffer CmdBuffer = VK_NULL_HANDLE;

  static llvm::Expected<std::unique_ptr<VulkanCommandBuffer>>
  create(VkDevice Device, uint32_t QueueFamilyIdx) {
    auto CB = std::unique_ptr<VulkanCommandBuffer>(new VulkanCommandBuffer());
    CB->Device = Device;

    VkCommandPoolCreateInfo CmdPoolInfo = {};
    CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    CmdPoolInfo.queueFamilyIndex = QueueFamilyIdx;
    CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (auto Err = VK::toError(
            vkCreateCommandPool(Device, &CmdPoolInfo, nullptr, &CB->CmdPool),
            "Could not create command pool."))
      return Err;

    VkCommandBufferAllocateInfo CBufAllocInfo = {};
    CBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CBufAllocInfo.commandPool = CB->CmdPool;
    CBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CBufAllocInfo.commandBufferCount = 1;
    if (auto Err = VK::toError(
            vkAllocateCommandBuffers(Device, &CBufAllocInfo, &CB->CmdBuffer),
            "Could not create command buffer."))
      return Err;

    VkCommandBufferBeginInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (auto Err = VK::toError(vkBeginCommandBuffer(CB->CmdBuffer, &BufferInfo),
                               "Could not begin command buffer."))
      return Err;
    return CB;
  }

  ~VulkanCommandBuffer() override {
    if (CmdPool != VK_NULL_HANDLE)
      vkDestroyCommandPool(Device, CmdPool, nullptr);
  }

  static bool classof(const CommandBuffer *CB) {
    return CB->getKind() == GPUAPI::Vulkan;
  }

private:
  VulkanCommandBuffer() : CommandBuffer(GPUAPI::Vulkan) {}
};

class VulkanPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  VkDevice Dev;
  VkPipeline Pipeline;
  VkPipelineLayout Layout;
  llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
  VkRenderPass RenderPass;

  VulkanPipelineState(llvm::StringRef Name, VkDevice Dev, VkPipeline Pipeline,
                      VkPipelineLayout Layout,
                      llvm::SmallVector<VkDescriptorSetLayout> SetLayouts,
                      VkRenderPass RenderPass)
      : offloadtest::PipelineState(GPUAPI::Vulkan), Name(Name.str()), Dev(Dev),
        Pipeline(Pipeline), Layout(Layout), SetLayouts(std::move(SetLayouts)),
        RenderPass(RenderPass) {}

  ~VulkanPipelineState() override {
    vkDestroyPipeline(Dev, Pipeline, nullptr);
    vkDestroyRenderPass(Dev, RenderPass, nullptr);
    vkDestroyPipelineLayout(Dev, Layout, nullptr);
    for (VkDescriptorSetLayout L : SetLayouts)
      vkDestroyDescriptorSetLayout(Dev, L, nullptr);
  }

  static bool classof(const offloadtest::PipelineState *B) {
    return B->getAPI() == GPUAPI::Vulkan;
  }
};

class VulkanDevice : public offloadtest::Device {
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

  struct InvocationState {
    std::unique_ptr<VulkanCommandBuffer> CB;
    VkDescriptorPool Pool = VK_NULL_HANDLE;

    std::unique_ptr<PipelineState> Pipeline;

    // FrameBuffer associated data for offscreen rendering.
    VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
    std::unique_ptr<offloadtest::Texture> RenderTarget;
    std::unique_ptr<offloadtest::Buffer> RTReadback;
    std::unique_ptr<offloadtest::Texture> DepthStencil;
    std::optional<ResourceRef> VertexBuffer = std::nullopt;

    uint32_t ShaderStageMask = 0;

    llvm::SmallVector<ResourceBundle> Resources;
    llvm::SmallVector<VkDescriptorSet> DescriptorSets;
    llvm::SmallVector<VkBufferView> BufferViews;
    llvm::SmallVector<VkImageView> ImageViews;
  };

public:
  static llvm::Expected<std::unique_ptr<VulkanDevice>>
  create(std::shared_ptr<VulkanInstance> Instance,
         VkPhysicalDevice PhysicalDevice,
         llvm::SmallVector<VkLayerProperties, 0> InstanceLayers) {
    VkPhysicalDeviceProperties Props;
    vkGetPhysicalDeviceProperties(PhysicalDevice, &Props);

    // Find a queue family that supports both graphics and compute.
    uint32_t QueueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueCount,
                                             nullptr);
    if (QueueCount == 0)
      return llvm::createStringError(std::errc::no_such_device,
                                     "No queue families reported.");

    const std::unique_ptr<VkQueueFamilyProperties[]> QueueFamilyProps(
        new VkQueueFamilyProperties[QueueCount]);
    vkGetPhysicalDeviceQueueFamilyProperties(PhysicalDevice, &QueueCount,
                                             QueueFamilyProps.get());

    std::optional<uint32_t> SelectedIdx;
    for (uint32_t I = 0; I < QueueCount; ++I) {
      const VkQueueFlags Flags = QueueFamilyProps[I].queueFlags;
      // Prefer family supporting both GRAPHICS and COMPUTE
      if ((Flags & VK_QUEUE_GRAPHICS_BIT) && (Flags & VK_QUEUE_COMPUTE_BIT)) {
        SelectedIdx = static_cast<int>(I);
        break;
      }
    }

    if (!SelectedIdx)
      return llvm::createStringError(std::errc::no_such_device,
                                     "No suitable queue family found.");

    const uint32_t QueueFamilyIdx = *SelectedIdx;

    VkDeviceQueueCreateInfo QueueInfo = {};
    const float QueuePriority = 1.0f;
    QueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    QueueInfo.queueFamilyIndex = QueueFamilyIdx;
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
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &Features);

    DeviceInfo.pEnabledFeatures = &Features.features;
    DeviceInfo.pNext = Features.pNext;

    VkDevice Device = VK_NULL_HANDLE;
    if (auto Err = VK::toError(
            vkCreateDevice(PhysicalDevice, &DeviceInfo, nullptr, &Device),
            "Could not create Vulkan logical device."))
      return Err;
    VkQueue DeviceQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(Device, QueueFamilyIdx, 0, &DeviceQueue);

    auto SubmitFenceOrErr = VulkanFence::create(Device, "QueueSubmitFence");
    if (!SubmitFenceOrErr)
      return SubmitFenceOrErr.takeError();
    VulkanQueue GraphicsQueue(DeviceQueue, QueueFamilyIdx, Device,
                              std::move(*SubmitFenceOrErr));

    return std::make_unique<VulkanDevice>(Instance, PhysicalDevice, Props,
                                          Device, std::move(GraphicsQueue),
                                          std::move(InstanceLayers));
  }

  VulkanDevice(std::shared_ptr<VulkanInstance> I, VkPhysicalDevice P,
               VkPhysicalDeviceProperties Props, VkDevice D, VulkanQueue Q,
               llvm::SmallVector<VkLayerProperties, 0> InstanceLayers)
      : Instance(I), PhysicalDevice(P), Props(Props), Device(D),
        GraphicsQueue(std::move(Q)), InstanceLayers(std::move(InstanceLayers)) {
    const uint64_t DeviceNameSz =
        strnlen(Props.deviceName, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
    Description = std::string(Props.deviceName, DeviceNameSz);

    FloatControlProp.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT_CONTROLS_PROPERTIES;
    FloatControlProp.pNext = nullptr;

    DriverProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    DriverProps.pNext = &FloatControlProp;

    Props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    Props2.pNext = &DriverProps;
    vkGetPhysicalDeviceProperties2(PhysicalDevice, &Props2);

    const uint64_t DriverNameSz =
        strnlen(DriverProps.driverName, VK_MAX_DRIVER_NAME_SIZE);
    DriverName = std::string(DriverProps.driverName, DriverNameSz);
#if defined(__APPLE__) && defined(__aarch64__)
    // Apple silicon Macs may have multiple Vulkan drivers sharing one device
    // name. Include the driver name in the description to enable
    // adapter-regex matching.
    Description += " (" + DriverName + ")";
#endif

    DeviceExtensions = queryDeviceExtensions(PhysicalDevice);
  }
  VulkanDevice(const VulkanDevice &) = delete;

  ~VulkanDevice() override {
    if (Device != VK_NULL_HANDLE) {
      vkDeviceWaitIdle(Device);
      // Release in-flight command buffers before destroying the device,
      // since their destructors call vkDestroyCommandPool on the VkDevice.
      GraphicsQueue.InFlightBatches.clear();
      // Destroy the queue's fence before the device, since the fence
      // references the VkDevice for vkDestroySemaphore.
      GraphicsQueue.SubmitFence.reset();
      vkDestroyDevice(Device, nullptr);
    }
  }

  llvm::StringRef getAPIName() const override { return "Vulkan"; }
  GPUAPI getAPI() const override { return GPUAPI::Vulkan; }

  Queue &getGraphicsQueue() override { return GraphicsQueue; }

  llvm::Error
  createPipelineLayout(const BindingsDesc &BindingsDesc,
                       VkShaderStageFlags StageFlags,
                       llvm::SmallVectorImpl<VkDescriptorSetLayout> &SetLayouts,
                       VkPipelineLayout &PipelineLayout) {
    assert(SetLayouts.empty() && "Output vector SetLayouts must be empty.");

    // Build descriptor set layouts from BindingsDesc.
    for (const DescriptorSetLayoutDesc &SetDesc :
         BindingsDesc.DescriptorSetDescs) {
      std::vector<VkDescriptorSetLayoutBinding> Binds;
      for (const ResourceBindingDesc &RB : SetDesc.ResourceBindings) {
        const VulkanBinding VKBinding = RB.VKBinding.value();

        VkDescriptorSetLayoutBinding B = {};
        B.binding = VKBinding.Binding;
        B.descriptorType = getDescriptorType(RB.Kind);
        B.descriptorCount = RB.DescriptorCount;
        B.stageFlags = StageFlags;
        Binds.push_back(B);

        if (VKBinding.CounterBinding) {
          VkDescriptorSetLayoutBinding CB = {};
          CB.binding = *VKBinding.CounterBinding;
          CB.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          CB.descriptorCount = RB.DescriptorCount;
          CB.stageFlags = StageFlags;
          Binds.push_back(CB);
        }
      }
      VkDescriptorSetLayoutCreateInfo SetCI = {};
      SetCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      SetCI.bindingCount = static_cast<uint32_t>(Binds.size());
      SetCI.pBindings = Binds.data();
      VkDescriptorSetLayout SetLayout = VK_NULL_HANDLE;
      if (auto Err = VK::toError(
              vkCreateDescriptorSetLayout(Device, &SetCI, nullptr, &SetLayout),
              "Failed to create descriptor set layout.")) {
        for (auto *L : SetLayouts)
          vkDestroyDescriptorSetLayout(Device, L, nullptr);
        return Err;
      }
      SetLayouts.push_back(SetLayout);
    }

    llvm::SmallVector<VkPushConstantRange> Ranges;
    for (const auto &PCR : BindingsDesc.PushConstantRanges) {
      const VkPushConstantRange R = {
          static_cast<VkShaderStageFlags>(StageFlags), PCR.OffsetInBytes,
          PCR.SizeInBytes};
      Ranges.emplace_back(std::move(R));
    }

    VkPipelineLayoutCreateInfo LayoutCI = {};
    LayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    LayoutCI.setLayoutCount = static_cast<uint32_t>(SetLayouts.size());
    LayoutCI.pSetLayouts = SetLayouts.empty() ? nullptr : SetLayouts.data();
    LayoutCI.pushConstantRangeCount = static_cast<uint32_t>(Ranges.size());
    LayoutCI.pPushConstantRanges = Ranges.empty() ? nullptr : Ranges.data();
    if (auto Err = VK::toError(
            vkCreatePipelineLayout(Device, &LayoutCI, nullptr, &PipelineLayout),
            "Failed to create pipeline layout.")) {
      for (auto *L : SetLayouts)
        vkDestroyDescriptorSetLayout(Device, L, nullptr);
      return Err;
    }

    return llvm::Error::success();
  }

  llvm::Expected<VkShaderModule>
  createShaderModule(const llvm::MemoryBuffer *Shader, const char *Kind) {
    const llvm::StringRef Bytecode = Shader->getBuffer();
    VkShaderModuleCreateInfo ModuleCI = {};
    ModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ModuleCI.codeSize = Bytecode.size();
    ModuleCI.pCode = reinterpret_cast<const uint32_t *>(Bytecode.data());
    VkShaderModule Module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(Device, &ModuleCI, nullptr, &Module))
      return llvm::createStringError(
          std::errc::not_supported, "Failed to create %s shader module.", Kind);
    return Module;
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                   ShaderContainer CS) override {
    llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
    VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
    if (auto Err =
            createPipelineLayout(BindingsDesc, VK_SHADER_STAGE_COMPUTE_BIT,
                                 SetLayouts, PipelineLayout))
      return Err;

    auto CleanupState = llvm::scope_exit([&]() {
      for (auto &Layout : SetLayouts)
        vkDestroyDescriptorSetLayout(Device, Layout, nullptr);
    });

    // Create compute shader module.
    auto CSModOrErr = createShaderModule(CS.Shader, "compute");
    if (!CSModOrErr) {
      vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
      return CSModOrErr.takeError();
    }
    VkShaderModule CSModule = *CSModOrErr;

    VkPipelineShaderStageCreateInfo StageCI = {};
    StageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    StageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    StageCI.module = CSModule;
    StageCI.pName = CS.EntryPoint.c_str();

    llvm::SmallVector<VkSpecializationMapEntry> SpecEntries;
    llvm::SmallVector<char> SpecData;
    VkSpecializationInfo SpecInfo = {};
    if (!CS.SpecializationConstants.empty()) {
      llvm::DenseSet<uint32_t> SeenConstantIDs;

      for (const auto &SpecConst : CS.SpecializationConstants) {
        if (!SeenConstantIDs.insert(SpecConst.ConstantID).second)
          return llvm::createStringError(
              std::errc::invalid_argument,
              "Test configuration contains multiple entries for "
              "specialization constant ID %u.",
              SpecConst.ConstantID);

        VkSpecializationMapEntry Entry;
        if (auto Err =
                parseSpecializationConstant(SpecConst, Entry, SpecData)) {
          vkDestroyShaderModule(Device, CSModule, nullptr);
          vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
          return Err;
        }
        SpecEntries.push_back(Entry);
      }
      SpecInfo.mapEntryCount = SpecEntries.size();
      SpecInfo.pMapEntries = SpecEntries.data();
      SpecInfo.dataSize = SpecData.size();
      SpecInfo.pData = SpecData.data();
      StageCI.pSpecializationInfo = &SpecInfo;
    }

    VkComputePipelineCreateInfo PipelineCI = {};
    PipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    PipelineCI.stage = StageCI;
    PipelineCI.layout = PipelineLayout;
    VkPipeline Pipeline = VK_NULL_HANDLE;
    if (auto Err = VK::toError(vkCreateComputePipelines(Device, VK_NULL_HANDLE,
                                                        1, &PipelineCI, nullptr,
                                                        &Pipeline),
                               "Failed to create compute pipeline.")) {
      vkDestroyShaderModule(Device, CSModule, nullptr);
      vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
      return Err;
    }

    // No longer need shader modules after pipeline compilation.
    vkDestroyShaderModule(Device, CSModule, nullptr);

    return std::make_unique<VulkanPipelineState>(
        Name, Device, Pipeline, PipelineLayout, std::move(SetLayouts),
        VK_NULL_HANDLE);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineVsPs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                     llvm::ArrayRef<InputLayoutDesc> InputLayout,
                     llvm::ArrayRef<Format> RTFormats,
                     std::optional<Format> DSFormat, ShaderContainer VS,
                     ShaderContainer PS) override {
    const VkShaderStageFlags GraphicsFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
    VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
    if (auto Err = createPipelineLayout(BindingsDesc, GraphicsFlags, SetLayouts,
                                        PipelineLayout))
      return Err;

    auto RenderPassOrErr = createRenderPass(RTFormats, DSFormat);
    if (!RenderPassOrErr) {
      vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
      for (auto *L : SetLayouts)
        vkDestroyDescriptorSetLayout(Device, L, nullptr);
      return RenderPassOrErr.takeError();
    }
    VkRenderPass RenderPass = *RenderPassOrErr;
    llvm::outs() << "Render pass created.\n";

    std::vector<VkShaderModule> ShaderModules;
    auto VSModOrErr = createShaderModule(VS.Shader, "vertex");
    if (!VSModOrErr) {
      vkDestroyRenderPass(Device, RenderPass, nullptr);
      vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
      for (auto *L : SetLayouts)
        vkDestroyDescriptorSetLayout(Device, L, nullptr);
      return VSModOrErr.takeError();
    }
    ShaderModules.push_back(*VSModOrErr);

    auto PSModOrErr = createShaderModule(PS.Shader, "pixel");
    if (!PSModOrErr) {
      for (auto *M : ShaderModules)
        vkDestroyShaderModule(Device, M, nullptr);
      vkDestroyRenderPass(Device, RenderPass, nullptr);
      vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
      for (auto *L : SetLayouts)
        vkDestroyDescriptorSetLayout(Device, L, nullptr);
      return PSModOrErr.takeError();
    }
    ShaderModules.push_back(*PSModOrErr);

    // Build specialization info for vertex shader.
    llvm::SmallVector<VkSpecializationMapEntry> VSSpecEntries;
    llvm::SmallVector<char> VSSpecData;
    VkSpecializationInfo VSSpecInfo = {};
    if (!VS.SpecializationConstants.empty()) {
      llvm::DenseSet<uint32_t> SeenConstantIDs;
      for (const auto &SpecConst : VS.SpecializationConstants) {
        if (!SeenConstantIDs.insert(SpecConst.ConstantID).second)
          return llvm::createStringError(
              std::errc::invalid_argument,
              "Test configuration contains multiple entries for "
              "specialization constant ID %u.",
              SpecConst.ConstantID);

        VkSpecializationMapEntry Entry;
        if (auto Err =
                parseSpecializationConstant(SpecConst, Entry, VSSpecData)) {
          for (auto *M : ShaderModules)
            vkDestroyShaderModule(Device, M, nullptr);
          vkDestroyRenderPass(Device, RenderPass, nullptr);
          vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
          for (auto *L : SetLayouts)
            vkDestroyDescriptorSetLayout(Device, L, nullptr);
          return Err;
        }
        VSSpecEntries.push_back(Entry);
      }
      VSSpecInfo.mapEntryCount = VSSpecEntries.size();
      VSSpecInfo.pMapEntries = VSSpecEntries.data();
      VSSpecInfo.dataSize = VSSpecData.size();
      VSSpecInfo.pData = VSSpecData.data();
    }

    // Build specialization info for pixel/fragment shader.
    llvm::SmallVector<VkSpecializationMapEntry> PSSpecEntries;
    llvm::SmallVector<char> PSSpecData;
    VkSpecializationInfo PSSpecInfo = {};
    if (!PS.SpecializationConstants.empty()) {
      llvm::DenseSet<uint32_t> SeenConstantIDs;
      for (const auto &SpecConst : PS.SpecializationConstants) {
        if (!SeenConstantIDs.insert(SpecConst.ConstantID).second)
          return llvm::createStringError(
              std::errc::invalid_argument,
              "Test configuration contains multiple entries for "
              "specialization constant ID %u.",
              SpecConst.ConstantID);

        VkSpecializationMapEntry Entry;
        if (auto Err =
                parseSpecializationConstant(SpecConst, Entry, PSSpecData)) {
          for (auto *M : ShaderModules)
            vkDestroyShaderModule(Device, M, nullptr);
          vkDestroyRenderPass(Device, RenderPass, nullptr);
          vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
          for (auto *L : SetLayouts)
            vkDestroyDescriptorSetLayout(Device, L, nullptr);
          return Err;
        }
        PSSpecEntries.push_back(Entry);
      }
      PSSpecInfo.mapEntryCount = PSSpecEntries.size();
      PSSpecInfo.pMapEntries = PSSpecEntries.data();
      PSSpecInfo.dataSize = PSSpecData.size();
      PSSpecInfo.pData = PSSpecData.data();
    }

    const std::array<VkPipelineShaderStageCreateInfo, 2> Stages = {{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_VERTEX_BIT, ShaderModules[0], VS.EntryPoint.c_str(),
         VS.SpecializationConstants.empty() ? nullptr : &VSSpecInfo},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
         VK_SHADER_STAGE_FRAGMENT_BIT, ShaderModules[1], PS.EntryPoint.c_str(),
         PS.SpecializationConstants.empty() ? nullptr : &PSSpecInfo},
    }};

    // Build vertex input attribute and binding descriptions from InputLayout.
    uint32_t Stride = 0;
    std::vector<VkVertexInputAttributeDescription> Attributes;
    Attributes.reserve(InputLayout.size());
    for (uint32_t I = 0; I < static_cast<uint32_t>(InputLayout.size()); ++I) {
      const InputLayoutDesc &Elem = InputLayout[I];
      assert(!Elem.InstanceStepRate &&
             "Instance step rate is currently not supported.");

      const uint32_t ElemSize = getFormatSizeInBytes(Elem.Fmt);
      VkVertexInputAttributeDescription Attr = {};
      Attr.location = I;
      Attr.binding = 0;
      Attr.format = getVulkanFormat(Elem.Fmt);
      Attr.offset = Elem.OffsetInBytes;
      Attributes.push_back(Attr);
      Stride = std::max(Stride, Elem.OffsetInBytes + ElemSize);
    }

    VkVertexInputBindingDescription BindingDesc = {};
    BindingDesc.binding = 0;
    BindingDesc.stride = Stride;
    BindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo VertexInputCI = {};
    VertexInputCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputCI.vertexBindingDescriptionCount = InputLayout.empty() ? 0 : 1;
    VertexInputCI.pVertexBindingDescriptions =
        InputLayout.empty() ? nullptr : &BindingDesc;
    VertexInputCI.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(Attributes.size());
    VertexInputCI.pVertexAttributeDescriptions =
        Attributes.empty() ? nullptr : Attributes.data();

    VkPipelineInputAssemblyStateCreateInfo InputAssemblyCI = {};
    InputAssemblyCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo ViewportCI = {};
    ViewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportCI.viewportCount = 1;
    ViewportCI.scissorCount = 1;

    const VkDynamicState DynStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                        VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo DynamicCI = {};
    DynamicCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicCI.dynamicStateCount = 2;
    DynamicCI.pDynamicStates = DynStates;

    VkPipelineRasterizationStateCreateInfo RastCI = {};
    RastCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    RastCI.polygonMode = VK_POLYGON_MODE_FILL;
    RastCI.cullMode = VK_CULL_MODE_NONE;
    RastCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    RastCI.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo MultisampleCI = {};
    MultisampleCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    MultisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo DepthStencilCI = {};
    DepthStencilCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilCI.depthTestEnable = VK_TRUE;
    DepthStencilCI.depthWriteEnable = VK_TRUE;
    DepthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    DepthStencilCI.back.failOp = VK_STENCIL_OP_KEEP;
    DepthStencilCI.back.passOp = VK_STENCIL_OP_KEEP;
    DepthStencilCI.back.compareOp = VK_COMPARE_OP_ALWAYS;
    DepthStencilCI.front = DepthStencilCI.back;

    llvm::SmallVector<VkPipelineColorBlendAttachmentState> BlendAttachments(
        RTFormats.size());
    for (auto &BA : BlendAttachments)
      BA.colorWriteMask = 0xf;
    VkPipelineColorBlendStateCreateInfo BlendCI = {};
    BlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    BlendCI.attachmentCount = static_cast<uint32_t>(BlendAttachments.size());
    BlendCI.pAttachments = BlendAttachments.data();

    VkGraphicsPipelineCreateInfo PipelineCI = {};
    PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineCI.stageCount = static_cast<uint32_t>(Stages.size());
    PipelineCI.pStages = Stages.data();
    PipelineCI.pVertexInputState = &VertexInputCI;
    PipelineCI.pInputAssemblyState = &InputAssemblyCI;
    PipelineCI.pViewportState = &ViewportCI;
    PipelineCI.pRasterizationState = &RastCI;
    PipelineCI.pMultisampleState = &MultisampleCI;
    PipelineCI.pDepthStencilState = &DepthStencilCI;
    PipelineCI.pColorBlendState = &BlendCI;
    PipelineCI.pDynamicState = &DynamicCI;
    PipelineCI.layout = PipelineLayout;
    PipelineCI.renderPass = RenderPass;

    VkPipeline Pipeline = VK_NULL_HANDLE;
    if (auto Err = VK::toError(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE,
                                                         1, &PipelineCI,
                                                         nullptr, &Pipeline),
                               "Failed to create graphics pipeline.")) {
      for (auto *M : ShaderModules)
        vkDestroyShaderModule(Device, M, nullptr);
      vkDestroyRenderPass(Device, RenderPass, nullptr);
      vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
      for (auto *L : SetLayouts)
        vkDestroyDescriptorSetLayout(Device, L, nullptr);
      return Err;
    }

    // No longer need shader modules after pipeline compilation.
    for (auto *M : ShaderModules)
      vkDestroyShaderModule(Device, M, nullptr);

    return std::make_unique<VulkanPipelineState>(
        Name, Device, Pipeline, PipelineLayout, std::move(SetLayouts),
        RenderPass);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override {
    return VulkanFence::create(Device, Name);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, BufferCreateDesc &Desc,
               size_t SizeInBytes) override {
    VkBufferCreateInfo BufInfo = {};
    BufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufInfo.size = SizeInBytes;
    BufInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    BufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer DeviceBuffer;
    if (auto Err = VK::toError(
            vkCreateBuffer(Device, &BufInfo, nullptr, &DeviceBuffer),
            "Failed to create device buffer."))
      return Err;

    VkMemoryRequirements MemReqs;
    vkGetBufferMemoryRequirements(Device, DeviceBuffer, &MemReqs);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;
    auto MemIdx = getMemoryIndex(PhysicalDevice, MemReqs.memoryTypeBits,
                                 getVulkanMemoryFlags(Desc.Location));
    if (!MemIdx)
      return MemIdx.takeError();
    AllocInfo.memoryTypeIndex = *MemIdx;

    VkDeviceMemory DeviceMemory;
    if (auto Err = VK::toError(
            vkAllocateMemory(Device, &AllocInfo, nullptr, &DeviceMemory),
            "Failed to allocate device memory."))
      return Err;
    if (auto Err = VK::toError(
            vkBindBufferMemory(Device, DeviceBuffer, DeviceMemory, 0),
            "Failed to bind device buffer memory."))
      return Err;

    return std::make_unique<VulkanBuffer>(Device, DeviceBuffer, DeviceMemory,
                                          Name, Desc, SizeInBytes);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Texture>>
  createTexture(std::string Name, TextureCreateDesc &Desc) override {
    if (auto Err = validateTextureCreateDesc(Desc))
      return Err;

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.format = getVulkanFormat(Desc.Fmt);
    ImageInfo.extent = {Desc.Width, Desc.Height, 1};
    ImageInfo.mipLevels = Desc.MipLevels;
    ImageInfo.arrayLayers = 1;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.usage = getVulkanImageUsage(Desc.Usage);
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImage Image;
    if (auto Err =
            VK::toError(vkCreateImage(Device, &ImageInfo, nullptr, &Image),
                        "Failed to create image."))
      return Err;

    VkMemoryRequirements MemReqs;
    vkGetImageMemoryRequirements(Device, Image, &MemReqs);

    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;
    auto MemIdx = getMemoryIndex(PhysicalDevice, MemReqs.memoryTypeBits,
                                 getVulkanMemoryFlags(Desc.Location));
    if (!MemIdx) {
      vkDestroyImage(Device, Image, nullptr);
      return MemIdx.takeError();
    }
    AllocInfo.memoryTypeIndex = *MemIdx;

    VkDeviceMemory DeviceMemory;
    if (auto Err = VK::toError(
            vkAllocateMemory(Device, &AllocInfo, nullptr, &DeviceMemory),
            "Failed to allocate image memory.")) {
      vkDestroyImage(Device, Image, nullptr);
      return Err;
    }
    if (auto Err =
            VK::toError(vkBindImageMemory(Device, Image, DeviceMemory, 0),
                        "Failed to bind image memory.")) {
      vkDestroyImage(Device, Image, nullptr);
      vkFreeMemory(Device, DeviceMemory, nullptr);
      return Err;
    }

    auto Tex = std::make_unique<VulkanTexture>(Device, Image, DeviceMemory,
                                               Name, Desc);

    const bool IsRT = (Desc.Usage & TextureUsage::RenderTarget) != 0;
    const bool IsDS = (Desc.Usage & TextureUsage::DepthStencil) != 0;
    if (IsRT || IsDS) {
      VkImageViewCreateInfo ViewCi = {};
      ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ViewCi.format = getVulkanFormat(Desc.Fmt);
      ViewCi.subresourceRange.baseMipLevel = 0;
      ViewCi.subresourceRange.levelCount = 1;
      ViewCi.subresourceRange.baseArrayLayer = 0;
      ViewCi.subresourceRange.layerCount = 1;
      ViewCi.image = Image;
      if (IsRT) {
        ViewCi.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                             VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        ViewCi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      } else {
        ViewCi.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
      }
      // Tex destructor will clean up Image + Memory on failure.
      if (auto Err = VK::toError(
              vkCreateImageView(Device, &ViewCi, nullptr, &Tex->View),
              "Failed to create image view."))
        return Err;
    }

    return Tex;
  }

  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      queryCapabilities();
    return Caps;
  }

  void printExtra(llvm::raw_ostream &OS) override {
    OS << "  Layers:\n";
    for (auto &Layer : InstanceLayers) {
      uint64_t Sz = strnlen(Layer.layerName, VK_MAX_EXTENSION_NAME_SIZE);
      OS << "  - LayerName: " << llvm::StringRef(Layer.layerName, Sz) << "\n";
      OS << "    SpecVersion: " << Layer.specVersion << "\n";
      OS << "    ImplVersion: " << Layer.implementationVersion << "\n";
      Sz = strnlen(Layer.description, VK_MAX_DESCRIPTION_SIZE);
      OS << "    LayerDesc: " << llvm::StringRef(Layer.description, Sz) << "\n";
    }

    OS << "  Extensions:\n";
    for (const auto &Ext : DeviceExtensions) {
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
    vkGetPhysicalDeviceFeatures2(PhysicalDevice, &Features);

    Caps.insert(std::make_pair(
        "APIMajorVersion",
        makeCapability<uint32_t>("APIMajorVersion",
                                 VK_API_VERSION_MAJOR(Props.apiVersion))));

    Caps.insert(std::make_pair(
        "APIMinorVersion",
        makeCapability<uint32_t>("APIMinorVersion",
                                 VK_API_VERSION_MINOR(Props.apiVersion))));

#define VULKAN_FLOAT_CONTROLS_FEATURE_BOOL(Name)                               \
  Caps.insert(std::make_pair(                                                  \
      #Name, makeCapability<bool>(#Name, FloatControlProp.Name)));
#define VULKAN_FEATURE_BOOL(Name)                                              \
  Caps.insert(std::make_pair(                                                  \
      #Name, makeCapability<bool>(#Name, Features.features.Name)));
#define VULKAN11_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, makeCapability<bool>(#Name, Features11.Name)));
#define VULKAN12_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, makeCapability<bool>(#Name, Features12.Name)));
#define VULKAN13_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, makeCapability<bool>(#Name, Features13.Name)));
#ifdef VK_VERSION_1_4
#define VULKAN14_FEATURE_BOOL(Name)                                            \
  Caps.insert(                                                                 \
      std::make_pair(#Name, makeCapability<bool>(#Name, Features14.Name)));
#endif
#include "VKFeatures.def"
  }

public:
  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override {
    return VulkanCommandBuffer::create(Device, GraphicsQueue.QueueFamilyIdx);
  }

  llvm::Expected<BufferRef> createBuffer(VkBufferUsageFlags Usage,
                                         VkMemoryPropertyFlags MemoryFlags,
                                         size_t Size, void *Data = nullptr) {
    VkBuffer Buffer;
    VkDeviceMemory Memory;
    VkBufferCreateInfo BufferInfo = {};
    BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size = Size;
    BufferInfo.usage = Usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (auto Err =
            VK::toError(vkCreateBuffer(Device, &BufferInfo, nullptr, &Buffer),
                        "Could not create buffer."))
      return Err;

    VkMemoryRequirements MemReqs;
    vkGetBufferMemoryRequirements(Device, Buffer, &MemReqs);
    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;

    llvm::Expected<uint32_t> MemIdx =
        getMemoryIndex(PhysicalDevice, MemReqs.memoryTypeBits, MemoryFlags);
    if (!MemIdx)
      return MemIdx.takeError();

    AllocInfo.memoryTypeIndex = *MemIdx;

    if (auto Err =
            VK::toError(vkAllocateMemory(Device, &AllocInfo, nullptr, &Memory),
                        "Memory allocation failed."))
      return Err;
    if (Data) {
      void *Dst = nullptr;
      if (auto Err = VK::toError(
              vkMapMemory(Device, Memory, 0, VK_WHOLE_SIZE, 0, &Dst),
              "Failed to map memory."))
        return Err;
      memcpy(Dst, Data, Size);

      VkMappedMemoryRange Range = {};
      Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      Range.memory = Memory;
      Range.offset = 0;
      Range.size = VK_WHOLE_SIZE;
      vkFlushMappedMemoryRanges(Device, 1, &Range);

      vkUnmapMemory(Device, Memory);
    }

    if (auto Err = VK::toError(vkBindBufferMemory(Device, Buffer, Memory, 0),
                               "Failed to bind buffer to memory."))
      return Err;

    return BufferRef{Buffer, Memory};
  }

  llvm::Expected<ResourceRef> createImage(Resource &R, BufferRef &Host,
                                          int UsageOverride = 0) {
    const offloadtest::CPUBuffer &B = *R.BufferPtr;
    if (B.Format == DataFormat::Depth32 && R.isReadWrite())
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Image memory allocation failed.");
    VkImageCreateInfo ImageCreateInfo = {};
    ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCreateInfo.imageType = getVKImageType(R.Kind);
    ImageCreateInfo.format = getVKFormat(B.Format, B.Channels);
    ImageCreateInfo.mipLevels = B.OutputProps.MipLevels;
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
    if (auto Err = VK::toError(
            vkCreateImage(Device, &ImageCreateInfo, nullptr, &Image),
            "Failed to create image."))
      return Err;

    VkSampler Sampler = 0;

    VkMemoryRequirements MemReqs;
    vkGetImageMemoryRequirements(Device, Image, &MemReqs);
    VkMemoryAllocateInfo AllocInfo = {};
    AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocInfo.allocationSize = MemReqs.size;

    VkDeviceMemory Memory;
    if (auto Err =
            VK::toError(vkAllocateMemory(Device, &AllocInfo, nullptr, &Memory),
                        "Image memory allocation failed."))
      return Err;
    if (auto Err = VK::toError(vkBindImageMemory(Device, Image, Memory, 0),
                               "Image memory binding failed."))
      return Err;

    return ResourceRef(Host, ImageRef{Image, Sampler, Memory});
  }

  llvm::Expected<ResourceRef> createSampler(Resource &R, BufferRef &Host) {
    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    const Sampler &S = *R.SamplerPtr;
    SamplerInfo.magFilter = getVKFilter(S.MagFilter);
    SamplerInfo.minFilter = getVKFilter(S.MinFilter);
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    SamplerInfo.addressModeU = getVKAddressMode(S.Address);
    SamplerInfo.addressModeV = getVKAddressMode(S.Address);
    SamplerInfo.addressModeW = getVKAddressMode(S.Address);
    SamplerInfo.mipLodBias = S.MipLODBias;
    SamplerInfo.anisotropyEnable = VK_FALSE;
    SamplerInfo.maxAnisotropy = 1.0f;
    SamplerInfo.compareEnable =
        S.Kind == SamplerKind::SamplerComparison ? VK_TRUE : VK_FALSE;
    SamplerInfo.compareOp = getVKCompareOp(S.ComparisonOp);
    SamplerInfo.minLod = S.MinLOD;
    SamplerInfo.maxLod = S.MaxLOD;
    SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;

    VkSampler Sampler;
    if (auto Err = VK::toError(
            vkCreateSampler(Device, &SamplerInfo, nullptr, &Sampler),
            "Failed to create sampler."))
      return Err;

    return ResourceRef(Host, ImageRef{0, Sampler, 0});
  }

  llvm::Error createResource(Resource &R, InvocationState &IS) {
    // Samplers don't have backing data buffers, so handle them separately
    if (R.isSampler()) {
      ResourceBundle Bundle{getDescriptorType(R.Kind), 0, nullptr};
      BufferRef HostBuf = {0, 0};
      auto ExSamplerRef = createSampler(R, HostBuf);
      if (!ExSamplerRef)
        return ExSamplerRef.takeError();
      Bundle.ResourceRefs.push_back(*ExSamplerRef);
      IS.Resources.push_back(Bundle);
      return llvm::Error::success();
    }

    ResourceBundle Bundle{getDescriptorType(R.Kind), R.size(), R.BufferPtr};
    for (auto &ResData : R.BufferPtr->Data) {
      auto ExHostBuf = createBuffer(
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, R.size(), ResData.get());
      if (!ExHostBuf)
        return ExHostBuf.takeError();

      if (R.isTexture()) {
        auto ExImageRef = createImage(R, *ExHostBuf);
        if (!ExImageRef)
          return ExImageRef.takeError();

        // Sampled textures use combined-image-sampler descriptors and need
        // both valid image and sampler handles.
        if (R.isSampledTexture()) {
          BufferRef NullHost = {0, 0};
          auto ExSamplerRef = createSampler(R, NullHost);
          if (!ExSamplerRef)
            return ExSamplerRef.takeError();
          ExImageRef->Image.Sampler = ExSamplerRef->Image.Sampler;
        }

        Bundle.ResourceRefs.push_back(*ExImageRef);
      } else {
        auto ExDeviceBuf = createBuffer(
            getFlagBits(R.Kind) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, R.size());
        if (!ExDeviceBuf)
          return ExDeviceBuf.takeError();
        VkBufferCopy Copy = {};
        Copy.size = R.size();
        vkCmdCopyBuffer(IS.CB->CmdBuffer, ExHostBuf->Buffer,
                        ExDeviceBuf->Buffer, 1, &Copy);
        Bundle.ResourceRefs.emplace_back(*ExHostBuf, *ExDeviceBuf);
      }
    }
    if (R.HasCounter) {
      for (uint32_t I = 0; I < R.getArraySize(); ++I) {
        uint32_t CounterValue = 0;
        auto ExHostBuf = createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      sizeof(uint32_t), &CounterValue);
        if (!ExHostBuf)
          return ExHostBuf.takeError();

        auto ExDeviceBuf = createBuffer(
            getFlagBits(R.Kind) | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uint32_t));
        if (!ExDeviceBuf)
          return ExDeviceBuf.takeError();
        VkBufferCopy Copy = {};
        Copy.size = sizeof(uint32_t);
        vkCmdCopyBuffer(IS.CB->CmdBuffer, ExHostBuf->Buffer,
                        ExDeviceBuf->Buffer, 1, &Copy);
        Bundle.CounterResourceRefs.emplace_back(*ExHostBuf, *ExDeviceBuf);
      }
    }
    IS.Resources.push_back(Bundle);
    return llvm::Error::success();
  }

  llvm::Error createRenderTarget(Pipeline &P, InvocationState &IS) {
    if (!P.Bindings.RTargetBufferPtr)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "No render target bound for graphics pipeline.");
    const CPUBuffer &RTBuf = *P.Bindings.RTargetBufferPtr;

    auto TexOrErr = offloadtest::createRenderTargetFromCPUBuffer(*this, RTBuf);
    if (!TexOrErr)
      return TexOrErr.takeError();

    IS.RenderTarget = std::move(*TexOrErr);

    // Create a host-visible staging buffer for readback.
    BufferCreateDesc BufDesc = {};
    BufDesc.Location = MemoryLocation::GpuToCpu;
    auto BufOrErr = createBuffer("RTReadback", BufDesc, RTBuf.size());
    if (!BufOrErr)
      return BufOrErr.takeError();
    IS.RTReadback = std::move(*BufOrErr);

    return llvm::Error::success();
  }

  llvm::Error createDepthStencil(Pipeline &P, InvocationState &IS) {
    auto TexOrErr = offloadtest::createDefaultDepthStencilTarget(
        *this, P.Bindings.RTargetBufferPtr->OutputProps.Width,
        P.Bindings.RTargetBufferPtr->OutputProps.Height);
    if (!TexOrErr)
      return TexOrErr.takeError();
    IS.DepthStencil = std::move(*TexOrErr);
    return llvm::Error::success();
  }

  llvm::Error createResources(Pipeline &P, InvocationState &IS) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        if (auto Err = createResource(R, IS))
          return Err;
      }
    }

    if (P.isTraditionalRaster()) {
      if (auto Err = createRenderTarget(P, IS))
        return Err;
      // TODO: Always created for graphics pipelines. Consider making this
      // conditional on the pipeline definition.
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
      auto ExVHostBuf = createBuffer(
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          VertexBuffer.size(), VertexBuffer.BufferPtr->Data[0].get());
      if (!ExVHostBuf)
        return ExVHostBuf.takeError();
      auto ExDeviceBuf = createBuffer(
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VertexBuffer.size());
      if (!ExDeviceBuf)
        return ExDeviceBuf.takeError();
      VkBufferCopy Copy = {};
      Copy.size = VertexBuffer.size();
      vkCmdCopyBuffer(IS.CB->CmdBuffer, ExVHostBuf->Buffer, ExDeviceBuf->Buffer,
                      1, &Copy);
      IS.VertexBuffer = ResourceRef(*ExVHostBuf, *ExDeviceBuf);
    }

    return llvm::Error::success();
  }

  llvm::Expected<offloadtest::SubmitResult>
  executeCommandBuffer(InvocationState &IS) {
    return GraphicsQueue.submit(std::move(IS.CB));
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
      if (auto Err = VK::toError(vkCreateDescriptorPool(Device, &PoolCreateInfo,
                                                        nullptr, &IS.Pool),
                                 "Failed to create descriptor pool."))
        return Err;
    }
    return llvm::Error::success();
  }

  llvm::Error createDescriptorSets(Pipeline &P, InvocationState &IS) {
    if (P.Sets.size() == 0)
      return llvm::Error::success();

    const VulkanPipelineState &VulkanPipeline =
        llvm::cast<VulkanPipelineState>(*IS.Pipeline.get());

    VkDescriptorSetAllocateInfo DSAllocInfo = {};
    DSAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DSAllocInfo.descriptorPool = IS.Pool;
    DSAllocInfo.descriptorSetCount = VulkanPipeline.SetLayouts.size();
    DSAllocInfo.pSetLayouts = VulkanPipeline.SetLayouts.data();
    assert(IS.DescriptorSets.empty());
    IS.DescriptorSets.insert(IS.DescriptorSets.begin(),
                             VulkanPipeline.SetLayouts.size(),
                             VkDescriptorSet());
    llvm::outs() << "Num Descriptor sets: " << VulkanPipeline.SetLayouts.size()
                 << "\n";
    if (auto Err =
            VK::toError(vkAllocateDescriptorSets(Device, &DSAllocInfo,
                                                 IS.DescriptorSets.data()),
                        "Failed to allocate descriptor sets."))
      return Err;

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
            const VkDescriptorImageInfo ImageInfo = {
                ResRef.Image.Sampler, 0,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
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
              R.BufferPtr->Format == DataFormat::Depth32
                  ? VK_IMAGE_ASPECT_DEPTH_BIT
                  : VK_IMAGE_ASPECT_COLOR_BIT;
          ViewCreateInfo.subresourceRange.baseMipLevel = 0;
          ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
          ViewCreateInfo.subresourceRange.layerCount = 1;
          ViewCreateInfo.subresourceRange.levelCount =
              R.BufferPtr->OutputProps.MipLevels;
          IndexOfFirstBufferDataInArray = ImageInfos.size();
          for (auto &ResRef : IS.Resources[OverallResIdx].ResourceRefs) {
            ViewCreateInfo.image = ResRef.Image.Image;
            VkImageView View = {0};
            if (auto Err = VK::toError(
                    vkCreateImageView(Device, &ViewCreateInfo, nullptr, &View),
                    "Failed to create image view."))
              return Err;
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
            if (auto Err = VK::toError(
                    vkCreateBufferView(Device, &ViewCreateInfo, nullptr, &View),
                    "Failed to create buffer view."))
              return Err;
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
    vkUpdateDescriptorSets(Device, WriteDescriptors.size(),
                           WriteDescriptors.data(), 0, nullptr);
    return llvm::Error::success();
  }

  llvm::Expected<VkRenderPass>
  createRenderPass(llvm::ArrayRef<Format> RTFormats,
                   std::optional<Format> DSFormat) {
    // Only 8 render targets can be bound + 1 depth stencil target.
    llvm::SmallVector<VkAttachmentDescription, 9> Attachments;
    llvm::SmallVector<VkAttachmentReference, 8> ColorReferences;
    for (size_t I = 0, N = RTFormats.size(); I < N; ++I) {
      VkAttachmentDescription AttachmentDesc = {};
      AttachmentDesc.format = getVulkanFormat(RTFormats[I]);
      AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
      AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      AttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      AttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      Attachments.push_back(AttachmentDesc);

      VkAttachmentReference ColorReference = {};
      ColorReference.attachment = I;
      ColorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      ColorReferences.push_back(ColorReference);
    }

    VkAttachmentReference DepthReference = {};
    if (DSFormat.has_value()) {
      VkAttachmentDescription AttachmentDesc = {};
      AttachmentDesc.format = getVulkanFormat(*DSFormat);
      AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
      AttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      AttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      AttachmentDesc.finalLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      Attachments.push_back(AttachmentDesc);

      DepthReference.attachment = Attachments.size() - 1;
      DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription SubpassDescription = {};
    SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    SubpassDescription.colorAttachmentCount = ColorReferences.size();
    SubpassDescription.pColorAttachments = ColorReferences.data();
    SubpassDescription.pDepthStencilAttachment =
        DSFormat.has_value() ? &DepthReference : nullptr;
    SubpassDescription.inputAttachmentCount = 0;
    SubpassDescription.pInputAttachments = nullptr;
    SubpassDescription.preserveAttachmentCount = 0;
    SubpassDescription.pPreserveAttachments = nullptr;
    SubpassDescription.pResolveAttachments = nullptr;

    VkRenderPassCreateInfo RPCI = {};
    RPCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RPCI.attachmentCount = static_cast<uint32_t>(Attachments.size());
    RPCI.pAttachments = Attachments.data();
    RPCI.subpassCount = 1;
    RPCI.pSubpasses = &SubpassDescription;
    // RPCI.dependencyCount = static_cast<uint32_t>(Dependencies.size());
    // RPCI.pDependencies = Dependencies.data();

    VkRenderPass RenderPass = VK_NULL_HANDLE;
    if (auto Err =
            VK::toError(vkCreateRenderPass(Device, &RPCI, nullptr, &RenderPass),
                        "Failed to create render pass."))
      return Err;
    return RenderPass;
  }

  llvm::Error createFrameBuffer(InvocationState &IS) {
    auto &RT = llvm::cast<VulkanTexture>(*IS.RenderTarget);
    auto &DS = llvm::cast<VulkanTexture>(*IS.DepthStencil);
    auto &PipelineState = llvm::cast<VulkanPipelineState>(*IS.Pipeline);

    std::array<VkImageView, 2> Views = {RT.View, DS.View};

    VkFramebufferCreateInfo FbufCreateInfo = {};
    FbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FbufCreateInfo.renderPass = PipelineState.RenderPass;
    FbufCreateInfo.attachmentCount = Views.size();
    FbufCreateInfo.pAttachments = Views.data();
    FbufCreateInfo.width = RT.Desc.Width;
    FbufCreateInfo.height = RT.Desc.Height;
    FbufCreateInfo.layers = 1;

    if (auto Err = VK::toError(vkCreateFramebuffer(Device, &FbufCreateInfo,
                                                   nullptr, &IS.FrameBuffer),
                               "Failed to create frame buffer."))
      return Err;
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

  void copyResourceDataToDevice(InvocationState &IS, ResourceBundle &R) {
    if (R.isSampler())
      return;
    if (R.isImage()) {
      const offloadtest::CPUBuffer &B = *R.BufferPtr;
      llvm::SmallVector<VkBufferImageCopy> Regions;
      uint64_t CurrentOffset = 0;
      for (int I = 0; I < B.OutputProps.MipLevels; ++I) {
        VkBufferImageCopy Region = {};
        Region.imageSubresource.aspectMask = B.Format == DataFormat::Depth32
                                                 ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                 : VK_IMAGE_ASPECT_COLOR_BIT;
        Region.imageSubresource.mipLevel = I;
        Region.imageSubresource.baseArrayLayer = 0;
        Region.imageSubresource.layerCount = 1;
        Region.imageExtent.width =
            std::max(1u, static_cast<uint32_t>(B.OutputProps.Width) >> I);
        Region.imageExtent.height =
            std::max(1u, static_cast<uint32_t>(B.OutputProps.Height) >> I);
        Region.imageExtent.depth =
            std::max(1u, static_cast<uint32_t>(B.OutputProps.Depth) >> I);
        Region.bufferOffset = CurrentOffset;
        Regions.push_back(Region);
        CurrentOffset += static_cast<uint64_t>(Region.imageExtent.width) *
                         Region.imageExtent.height * Region.imageExtent.depth *
                         B.getElementSize();
      }

      VkImageSubresourceRange SubRange = {};
      SubRange.aspectMask = B.Format == DataFormat::Depth32
                                ? VK_IMAGE_ASPECT_DEPTH_BIT
                                : VK_IMAGE_ASPECT_COLOR_BIT;
      SubRange.baseMipLevel = 0;
      SubRange.levelCount = B.OutputProps.MipLevels;
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
        vkCmdPipelineBarrier(IS.CB->CmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &ImageBarrier);

        vkCmdCopyBufferToImage(IS.CB->CmdBuffer, ResRef.Host.Buffer,
                               ResRef.Image.Image, VK_IMAGE_LAYOUT_GENERAL,
                               Regions.size(), Regions.data());
      }

      ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      ImageBarrier.dstAccessMask =
          VK_ACCESS_SHADER_READ_BIT |
          (R.isReadWrite() ? VK_ACCESS_SHADER_WRITE_BIT : 0);
      ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
      ImageBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

      for (auto &ResRef : R.ResourceRefs) {
        ImageBarrier.image = ResRef.Image.Image;
        vkCmdPipelineBarrier(IS.CB->CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
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
      vkCmdPipelineBarrier(IS.CB->CmdBuffer, VK_PIPELINE_STAGE_HOST_BIT,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr,
                           1, &Barrier, 0, nullptr);
    }
  }

  // Record commands to copy a texture into a readback buffer.
  void copyTextureToReadback(VkCommandBuffer CmdBuffer,
                             const VulkanTexture &Tex,
                             const VulkanBuffer &Readback,
                             VkImageLayout OldLayout,
                             VkAccessFlags SrcAccessMask,
                             VkPipelineStageFlags SrcStageMask) {
    const VkImageAspectFlags AspectMask = isDepthFormat(Tex.Desc.Fmt)
                                              ? VK_IMAGE_ASPECT_DEPTH_BIT
                                              : VK_IMAGE_ASPECT_COLOR_BIT;

    // Transition texture to transfer source.
    VkImageSubresourceRange SubRange = {};
    SubRange.aspectMask = AspectMask;
    SubRange.baseMipLevel = 0;
    SubRange.levelCount = 1;
    SubRange.layerCount = 1;

    VkImageMemoryBarrier ImageBarrier = {};
    ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImageBarrier.subresourceRange = SubRange;
    ImageBarrier.srcAccessMask = SrcAccessMask;
    ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    ImageBarrier.oldLayout = OldLayout;
    ImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    ImageBarrier.image = Tex.Image;
    vkCmdPipelineBarrier(CmdBuffer, SrcStageMask,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &ImageBarrier);

    // Copy image to readback buffer.
    VkBufferImageCopy Region = {};
    Region.imageSubresource.aspectMask = AspectMask;
    Region.imageSubresource.mipLevel = 0;
    Region.imageSubresource.baseArrayLayer = 0;
    Region.imageSubresource.layerCount = 1;
    Region.imageExtent.width = Tex.Desc.Width;
    Region.imageExtent.height = Tex.Desc.Height;
    Region.imageExtent.depth = 1;
    vkCmdCopyImageToBuffer(CmdBuffer, Tex.Image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           Readback.Buffer, 1, &Region);

    // Barrier to make the readback buffer visible to the host.
    VkBufferMemoryBarrier BufBarrier = {};
    BufBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    BufBarrier.size = VK_WHOLE_SIZE;
    BufBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    BufBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    BufBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    BufBarrier.buffer = Readback.Buffer;
    vkCmdPipelineBarrier(CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                         &BufBarrier, 0, nullptr);
  }

  void copyResourceDataToHost(InvocationState &IS, ResourceBundle &R) {
    if (!R.isReadWrite())
      return;
    if (R.isImage()) {
      const offloadtest::CPUBuffer &B = *R.BufferPtr;
      VkImageSubresourceRange SubRange = {};
      SubRange.aspectMask = B.Format == DataFormat::Depth32
                                ? VK_IMAGE_ASPECT_DEPTH_BIT
                                : VK_IMAGE_ASPECT_COLOR_BIT;
      SubRange.baseMipLevel = 0;
      SubRange.levelCount = B.OutputProps.MipLevels;
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
        vkCmdPipelineBarrier(IS.CB->CmdBuffer,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &ImageBarrier);
      }

      llvm::SmallVector<VkBufferImageCopy> Regions;
      uint64_t CurrentOffset = 0;
      for (int I = 0; I < B.OutputProps.MipLevels; ++I) {
        VkBufferImageCopy Region = {};
        Region.imageSubresource.aspectMask = B.Format == DataFormat::Depth32
                                                 ? VK_IMAGE_ASPECT_DEPTH_BIT
                                                 : VK_IMAGE_ASPECT_COLOR_BIT;
        Region.imageSubresource.mipLevel = I;
        Region.imageSubresource.baseArrayLayer = 0;
        Region.imageSubresource.layerCount = 1;
        Region.imageExtent.width =
            std::max(1u, static_cast<uint32_t>(B.OutputProps.Width) >> I);
        Region.imageExtent.height =
            std::max(1u, static_cast<uint32_t>(B.OutputProps.Height) >> I);
        Region.imageExtent.depth =
            std::max(1u, static_cast<uint32_t>(B.OutputProps.Depth) >> I);
        Region.bufferOffset = CurrentOffset;
        Regions.push_back(Region);
        CurrentOffset += static_cast<uint64_t>(Region.imageExtent.width) *
                         Region.imageExtent.height * Region.imageExtent.depth *
                         B.getElementSize();
      }

      for (auto &ResRef : R.ResourceRefs)
        vkCmdCopyImageToBuffer(IS.CB->CmdBuffer, ResRef.Image.Image,
                               VK_IMAGE_LAYOUT_GENERAL, ResRef.Host.Buffer,
                               Regions.size(), Regions.data());

      VkBufferMemoryBarrier Barrier = {};
      Barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      Barrier.size = VK_WHOLE_SIZE;
      Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      Barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      for (auto &ResRef : R.ResourceRefs) {
        Barrier.buffer = ResRef.Host.Buffer;
        vkCmdPipelineBarrier(IS.CB->CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
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
      vkCmdPipelineBarrier(IS.CB->CmdBuffer,
                           VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
    VkBufferCopy CopyRegion = {};
    CopyRegion.size = R.size();
    for (auto &ResRef : R.ResourceRefs)
      vkCmdCopyBuffer(IS.CB->CmdBuffer, ResRef.Device.Buffer,
                      ResRef.Host.Buffer, 1, &CopyRegion);

    VkBufferCopy CounterCopyRegion = {};
    CounterCopyRegion.size = sizeof(uint32_t);
    for (auto &ResRef : R.CounterResourceRefs)
      vkCmdCopyBuffer(IS.CB->CmdBuffer, ResRef.Device.Buffer,
                      ResRef.Host.Buffer, 1, &CounterCopyRegion);

    Barrier.size = VK_WHOLE_SIZE;
    Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    Barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    for (auto &ResRef : R.ResourceRefs) {
      Barrier.buffer = ResRef.Host.Buffer;
      vkCmdPipelineBarrier(IS.CB->CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
    for (auto &ResRef : R.CounterResourceRefs) {
      Barrier.buffer = ResRef.Host.Buffer;
      vkCmdPipelineBarrier(IS.CB->CmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                           &Barrier, 0, nullptr);
    }
  }

  llvm::Error createCommands(Pipeline &P, InvocationState &IS) {
    for (auto &R : IS.Resources)
      copyResourceDataToDevice(IS, R);

    if (P.isTraditionalRaster()) {
      auto &RT = llvm::cast<VulkanTexture>(*IS.RenderTarget);
      auto &DS = llvm::cast<VulkanTexture>(*IS.DepthStencil);
      auto &PipelineState = llvm::cast<VulkanPipelineState>(*IS.Pipeline);

      const auto *ColorCV =
          std::get_if<ClearColor>(&*RT.Desc.OptimizedClearValue);
      if (!ColorCV)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Render target clear value must be a ClearColor.");
      const auto *DepthCV =
          std::get_if<ClearDepthStencil>(&*DS.Desc.OptimizedClearValue);
      if (!DepthCV)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Depth/stencil clear value must be a ClearDepthStencil.");
      VkClearValue ClearValues[2] = {};
      ClearValues[0].color = {{ColorCV->R, ColorCV->G, ColorCV->B, ColorCV->A}};
      ClearValues[1].depthStencil = {DepthCV->Depth, DepthCV->Stencil};

      VkRenderPassBeginInfo RenderPassBeginInfo = {};
      RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      RenderPassBeginInfo.renderPass = PipelineState.RenderPass;
      RenderPassBeginInfo.framebuffer = IS.FrameBuffer;
      RenderPassBeginInfo.renderArea.extent.width =
          P.Bindings.RTargetBufferPtr->OutputProps.Width;
      RenderPassBeginInfo.renderArea.extent.height =
          P.Bindings.RTargetBufferPtr->OutputProps.Height;
      RenderPassBeginInfo.clearValueCount = 2;
      RenderPassBeginInfo.pClearValues = ClearValues;

      vkCmdBeginRenderPass(IS.CB->CmdBuffer, &RenderPassBeginInfo,
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
      vkCmdSetViewport(IS.CB->CmdBuffer, 0, 1, &Viewport);

      VkRect2D Scissor = {};
      Scissor.offset = {0, 0};
      Scissor.extent.width = P.Bindings.RTargetBufferPtr->OutputProps.Width;
      Scissor.extent.height = P.Bindings.RTargetBufferPtr->OutputProps.Height;
      vkCmdSetScissor(IS.CB->CmdBuffer, 0, 1, &Scissor);
    }

    const VkPipelineBindPoint BindPoint = P.isTraditionalRaster()
                                              ? VK_PIPELINE_BIND_POINT_GRAPHICS
                                              : VK_PIPELINE_BIND_POINT_COMPUTE;
    const VulkanPipelineState &VulkanPipeline =
        llvm::cast<VulkanPipelineState>(*IS.Pipeline.get());
    vkCmdBindPipeline(IS.CB->CmdBuffer, BindPoint, VulkanPipeline.Pipeline);
    if (IS.DescriptorSets.size() > 0)
      vkCmdBindDescriptorSets(
          IS.CB->CmdBuffer, BindPoint, VulkanPipeline.Layout, 0,
          IS.DescriptorSets.size(), IS.DescriptorSets.data(), 0, 0);

    for (const auto &PCB : P.PushConstants) {
      llvm::SmallVector<uint8_t, 4> Data;
      PCB.getContent(Data);
      vkCmdPushConstants(IS.CB->CmdBuffer, VulkanPipeline.Layout,
                         getShaderStageFlag(PCB.Stage), 0, Data.size(),
                         Data.data());
    }

    if (P.isCompute()) {
      const llvm::ArrayRef<int> DispatchSize =
          llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
      vkCmdDispatch(IS.CB->CmdBuffer, DispatchSize[0], DispatchSize[1],
                    DispatchSize[2]);
      llvm::outs() << "Dispatched compute shader: { " << DispatchSize[0] << ", "
                   << DispatchSize[1] << ", " << DispatchSize[2] << " }\n";
    } else {
      VkDeviceSize Offsets[1]{0};
      assert(IS.VertexBuffer.has_value());
      vkCmdBindVertexBuffers(IS.CB->CmdBuffer, 0, 1,
                             &IS.VertexBuffer->Device.Buffer, Offsets);
      // instanceCount must be >=1 to draw; previously was 0 which draws nothing
      vkCmdDraw(IS.CB->CmdBuffer, P.Bindings.getVertexCount(), 1, 0, 0);
      llvm::outs() << "Drew " << P.Bindings.getVertexCount() << " vertices.\n";
      vkCmdEndRenderPass(IS.CB->CmdBuffer);
      copyTextureToReadback(IS.CB->CmdBuffer,
                            llvm::cast<VulkanTexture>(*IS.RenderTarget),
                            llvm::cast<VulkanBuffer>(*IS.RTReadback),
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
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
          vkMapMemory(Device, ResRefIt->Host.Memory, 0, VK_WHOLE_SIZE, 0,
                      &Mapped);
          Range.memory = ResRefIt->Host.Memory;
          vkInvalidateMappedMemoryRanges(Device, 1, &Range);
          memcpy(DataIt->get(), Mapped, R.size());
          vkUnmapMemory(Device, ResRefIt->Host.Memory);
        }
        if (R.HasCounter) {
          R.BufferPtr->Counters.clear();
          for (uint32_t I = 0; I < R.getArraySize(); ++I) {
            uint32_t *Mapped = nullptr; // NOLINT(misc-const-correctness)
            auto &CounterRef = IS.Resources[BufIdx].CounterResourceRefs[I];
            vkMapMemory(Device, CounterRef.Host.Memory, 0, VK_WHOLE_SIZE, 0,
                        (void **)&Mapped);
            Range.memory = CounterRef.Host.Memory;
            vkInvalidateMappedMemoryRanges(Device, 1, &Range);
            R.BufferPtr->Counters.push_back(*Mapped);
            vkUnmapMemory(Device, CounterRef.Host.Memory);
          }
        }
      }
    }

    // Copy back the frame buffer data if this was a graphics pipeline.
    if (P.isTraditionalRaster()) {
      auto &Readback = llvm::cast<VulkanBuffer>(*IS.RTReadback);

      VkMappedMemoryRange Range = {};
      Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      Range.offset = 0;
      Range.size = VK_WHOLE_SIZE;
      Range.memory = Readback.Memory;

      void *Mapped = nullptr; // NOLINT(misc-const-correctness)
      vkMapMemory(Device, Readback.Memory, 0, VK_WHOLE_SIZE, 0, &Mapped);
      vkInvalidateMappedMemoryRanges(Device, 1, &Range);

      const CPUBuffer &B = *P.Bindings.RTargetBufferPtr;
      memcpy(B.Data[0].get(), Mapped, B.size());
      vkUnmapMemory(Device, Readback.Memory);
    }
    return llvm::Error::success();
  }

  void cleanup(InvocationState &IS) {
    // Wait for all in-flight submissions to complete before destroying
    // resources. On the happy path the caller already waited, but this
    // handles early-return error paths.
    llvm::consumeError(GraphicsQueue.SubmitFence->waitForCompletion(
        GraphicsQueue.FenceCounter));
    for (auto &V : IS.BufferViews)
      vkDestroyBufferView(Device, V, nullptr);

    for (auto &V : IS.ImageViews)
      vkDestroyImageView(Device, V, nullptr);

    for (auto &R : IS.Resources) {
      for (auto &ResRef : R.ResourceRefs) {
        if (R.isBuffer()) {
          vkDestroyBuffer(Device, ResRef.Device.Buffer, nullptr);
          vkFreeMemory(Device, ResRef.Device.Memory, nullptr);
        } else if (R.isSampler()) {
          vkDestroySampler(Device, ResRef.Image.Sampler, nullptr);
        } else if (R.DescriptorType ==
                   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          vkDestroySampler(Device, ResRef.Image.Sampler, nullptr);
          vkDestroyImage(Device, ResRef.Image.Image, nullptr);
          vkFreeMemory(Device, ResRef.Image.Memory, nullptr);
        } else {
          assert(R.isImage());
          vkDestroyImage(Device, ResRef.Image.Image, nullptr);
          vkFreeMemory(Device, ResRef.Image.Memory, nullptr);
        }
        vkDestroyBuffer(Device, ResRef.Host.Buffer, nullptr);
        vkFreeMemory(Device, ResRef.Host.Memory, nullptr);
      }
      for (auto &ResRef : R.CounterResourceRefs) {
        vkDestroyBuffer(Device, ResRef.Device.Buffer, nullptr);
        vkFreeMemory(Device, ResRef.Device.Memory, nullptr);
        vkDestroyBuffer(Device, ResRef.Host.Buffer, nullptr);
        vkFreeMemory(Device, ResRef.Host.Memory, nullptr);
      }
    }

    if (IS.VertexBuffer.has_value()) {
      vkDestroyBuffer(Device, IS.VertexBuffer->Device.Buffer, nullptr);
      vkFreeMemory(Device, IS.VertexBuffer->Device.Memory, nullptr);
      vkDestroyBuffer(Device, IS.VertexBuffer->Host.Buffer, nullptr);
      vkFreeMemory(Device, IS.VertexBuffer->Host.Memory, nullptr);
    }
    if (IS.FrameBuffer)
      vkDestroyFramebuffer(Device, IS.FrameBuffer, nullptr);

    if (IS.Pool)
      vkDestroyDescriptorPool(Device, IS.Pool, nullptr);
  }

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState State;
    auto CleanupState = llvm::scope_exit([&]() {
      cleanup(State);
      llvm::outs() << "Cleanup complete.\n";
    });

    auto CBOrErr =
        VulkanCommandBuffer::create(Device, GraphicsQueue.QueueFamilyIdx);
    if (!CBOrErr)
      return CBOrErr.takeError();
    State.CB = std::move(*CBOrErr);
    llvm::outs() << "Command buffer created.\n";

    if (auto Err = createResources(P, State))
      return Err;

    BindingsDesc BindingsDesc = {};
    for (auto &S : P.Sets) {
      DescriptorSetLayoutDesc Layout;
      for (auto &R : S.Resources) {
        if (!R.VKBinding)
          return llvm::createStringError(std::errc::invalid_argument,
                                         "No VulkanBinding provided for '%s'",
                                         R.Name.c_str());

        ResourceBindingDesc ResourceBinding = {};
        ResourceBinding.Kind = R.Kind;
        ResourceBinding.DXBinding.Register = R.DXBinding.Register;
        ResourceBinding.DXBinding.Space = R.DXBinding.Space;
        ResourceBinding.VKBinding = R.VKBinding;
        ResourceBinding.DescriptorCount = R.getArraySize();
        Layout.ResourceBindings.push_back(ResourceBinding);

        if (R.HasCounter && !R.VKBinding->CounterBinding)
          return llvm::createStringError(
              std::errc::invalid_argument,
              "No CounterBinding provided for resource '%s' with a counter",
              R.Name.c_str());
      }
      BindingsDesc.DescriptorSetDescs.push_back(Layout);
    }
    for (const auto &PCB : P.PushConstants) {
      PushConstantsRange Range = {};
      Range.OffsetInBytes = 0;
      Range.SizeInBytes = PCB.size();
      BindingsDesc.PushConstantRanges.push_back(Range);
    }

    if (P.isCompute()) {
      // This is an arbitrary distinction that we could alter in the future.
      if (P.Shaders.size() != 1 || P.Shaders[0].Stage != Stages::Compute)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Compute pipeline must have exactly one compute shader.");

      ShaderContainer CS = {};
      CS.EntryPoint = P.Shaders[0].Entry;
      CS.Shader = P.Shaders[0].Shader.get();
      CS.SpecializationConstants = P.Shaders[0].SpecializationConstants;
      if (CS.Shader == nullptr) {
        llvm::outs() << "CS is null :(\n";
        llvm::outs() << "Shader count: " << P.Shaders.size() << "\n";
      }
      assert(CS.Shader != nullptr);

      auto PipelineStateOrErr =
          createPipelineCs("Compute Pipeline State", BindingsDesc, CS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Compute Pipeline created.\n";
    } else if (P.isTraditionalRaster()) {
      ShaderContainer VS = {};
      ShaderContainer PS = {};
      for (auto &Shader : P.Shaders) {
        if (Shader.Stage == Stages::Vertex) {
          VS.EntryPoint = Shader.Entry;
          VS.Shader = Shader.Shader.get();
          VS.SpecializationConstants = Shader.SpecializationConstants;
        } else if (Shader.Stage == Stages::Pixel) {
          PS.EntryPoint = Shader.Entry;
          PS.Shader = Shader.Shader.get();
          PS.SpecializationConstants = Shader.SpecializationConstants;
        }
      }

      // Create the input layout based on the vertex attributes.
      llvm::SmallVector<InputLayoutDesc> InputLayout;
      for (auto &Attr : P.Bindings.VertexAttributes) {
        auto FormatOrErr = toFormat(Attr.Format, Attr.Channels);
        if (!FormatOrErr)
          return FormatOrErr.takeError();

        InputLayoutDesc Desc = {};
        Desc.Name = Attr.Name;
        Desc.Fmt = *FormatOrErr;
        Desc.OffsetInBytes = Attr.Offset;
        InputLayout.push_back(Desc);
      }

      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();

      llvm::SmallVector<Format> RTFormats;
      RTFormats.push_back(*FormatOrErr);

      auto PipelineStateOrErr = createPipelineVsPs(
          "Graphics Pipeline State", BindingsDesc, InputLayout, RTFormats,
          Format::D32FloatS8Uint, VS, PS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Graphics Pipeline created.\n";

      if (auto Err = createFrameBuffer(State))
        return Err;
      llvm::outs() << "Frame buffer created.\n";
    } else {
      return llvm::createStringError(
          "Pipeline was neither Compute nor Traditional Graphics");
    }

    llvm::outs() << "Memory buffers created.\n";
    // No explicit wait: the next submit's GPU-side timeline semaphore
    // dependency ensures the copy completes before the dispatch runs.
    auto CopyResult = executeCommandBuffer(State);
    if (!CopyResult)
      return CopyResult.takeError();
    llvm::outs() << "Executed copy command buffer.\n";
    auto DispatchCBOrErr =
        VulkanCommandBuffer::create(Device, GraphicsQueue.QueueFamilyIdx);
    if (!DispatchCBOrErr)
      return DispatchCBOrErr.takeError();
    State.CB = std::move(*DispatchCBOrErr);
    llvm::outs() << "Execute command buffer created.\n";
    if (auto Err = createDescriptorPool(P, State))
      return Err;
    llvm::outs() << "Descriptor pool created.\n";
    if (auto Err = createDescriptorSets(P, State))
      return Err;
    llvm::outs() << "Descriptor sets created.\n";
    if (auto Err = createCommands(P, State))
      return Err;
    llvm::outs() << "Commands created.\n";
    auto DispatchResult = executeCommandBuffer(State);
    if (!DispatchResult)
      return DispatchResult.takeError();
    llvm::outs() << "Executed compute command buffer.\n";
    if (auto Err = DispatchResult->waitForCompletion())
      return Err;
    if (auto Err = readBackData(P, State))
      return Err;
    llvm::outs() << "Compute pipeline created.\n";

    return llvm::Error::success();
  }
};
} // namespace

llvm::Expected<offloadtest::SubmitResult> VulkanQueue::submit(
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs) {
  // Non-blocking: query how far the GPU has progressed and release
  // command buffers from completed submissions.
  {
    const uint64_t Completed = SubmitFence->getFenceValue();
    llvm::erase_if(InFlightBatches, [Completed](const InFlightBatch &B) {
      return B.FenceValue <= Completed;
    });
  }

  llvm::SmallVector<VkCommandBuffer> CmdBuffers;
  CmdBuffers.reserve(CBs.size());

  // GPU-side wait so that back-to-back submits don't overlap on the GPU.
  // Waiting for a value that is already signaled (including 0) is a no-op.
  const uint64_t WaitValue = FenceCounter;
  const uint64_t SignalValue = ++FenceCounter;
  const VkPipelineStageFlags WaitStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

  for (auto &CB : CBs) {
    auto &VCB = *llvm::cast<VulkanCommandBuffer>(CB.get());
    if (auto Err = VK::toError(vkEndCommandBuffer(VCB.CmdBuffer),
                               "Could not end command buffer."))
      return Err;
    CmdBuffers.push_back(VCB.CmdBuffer);
  }

  VkTimelineSemaphoreSubmitInfo TimelineInfo = {};
  TimelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  TimelineInfo.waitSemaphoreValueCount = 1;
  TimelineInfo.pWaitSemaphoreValues = &WaitValue;
  TimelineInfo.signalSemaphoreValueCount = 1;
  TimelineInfo.pSignalSemaphoreValues = &SignalValue;

  VkSubmitInfo SubmitInfo = {};
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.pNext = &TimelineInfo;
  SubmitInfo.waitSemaphoreCount = 1;
  SubmitInfo.pWaitSemaphores = &SubmitFence->Semaphore;
  SubmitInfo.pWaitDstStageMask = &WaitStage;
  SubmitInfo.commandBufferCount = CmdBuffers.size();
  SubmitInfo.pCommandBuffers = CmdBuffers.data();
  SubmitInfo.signalSemaphoreCount = 1;
  SubmitInfo.pSignalSemaphores = &SubmitFence->Semaphore;

  if (auto Err =
          VK::toError(vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE),
                      "Failed to submit to queue."))
    return Err;

  // Keep submitted command buffers alive until the GPU is done with them.
  InFlightBatches.push_back({SignalValue, std::move(CBs)});

  return offloadtest::SubmitResult{SubmitFence.get(), SignalValue};
}

llvm::Error offloadtest::initializeVulkanDevices(
    const DeviceConfig Config,
    llvm::SmallVectorImpl<std::unique_ptr<Device>> &Devices) {
  // Request the highest supported API version
  uint32_t ApiVersion = 0;
  vkEnumerateInstanceVersion(&ApiVersion);

  VkApplicationInfo AppInfo = {};
  AppInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  AppInfo.pApplicationName = "OffloadTest";
  AppInfo.apiVersion = ApiVersion;

  VkInstanceCreateInfo CreateInfo = {};
  CreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  CreateInfo.pApplicationInfo = &AppInfo;

  llvm::SmallVector<const char *> EnabledInstanceExtensions;
  llvm::SmallVector<const char *> EnabledLayers;
#if __APPLE__
  // If we build Vulkan support for Apple platforms the VK_KHR_PORTABILITY
  // extension is required, so we can just force this one on. If it fails, the
  // whole device would fail anyways.
  EnabledInstanceExtensions.push_back(
      VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  CreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

  const llvm::SmallVector<VkLayerProperties, 0> AvailableInstanceLayers =
      queryInstanceLayers();
  if (Config.EnableValidationLayer) {
    const llvm::StringRef ValidationLayer = "VK_LAYER_KHRONOS_validation";
    if (isLayerSupported(AvailableInstanceLayers, ValidationLayer))
      EnabledLayers.push_back(ValidationLayer.data());
  }
  const llvm::SmallVector<VkExtensionProperties, 0> AvailableExtensions =
      queryInstanceExtensions(nullptr);
  if (Config.EnableDebugLayer) {
    const llvm::StringRef DebugUtilsExtensionName = "VK_EXT_debug_utils";
    if (isExtensionSupported(AvailableExtensions, DebugUtilsExtensionName))
      EnabledInstanceExtensions.push_back(DebugUtilsExtensionName.data());
  }

  CreateInfo.ppEnabledLayerNames = EnabledLayers.data();
  CreateInfo.enabledLayerCount = EnabledLayers.size();
  CreateInfo.ppEnabledExtensionNames = EnabledInstanceExtensions.data();
  CreateInfo.enabledExtensionCount = EnabledInstanceExtensions.size();

  VkInstance Instance = VK_NULL_HANDLE;
  if (auto Err = VK::toError(vkCreateInstance(&CreateInfo, NULL, &Instance),
                             "Failed to create Vulkan instance"))
    return Err;

#ifndef NDEBUG
  VkDebugUtilsMessengerEXT DebugMessenger = registerDebugUtilCallback(Instance);
#else
  VkDebugUtilsMessengerEXT DebugMessenger = VK_NULL_HANDLE;
#endif

  const std::shared_ptr<VulkanInstance> VulkanInstanceShPtr =
      std::make_shared<VulkanInstance>(Instance, DebugMessenger);

  uint32_t DeviceCount = 0;
  if (auto Err = VK::toError(
          vkEnumeratePhysicalDevices(Instance, &DeviceCount, nullptr),
          "Failed to get device count"))
    return Err;
  std::vector<VkPhysicalDevice> PhysicalDevices(DeviceCount);
  if (auto Err = VK::toError(vkEnumeratePhysicalDevices(Instance, &DeviceCount,
                                                        PhysicalDevices.data()),
                             "Failed to enumerate devices"))
    return Err;

  for (const auto &PDev : PhysicalDevices) {
    auto DeviceOrErr = VulkanDevice::create(VulkanInstanceShPtr, PDev,
                                            AvailableInstanceLayers);
    if (!DeviceOrErr) {
      return DeviceOrErr.takeError();
    }
    Devices.push_back(std::move(*DeviceOrErr));
  }

  return llvm::Error::success();
}
