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
#include "API/Encoder.h"
#include "API/FormatConversion.h"
#include "API/Util.h"
#include "API/VK/AccelerationStructure.h"
#include "API/VK/Buffer.h"
#include "API/VK/CommandBuffer.h"
#include "API/VK/Descriptors.h"
#include "API/VK/Device.h"
#include "API/VK/Encoder.h"
#include "API/VK/PipelineState.h"
#include "API/VK/Queue.h"
#include "API/VK/RenderPass.h"
#include "API/VK/SBT.h"
#include "API/VK/Sampler.h"
#include "API/VK/Texture.h"
#include "Support/Pipeline.h"
#include "Support/VkError.h"
#include "VKResources.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/MathExtras.h"

#include "../Support/OffloadMigration.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <numeric>
#include <system_error>

using namespace offloadtest;

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
  case ResourceKind::AccelerationStructure:
    return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
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

// static VkBufferUsageFlagBits getFlagBits(const ResourceKind RK) {
//   switch (RK) {
//   case ResourceKind::Buffer:
//     return VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
//   case ResourceKind::RWBuffer:
//     return VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
//   case ResourceKind::ByteAddressBuffer:
//   case ResourceKind::RWByteAddressBuffer:
//   case ResourceKind::StructuredBuffer:
//   case ResourceKind::RWStructuredBuffer:
//     return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
//   case ResourceKind::ConstantBuffer:
//     return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
//   case ResourceKind::Texture2D:
//   case ResourceKind::RWTexture2D:
//   case ResourceKind::Sampler:
//   case ResourceKind::SampledTexture2D:
//   case ResourceKind::AccelerationStructure:
//     llvm_unreachable(
//         "Textures, samplers, and AS don't have buffer usage bits!");
//   }
//   llvm_unreachable("All cases handled");
// }

// static VkImageViewType getImageViewType(const ResourceKind RK) {
//   switch (RK) {
//   case ResourceKind::Texture2D:
//   case ResourceKind::RWTexture2D:
//   case ResourceKind::SampledTexture2D:
//     return VK_IMAGE_VIEW_TYPE_2D;
//   case ResourceKind::Buffer:
//   case ResourceKind::RWBuffer:
//   case ResourceKind::ByteAddressBuffer:
//   case ResourceKind::RWByteAddressBuffer:
//   case ResourceKind::StructuredBuffer:
//   case ResourceKind::RWStructuredBuffer:
//   case ResourceKind::ConstantBuffer:
//   case ResourceKind::Sampler:
//   case ResourceKind::AccelerationStructure:
//     llvm_unreachable("Not an image view!");
//   }
//   llvm_unreachable("All cases handled");
// }

// static VkImageType getVKImageType(const ResourceKind RK) {
//   switch (RK) {
//   case ResourceKind::Texture2D:
//   case ResourceKind::RWTexture2D:
//   case ResourceKind::SampledTexture2D:
//     return VK_IMAGE_TYPE_2D;
//   default:
//     llvm_unreachable("Unsupported image kind");
//   }
//   llvm_unreachable("All cases handled");
// }

static VkPrimitiveTopology getVkPrimitiveTopology(PrimitiveTopology Topology) {
  switch (Topology) {
  case PrimitiveTopology::TriangleList:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  case PrimitiveTopology::PointList:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case PrimitiveTopology::PatchList:
    return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
  case PrimitiveTopology::LineList:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  }
  llvm_unreachable("All PrimitiveTopology cases handled");
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

static VkAttachmentLoadOp getVkLoadOp(offloadtest::LoadAction Action) {
  switch (Action) {
  case offloadtest::LoadAction::Load:
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  case offloadtest::LoadAction::Clear:
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  case offloadtest::LoadAction::DontCare:
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }
  llvm_unreachable("All LoadAction cases handled");
}

static VkAttachmentStoreOp getVkStoreOp(offloadtest::StoreAction Action) {
  switch (Action) {
  case offloadtest::StoreAction::Store:
    return VK_ATTACHMENT_STORE_OP_STORE;
  case offloadtest::StoreAction::DontCare:
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }
  llvm_unreachable("All StoreAction cases handled");
}

namespace offloadtest {

llvm::Expected<std::unique_ptr<VulkanDevice>>
VulkanDevice::create(std::shared_ptr<VulkanInstance> Instance,
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

  const VulkanDevice::ExtensionVector AvailableDeviceExtensions =
      queryDeviceExtensions(PhysicalDevice);
  const bool HasVulkan12 = Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0);
  const bool HasVulkan13 = Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0);
#ifdef VK_VERSION_1_4
  const bool HasVulkan14 = Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0);
#endif

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
  if (HasVulkan12)
    Features11.pNext = &Features12;
  if (HasVulkan13)
    Features12.pNext = &Features13;
#ifdef VK_VERSION_1_4
  if (HasVulkan14)
    Features13.pNext = &Features14;
#endif

  llvm::SmallVector<const char *> EnabledDeviceExtensions;

#ifdef VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME
  const bool HasShaderImageAtomicInt64Ext =
      isExtensionSupported(AvailableDeviceExtensions,
                           VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);
  VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT FeaturesImageAtomicInt64{};
  if (HasShaderImageAtomicInt64Ext) {
    FeaturesImageAtomicInt64.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
    FeaturesImageAtomicInt64.pNext = Features.pNext;
    Features.pNext = &FeaturesImageAtomicInt64;
  }
#endif

  const bool HasMeshShader = isExtensionSupported(
      AvailableDeviceExtensions, VK_EXT_MESH_SHADER_EXTENSION_NAME);
  VkPhysicalDeviceMeshShaderFeaturesEXT MeshFeatures{};
  if (HasMeshShader) {
    MeshFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
    MeshFeatures.pNext = Features.pNext;
    Features.pNext = &MeshFeatures;
  }

  const bool HasASExts =
      isExtensionSupported(AvailableDeviceExtensions,
                           VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
      isExtensionSupported(AvailableDeviceExtensions,
                           VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) &&
      // bufferDeviceAddress is core in Vulkan 1.2; on 1.1 we need the ext.
      (HasVulkan12 ||
       isExtensionSupported(AvailableDeviceExtensions,
                            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME));
  const bool HasRayQueryExt =
      HasASExts && isExtensionSupported(AvailableDeviceExtensions,
                                        VK_KHR_RAY_QUERY_EXTENSION_NAME);
  const bool HasRayTracingPipelineExt =
      HasASExts &&
      isExtensionSupported(AvailableDeviceExtensions,
                           VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

  VkPhysicalDeviceAccelerationStructureFeaturesKHR ASFeatures{};
  // On Vulkan 1.1 we need a separate BDA features struct; on 1.2+
  // bufferDeviceAddress lives in VkPhysicalDeviceVulkan12Features which is
  // already in the chain, and adding a duplicate is a validation error.
  VkPhysicalDeviceBufferDeviceAddressFeatures BDAFeatures{};
  VkPhysicalDeviceRayQueryFeaturesKHR RayQueryFeatures{};
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR RTPipelineFeatures{};
  if (HasASExts) {
    ASFeatures.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    ASFeatures.pNext = Features.pNext;
    Features.pNext = &ASFeatures;
    if (!HasVulkan12) {
      BDAFeatures.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
      BDAFeatures.pNext = Features.pNext;
      Features.pNext = &BDAFeatures;
    }
    if (HasRayQueryExt) {
      RayQueryFeatures.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
      RayQueryFeatures.pNext = Features.pNext;
      Features.pNext = &RayQueryFeatures;
    }
    if (HasRayTracingPipelineExt) {
      RTPipelineFeatures.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
      RTPipelineFeatures.pNext = Features.pNext;
      Features.pNext = &RTPipelineFeatures;
    }
  }

  vkGetPhysicalDeviceFeatures2(PhysicalDevice, &Features);

  // For every extension chained above we verify that its gating feature bool
  // came back true; if it didn't, return a descriptive error rather than
  // letting vkCreateDevice raise the generic VK_ERROR_FEATURE_NOT_PRESENT.
  // If this ever fires on a real driver, make the check infallible: either
  // splice the struct back out of pNext or treat the feature as unsupported
  // down below (skip the matching extension push).
  if (HasMeshShader) {
    if (!MeshFeatures.taskShader)
      return llvm::createStringError(
          std::errc::not_supported,
          "Device advertises %s but reports taskShader=0",
          VK_EXT_MESH_SHADER_EXTENSION_NAME);
    if (!MeshFeatures.meshShader)
      return llvm::createStringError(
          std::errc::not_supported,
          "Device advertises %s but reports meshShader=0",
          VK_EXT_MESH_SHADER_EXTENSION_NAME);
    // primitiveFragmentShadingRateMeshShader depends on
    // primitiveFragmentShadingRate (VUID-...-07033), which we don't enable.
    MeshFeatures.multiviewMeshShader = 0;
    MeshFeatures.primitiveFragmentShadingRateMeshShader = 0;
    MeshFeatures.meshShaderQueries = 0;
    EnabledDeviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
  }

#ifdef VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME
  if (HasShaderImageAtomicInt64Ext) {
    if (!FeaturesImageAtomicInt64.shaderImageInt64Atomics)
      return llvm::createStringError(
          std::errc::not_supported,
          "Device advertises %s but reports shaderImageInt64Atomics=0",
          VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);
    FeaturesImageAtomicInt64.sparseImageInt64Atomics = 0;
    EnabledDeviceExtensions.push_back(
        VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);
  }
#endif

  if (HasASExts) {
    if (!ASFeatures.accelerationStructure)
      return llvm::createStringError(
          std::errc::not_supported,
          "Device advertises %s but reports accelerationStructure=0",
          VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    const bool HasBDA = HasVulkan12 ? Features12.bufferDeviceAddress
                                    : BDAFeatures.bufferDeviceAddress;
    if (!HasBDA)
      return llvm::createStringError(
          std::errc::not_supported,
          "Device advertises %s but reports bufferDeviceAddress=0",
          VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    // Enable only the base feature; capture-replay / indirect-build /
    // host-commands / updateAfterBind aren't used by these tests.
    ASFeatures.accelerationStructureCaptureReplay = 0;
    ASFeatures.accelerationStructureIndirectBuild = 0;
    ASFeatures.accelerationStructureHostCommands = 0;
    ASFeatures.descriptorBindingAccelerationStructureUpdateAfterBind = 0;
    if (!HasVulkan12) {
      BDAFeatures.bufferDeviceAddressCaptureReplay = 0;
      BDAFeatures.bufferDeviceAddressMultiDevice = 0;
    }
    EnabledDeviceExtensions.push_back(
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    EnabledDeviceExtensions.push_back(
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    if (!HasVulkan12)
      EnabledDeviceExtensions.push_back(
          VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    if (HasRayQueryExt) {
      if (!RayQueryFeatures.rayQuery)
        return llvm::createStringError(
            std::errc::not_supported,
            "Device advertises %s but reports rayQuery=0",
            VK_KHR_RAY_QUERY_EXTENSION_NAME);
      EnabledDeviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
    }
    if (HasRayTracingPipelineExt) {
      if (!RTPipelineFeatures.rayTracingPipeline)
        return llvm::createStringError(
            std::errc::not_supported,
            "Device advertises %s but reports rayTracingPipeline=0",
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
      // Only the base feature is needed for DispatchRays. Capture-replay /
      // trace-rays-indirect / chained-mode aren't used.
      RTPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplay = 0;
      RTPipelineFeatures.rayTracingPipelineShaderGroupHandleCaptureReplayMixed =
          0;
      RTPipelineFeatures.rayTracingPipelineTraceRaysIndirect = 0;
      RTPipelineFeatures.rayTraversalPrimitiveCulling = 0;
      EnabledDeviceExtensions.push_back(
          VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    }
  }

  DeviceInfo.enabledExtensionCount =
      static_cast<uint32_t>(EnabledDeviceExtensions.size());
  DeviceInfo.ppEnabledExtensionNames = EnabledDeviceExtensions.data();
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

  auto Dev = std::make_unique<VulkanDevice>(
      Instance, PhysicalDevice, Props, Device, std::move(GraphicsQueue),
      std::move(InstanceLayers), std::move(AvailableDeviceExtensions));

  // Load acceleration-structure and ray-tracing-pipeline function pointers
  // after device creation. These two feature sets are independent; the RT
  // pipeline path needs AS as a prerequisite, but AS-only support (ray
  // query in compute) is a complete configuration on its own.
  if (HasASExts) {
    Dev->HasASSupport = true;
    Dev->AS.Create = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
        vkGetDeviceProcAddr(Device, "vkCreateAccelerationStructureKHR"));
    Dev->AS.Destroy = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
        vkGetDeviceProcAddr(Device, "vkDestroyAccelerationStructureKHR"));
    Dev->AS.GetBuildSizes =
        reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
            vkGetDeviceProcAddr(Device,
                                "vkGetAccelerationStructureBuildSizesKHR"));
    Dev->AS.GetDeviceAddress =
        reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            vkGetDeviceProcAddr(Device,
                                "vkGetAccelerationStructureDeviceAddressKHR"));
    Dev->AS.CmdBuild =
        reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr(Device, "vkCmdBuildAccelerationStructuresKHR"));
    if (HasRayTracingPipelineExt) {
      Dev->HasRTPipelineSupport = true;
      Dev->RT.CreatePipelines =
          reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
              vkGetDeviceProcAddr(Device, "vkCreateRayTracingPipelinesKHR"));
      Dev->RT.GetGroupHandles =
          reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
              vkGetDeviceProcAddr(Device,
                                  "vkGetRayTracingShaderGroupHandlesKHR"));
      Dev->RT.CmdTraceRays = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
          vkGetDeviceProcAddr(Device, "vkCmdTraceRaysKHR"));

      // Cache SBT handle size / alignments for the lifetime of the device.
      VkPhysicalDeviceProperties2 Props2{};
      Props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
      Dev->RTPipelineProps.sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
      Props2.pNext = &Dev->RTPipelineProps;
      vkGetPhysicalDeviceProperties2(PhysicalDevice, &Props2);
    }
  }

  return Dev;
}

VulkanDevice::VulkanDevice(
    std::shared_ptr<VulkanInstance> I, VkPhysicalDevice P,
    VkPhysicalDeviceProperties Props, VkDevice D, VulkanQueue Q,
    llvm::SmallVector<VkLayerProperties, 0> InstanceLayers,
    ExtensionVector DeviceExtensions)
    : Instance(I), PhysicalDevice(P), Props(Props), Device(D),
      GraphicsQueue(std::move(Q)), InstanceLayers(std::move(InstanceLayers)),
      DeviceExtensions(std::move(DeviceExtensions)) {
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

  const uint64_t DriverInfoSz =
      strnlen(DriverProps.driverInfo, VK_MAX_DRIVER_INFO_SIZE);
  DriverVersion = std::string(DriverProps.driverInfo, DriverInfoSz);

  // 0x8086 is the Vendor ID for Intel
  if (Props.vendorID == 0x8086) {
    FamilyPrefix = static_cast<uint16_t>(Props.deviceID) & 0xFF00;
    const IntelGpuEra Era =
        getIntelGpuEra(static_cast<uint16_t>(Props.deviceID));
    if (Era == IntelGpuEra::Gen7_to_10)
      GPUGeneration = "Intel Gen7-10";
    else if (Era == IntelGpuEra::Gen11_to_14_and_Xe)
      GPUGeneration = "Intel Gen11-14/Xe";
    else
      GPUGeneration = "Intel Unknown";
  } else {
    // We don't have a need yet to identify other GPU vendors.
    GPUGeneration = "Unknown";
  }
#if defined(__APPLE__) && defined(__aarch64__)
  // Apple silicon Macs may have multiple Vulkan drivers sharing one device
  // name. Include the driver name in the description to enable
  // adapter-regex matching.
  Description += " (" + DriverName + ")";
#endif

  CmdBeginDebugUtilsLabel =
      (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(
          Device, "vkCmdBeginDebugUtilsLabelEXT");
  CmdEndDebugUtilsLabel = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetDeviceProcAddr(
      Device, "vkCmdEndDebugUtilsLabelEXT");
  CmdInsertDebugUtilsLabel =
      (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetDeviceProcAddr(
          Device, "vkCmdInsertDebugUtilsLabelEXT");

  MeshShaderFns = MeshShaderFunctions::create(Device);
}

VulkanDevice::~VulkanDevice() {
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

llvm::StringRef VulkanDevice::getAPIName() const { return "Vulkan"; }

GPUAPI VulkanDevice::getAPI() const { return GPUAPI::Vulkan; }

bool VulkanDevice::classof(const offloadtest::Device *D) {
  return D->getAPI() == GPUAPI::Vulkan;
}

Queue &VulkanDevice::getGraphicsQueue() { return GraphicsQueue; }

llvm::Error VulkanDevice::createPipelineLayout(
    const BindingsDesc &BindingsDesc, VkShaderStageFlags StageFlags,
    llvm::SmallVectorImpl<VkDescriptorSetLayout> &SetLayouts,
    VkPipelineLayout &PipelineLayout, DescriptorCounts &DescCounts) {
  assert(SetLayouts.empty() && "Output vector SetLayouts must be empty.");
  DescCounts = {};

  // Build descriptor set layouts from BindingsDesc.
  for (const DescriptorSetLayoutDesc &SetDesc :
       BindingsDesc.DescriptorSetDescs) {
    std::vector<VkDescriptorSetLayoutBinding> Binds;
    for (const ResourceBindingDesc &RB : SetDesc.ResourceBindings) {
      const VulkanBinding VKBinding = RB.VKBinding.value();

      switch (RB.Kind) {
      case ResourceKind::Buffer:
      case ResourceKind::RWBuffer:
        DescCounts.BufferViewCount += RB.DescriptorCount;
        break;
      case ResourceKind::StructuredBuffer:
      case ResourceKind::RWStructuredBuffer:
      case ResourceKind::ByteAddressBuffer:
      case ResourceKind::RWByteAddressBuffer:
      case ResourceKind::ConstantBuffer:
        DescCounts.BufferInfoCount += RB.DescriptorCount;
        break;
      case ResourceKind::Texture2D:
      case ResourceKind::RWTexture2D:
      case ResourceKind::Sampler:
      case ResourceKind::SampledTexture2D:
        DescCounts.ImageInfoCount += RB.DescriptorCount;
        break;
      case ResourceKind::AccelerationStructure:
        DescCounts.ASHandleCount += RB.DescriptorCount;

        // This will end up being to high if the user uses the
        // `DescriptorSetsBuilder` efficiently, but will always be correct
        // this way. Alternatively, the `DescriptorSetsBuilder` could patch
        // pointers on reallocation instead.
        DescCounts.ASInfoCount += RB.DescriptorCount;
        break;
      }

      VkDescriptorSetLayoutBinding B = {};
      B.binding = VKBinding.Binding;
      B.descriptorType = getDescriptorType(RB.Kind);
      B.descriptorCount = RB.DescriptorCount;
      B.stageFlags = StageFlags;
      Binds.push_back(B);

      if (VKBinding.CounterBinding) {
        DescCounts.BufferInfoCount += RB.DescriptorCount;

        VkDescriptorSetLayoutBinding CB = {};
        CB.binding = *VKBinding.CounterBinding;
        CB.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        CB.descriptorCount = RB.DescriptorCount;
        CB.stageFlags = StageFlags;
        Binds.push_back(CB);
      }
    }

    DescCounts.DescriptorWriteHint += static_cast<uint32_t>(Binds.size());

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
    const VkPushConstantRange R = {static_cast<VkShaderStageFlags>(StageFlags),
                                   PCR.OffsetInBytes, PCR.SizeInBytes};
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
VulkanDevice::createShaderModule(const llvm::MemoryBuffer *Shader,
                                 const char *Kind) {
  const llvm::StringRef Bytecode = Shader->getBuffer();
  VkShaderModuleCreateInfo ModuleCI = {};
  ModuleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  ModuleCI.codeSize = Bytecode.size();
  ModuleCI.pCode = reinterpret_cast<const uint32_t *>(Bytecode.data());
  VkShaderModule Module = VK_NULL_HANDLE;
  if (vkCreateShaderModule(Device, &ModuleCI, nullptr, &Module))
    return llvm::createStringError(std::errc::not_supported,
                                   "Failed to create %s shader module.", Kind);
  return Module;
}

llvm::Expected<std::unique_ptr<PipelineState>>
VulkanDevice::createPipelineCs(llvm::StringRef Name,
                               const BindingsDesc &BindingsDesc,
                               ShaderContainer CS) {
  auto CSModOrErr = createShaderModule(CS.Shader, "compute");
  if (!CSModOrErr)
    return CSModOrErr.takeError();

  VkShaderModule CSModule = *CSModOrErr;
  // No longer need shader modules after pipeline compilation.
  auto ShaderModuleCleanUp = llvm::scope_exit(
      [&] { vkDestroyShaderModule(Device, CSModule, nullptr); });

  llvm::SmallVector<VkSpecializationMapEntry> SpecEntries;
  llvm::SmallVector<char> SpecData;
  VkSpecializationInfo SpecInfo = {};
  if (auto Err = parseSpecializationConstants(CS.SpecializationConstants,
                                              SpecEntries, SpecData, SpecInfo))
    return Err;

  llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
  VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
  DescriptorCounts DescCounts = {};
  if (auto Err = createPipelineLayout(BindingsDesc, VK_SHADER_STAGE_COMPUTE_BIT,
                                      SetLayouts, PipelineLayout, DescCounts))
    return Err;

  auto CleanupState = llvm::scope_exit([&]() {
    for (auto &Layout : SetLayouts)
      vkDestroyDescriptorSetLayout(Device, Layout, nullptr);
  });

  VkPipelineShaderStageCreateInfo StageCI = {};
  StageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  StageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  StageCI.module = CSModule;
  StageCI.pName = CS.EntryPoint.c_str();
  StageCI.pSpecializationInfo =
      CS.SpecializationConstants.empty() ? nullptr : &SpecInfo;

  VkComputePipelineCreateInfo PipelineCI = {};
  PipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  PipelineCI.stage = StageCI;
  PipelineCI.layout = PipelineLayout;
  VkPipeline Pipeline = VK_NULL_HANDLE;
  if (auto Err =
          VK::toError(vkCreateComputePipelines(Device, VK_NULL_HANDLE, 1,
                                               &PipelineCI, nullptr, &Pipeline),
                      "Failed to create compute pipeline.")) {
    vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
    return Err;
  }

  return std::make_unique<VulkanPipelineState>(
      Name, Device, Pipeline, PipelineLayout, std::move(SetLayouts),
      DescCounts);
}

llvm::Expected<std::unique_ptr<PipelineState>>
VulkanDevice::createTraditionalRasterPipeline(
    llvm::StringRef Name, const BindingsDesc &BindingsDesc,
    const TraditionalRasterPipelineCreateDesc &Desc) {
  const ShaderContainer &VS = Desc.VS;
  const ShaderContainer &PS = Desc.PS;
  const std::optional<ShaderContainer> &HS = Desc.HS;
  const std::optional<ShaderContainer> &DS = Desc.DS;
  const std::optional<ShaderContainer> &GS = Desc.GS;
  const llvm::ArrayRef<InputLayoutDesc> InputLayout = Desc.InputLayout;
  const llvm::ArrayRef<Format> RTFormats = Desc.RTFormats;
  const std::optional<Format> DSFormat = Desc.DSFormat;

  VkShaderStageFlags GraphicsFlags = VK_SHADER_STAGE_VERTEX_BIT;
  llvm::SmallVector<VkPipelineShaderStageCreateInfo, 5> ShaderStages;
  // No longer need shader modules after pipeline compilation.
  auto ShaderModuleCleanUp = llvm::scope_exit([&] {
    for (auto &Stage : ShaderStages)
      vkDestroyShaderModule(Device, Stage.module, nullptr);
  });

  llvm::SmallVector<VkSpecializationMapEntry> VSSpecEntries;
  llvm::SmallVector<char> VSSpecData;
  VkSpecializationInfo VSSpecInfo = {};
  {
    if (auto Err = parseSpecializationConstants(
            VS.SpecializationConstants, VSSpecEntries, VSSpecData, VSSpecInfo))
      return Err;

    auto VSModOrErr = createShaderModule(VS.Shader, "vertex");
    if (!VSModOrErr)
      return VSModOrErr.takeError();

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStage.module = *VSModOrErr;
    ShaderStage.pName = VS.EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        VS.SpecializationConstants.empty() ? nullptr : &VSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  llvm::SmallVector<VkSpecializationMapEntry> HSSpecEntries;
  llvm::SmallVector<char> HSSpecData;
  VkSpecializationInfo HSSpecInfo = {};
  if (HS) {
    if (auto Err = parseSpecializationConstants(
            HS->SpecializationConstants, HSSpecEntries, HSSpecData, HSSpecInfo))
      return Err;

    auto HSModOrErr = createShaderModule(HS->Shader, "hull");
    if (!HSModOrErr)
      return HSModOrErr.takeError();

    GraphicsFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    ShaderStage.module = *HSModOrErr;
    ShaderStage.pName = HS->EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        HS->SpecializationConstants.empty() ? nullptr : &HSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  llvm::SmallVector<VkSpecializationMapEntry> DSSpecEntries;
  llvm::SmallVector<char> DSSpecData;
  VkSpecializationInfo DSSpecInfo = {};
  if (DS) {
    if (auto Err = parseSpecializationConstants(
            DS->SpecializationConstants, DSSpecEntries, DSSpecData, DSSpecInfo))
      return Err;

    auto DSModOrErr = createShaderModule(DS->Shader, "domain");
    if (!DSModOrErr)
      return DSModOrErr.takeError();

    GraphicsFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    ShaderStage.module = *DSModOrErr;
    ShaderStage.pName = DS->EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        DS->SpecializationConstants.empty() ? nullptr : &DSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  llvm::SmallVector<VkSpecializationMapEntry> GSSpecEntries;
  llvm::SmallVector<char> GSSpecData;
  VkSpecializationInfo GSSpecInfo = {};
  if (GS) {
    if (auto Err = parseSpecializationConstants(
            GS->SpecializationConstants, GSSpecEntries, GSSpecData, GSSpecInfo))
      return Err;

    auto GSModOrErr = createShaderModule(GS->Shader, "geometry");
    if (!GSModOrErr)
      return GSModOrErr.takeError();

    GraphicsFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
    ShaderStage.module = *GSModOrErr;
    ShaderStage.pName = GS->EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        GS->SpecializationConstants.empty() ? nullptr : &GSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  llvm::SmallVector<VkSpecializationMapEntry> PSSpecEntries;
  llvm::SmallVector<char> PSSpecData;
  VkSpecializationInfo PSSpecInfo = {};
  {
    if (auto Err = parseSpecializationConstants(
            PS.SpecializationConstants, PSSpecEntries, PSSpecData, PSSpecInfo))
      return Err;

    auto PSModOrErr = createShaderModule(PS.Shader, "pixel");
    if (!PSModOrErr)
      return PSModOrErr.takeError();

    GraphicsFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStage.module = *PSModOrErr;
    ShaderStage.pName = PS.EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        PS.SpecializationConstants.empty() ? nullptr : &PSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  // Build a RenderPassDesc from the PSO's RT/DS formats.
  RenderPassDesc PassDesc;
  PassDesc.ColorAttachments.reserve(RTFormats.size());
  for (const Format F : RTFormats) {
    ColorAttachmentFormatDesc CA = {};
    CA.Fmt = F;
    CA.Load = LoadAction::DontCare;
    CA.Store = StoreAction::DontCare;
    PassDesc.ColorAttachments.push_back(CA);
  }
  if (DSFormat) {
    DepthStencilAttachmentFormatDesc DS = {};
    DS.Fmt = *DSFormat;
    DS.DepthLoad = LoadAction::DontCare;
    DS.DepthStore = StoreAction::DontCare;
    DS.StencilLoad = LoadAction::DontCare;
    DS.StencilStore = StoreAction::DontCare;
    PassDesc.DepthStencil = DS;
  }

  // NOTE: After pipeline creation this render pass can be dropped. Later
  // render passes just need to be compatible with this render pass, or in
  // other words: the format, sample count and number of targets (rt and ds),
  // need to match, but Load/Store ops (and initial/final layout) are ignored.
  auto RenderPassOrErr = createRenderPass(PassDesc);
  if (!RenderPassOrErr)
    return RenderPassOrErr.takeError();
  const std::unique_ptr<offloadtest::RenderPass> RenderPass =
      std::move(*RenderPassOrErr);
  VkRenderPass RenderPassHandle =
      llvm::cast<VulkanRenderPass>(*RenderPass).Handle;

  llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
  VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
  DescriptorCounts DescCounts;
  if (auto Err = createPipelineLayout(BindingsDesc, GraphicsFlags, SetLayouts,
                                      PipelineLayout, DescCounts))
    return Err;

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
  InputAssemblyCI.topology = getVkPrimitiveTopology(Desc.Topology);

  VkPipelineTessellationStateCreateInfo TessellationCI = {};
  if (Desc.PatchControlPoints) {
    TessellationCI.sType =
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    TessellationCI.patchControlPoints = *Desc.PatchControlPoints;
  }

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
  PipelineCI.stageCount = static_cast<uint32_t>(ShaderStages.size());
  PipelineCI.pStages = ShaderStages.data();
  PipelineCI.pVertexInputState = &VertexInputCI;
  PipelineCI.pInputAssemblyState = &InputAssemblyCI;
  PipelineCI.pTessellationState =
      Desc.PatchControlPoints ? &TessellationCI : nullptr;
  PipelineCI.pViewportState = &ViewportCI;
  PipelineCI.pRasterizationState = &RastCI;
  PipelineCI.pMultisampleState = &MultisampleCI;
  PipelineCI.pDepthStencilState = &DepthStencilCI;
  PipelineCI.pColorBlendState = &BlendCI;
  PipelineCI.pDynamicState = &DynamicCI;
  PipelineCI.layout = PipelineLayout;
  PipelineCI.renderPass = RenderPassHandle;

  VkPipeline Pipeline = VK_NULL_HANDLE;
  if (auto Err = VK::toError(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE,
                                                       1, &PipelineCI, nullptr,
                                                       &Pipeline),
                             "Failed to create graphics pipeline.")) {
    vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
    for (auto *L : SetLayouts)
      vkDestroyDescriptorSetLayout(Device, L, nullptr);
    return Err;
  }

  return std::make_unique<VulkanPipelineState>(
      Name, Device, Pipeline, PipelineLayout, std::move(SetLayouts),
      DescCounts);
}

llvm::Expected<std::unique_ptr<PipelineState>>
VulkanDevice::createMeshShaderRasterPipeline(
    llvm::StringRef Name, const BindingsDesc &BindingsDesc,
    const MeshShaderRasterPipelineCreateDesc &Desc) {
  assert(Desc.RTFormats.size() <= 8);

  VkShaderStageFlags GraphicsFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
  llvm::SmallVector<VkPipelineShaderStageCreateInfo, 3> ShaderStages;
  // No longer need shader modules after pipeline compilation.
  auto ShaderModuleCleanUp = llvm::scope_exit([&] {
    for (auto &Stage : ShaderStages)
      vkDestroyShaderModule(Device, Stage.module, nullptr);
  });

  llvm::SmallVector<VkSpecializationMapEntry> MSSpecEntries;
  llvm::SmallVector<char> MSSpecData;
  VkSpecializationInfo MSSpecInfo = {};
  {
    if (auto Err =
            parseSpecializationConstants(Desc.MS.SpecializationConstants,
                                         MSSpecEntries, MSSpecData, MSSpecInfo))
      return Err;

    auto MSModOrErr = createShaderModule(Desc.MS.Shader, "mesh");
    if (!MSModOrErr)
      return MSModOrErr.takeError();

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
    ShaderStage.module = *MSModOrErr;
    ShaderStage.pName = Desc.MS.EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        Desc.MS.SpecializationConstants.empty() ? nullptr : &MSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  llvm::SmallVector<VkSpecializationMapEntry> ASSpecEntries;
  llvm::SmallVector<char> ASSpecData;
  VkSpecializationInfo ASSpecInfo = {};
  if (Desc.AS) {
    if (auto Err =
            parseSpecializationConstants((*Desc.AS).SpecializationConstants,
                                         ASSpecEntries, ASSpecData, ASSpecInfo))
      return Err;

    auto ASModOrErr = createShaderModule((*Desc.AS).Shader, "task");
    if (!ASModOrErr)
      return ASModOrErr.takeError();

    GraphicsFlags |= VK_SHADER_STAGE_TASK_BIT_EXT;

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
    ShaderStage.module = *ASModOrErr;
    ShaderStage.pName = (*Desc.AS).EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        (*Desc.AS).SpecializationConstants.empty() ? nullptr : &ASSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  llvm::SmallVector<VkSpecializationMapEntry> PSSpecEntries;
  llvm::SmallVector<char> PSSpecData;
  VkSpecializationInfo PSSpecInfo = {};
  if (Desc.PS) {
    if (auto Err =
            parseSpecializationConstants((*Desc.PS).SpecializationConstants,
                                         PSSpecEntries, PSSpecData, PSSpecInfo))
      return Err;

    auto PSModOrErr = createShaderModule((*Desc.PS).Shader, "pixel");
    if (!PSModOrErr)
      return PSModOrErr.takeError();

    GraphicsFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineShaderStageCreateInfo ShaderStage = {};
    ShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStage.module = *PSModOrErr;
    ShaderStage.pName = (*Desc.PS).EntryPoint.c_str();
    ShaderStage.pSpecializationInfo =
        (*Desc.PS).SpecializationConstants.empty() ? nullptr : &PSSpecInfo;
    ShaderStages.push_back(ShaderStage);
  }

  // Build a RenderPassDesc from the PSO's RT/DS formats.
  RenderPassDesc PassDesc;
  PassDesc.ColorAttachments.reserve(Desc.RTFormats.size());
  for (const Format F : Desc.RTFormats) {
    ColorAttachmentFormatDesc CA = {};
    CA.Fmt = F;
    PassDesc.ColorAttachments.push_back(CA);
  }
  if (Desc.DSFormat) {
    DepthStencilAttachmentFormatDesc DS = {};
    DS.Fmt = *Desc.DSFormat;
    PassDesc.DepthStencil = DS;
  }

  // NOTE: After pipeline creation this render pass can be dropped. Later
  // render passes just need to be compatible with this render pass, or in
  // other words: the format, sample count and number of targets (rt and ds),
  // need to match.
  auto RenderPassOrErr = createRenderPass(PassDesc);
  if (!RenderPassOrErr)
    return RenderPassOrErr.takeError();
  const std::unique_ptr<offloadtest::RenderPass> RenderPass =
      std::move(*RenderPassOrErr);
  VkRenderPass RenderPassHandle =
      llvm::cast<VulkanRenderPass>(*RenderPass).Handle;

  llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
  VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
  DescriptorCounts DescCounts = {};
  if (auto Err = createPipelineLayout(BindingsDesc, GraphicsFlags, SetLayouts,
                                      PipelineLayout, DescCounts))
    return Err;

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
      Desc.RTFormats.size());
  for (auto &BA : BlendAttachments)
    BA.colorWriteMask = 0xf;
  VkPipelineColorBlendStateCreateInfo BlendCI = {};
  BlendCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  BlendCI.attachmentCount = static_cast<uint32_t>(BlendAttachments.size());
  BlendCI.pAttachments = BlendAttachments.data();

  VkGraphicsPipelineCreateInfo PipelineCI = {};
  PipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  PipelineCI.stageCount = static_cast<uint32_t>(ShaderStages.size());
  PipelineCI.pStages = ShaderStages.data();
  PipelineCI.pViewportState = &ViewportCI;
  PipelineCI.pRasterizationState = &RastCI;
  PipelineCI.pMultisampleState = &MultisampleCI;
  PipelineCI.pDepthStencilState = &DepthStencilCI;
  PipelineCI.pColorBlendState = &BlendCI;
  PipelineCI.pDynamicState = &DynamicCI;
  PipelineCI.layout = PipelineLayout;
  PipelineCI.renderPass = RenderPassHandle;

  VkPipeline Pipeline = VK_NULL_HANDLE;
  if (auto Err = VK::toError(vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE,
                                                       1, &PipelineCI, nullptr,
                                                       &Pipeline),
                             "Failed to create mesh shader pipeline.")) {
    vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
    for (auto *L : SetLayouts)
      vkDestroyDescriptorSetLayout(Device, L, nullptr);
    return Err;
  }

  return std::make_unique<VulkanPipelineState>(
      Name, Device, Pipeline, PipelineLayout, std::move(SetLayouts),
      DescCounts);
}

llvm::Expected<std::unique_ptr<offloadtest::Fence>>
VulkanDevice::createFence(llvm::StringRef Name) {
  return VulkanFence::create(Device, Name);
}

llvm::Expected<std::unique_ptr<offloadtest::MemoryHeap>>
VulkanDevice::createMemoryHeap(std::string /*Name*/, size_t /*SizeInBytes*/) {
  return llvm::createStringError(
      std::errc::not_supported,
      "Vulkan backend does not yet support memory heaps.");
}

llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
VulkanDevice::createBuffer(std::string Name, const BufferCreateDesc &Desc,
                           size_t SizeInBytes) {
  VkBufferCreateInfo BufInfo = {};
  BufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  BufInfo.size = SizeInBytes;
  BufInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  BufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  switch (Desc.Usage) {
  case BufferUsage::Storage:
    BufInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    break;
  case BufferUsage::ConstantBuffer:
    BufInfo.usage |=
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    break;
  case BufferUsage::IndexBuffer:
    BufInfo.usage |=
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    break;
  case BufferUsage::VertexBuffer:
    BufInfo.usage |=
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    break;
  case BufferUsage::IndirectArgs:
    BufInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    break;
  }

  // When ray tracing is supported, every buffer is eligible to act as an
  // acceleration-structure build input and to expose a device address. The
  // shared AS build helper assumes Storage buffers carry these flags.
  if (HasASSupport)
    BufInfo.usage |=
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

  if (Desc.AccessType == BufferShaderAccessType::Typed)
    BufInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;

  VkBuffer BufferObject;
  if (auto Err =
          VK::toError(vkCreateBuffer(Device, &BufInfo, nullptr, &BufferObject),
                      "Failed to create device buffer."))
    return Err;

  VkMemoryRequirements MemReqs;
  vkGetBufferMemoryRequirements(Device, BufferObject, &MemReqs);

  VkMemoryAllocateFlagsInfo AllocFlagsInfo = {};
  AllocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  AllocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

  VkMemoryAllocateInfo AllocInfo = {};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  if (HasASSupport)
    AllocInfo.pNext = &AllocFlagsInfo;
  AllocInfo.allocationSize = MemReqs.size;
  auto MemIdx = getMemoryIndex(PhysicalDevice, MemReqs.memoryTypeBits,
                               getVulkanMemoryFlags(Desc.Location));
  if (!MemIdx) {
    vkDestroyBuffer(Device, BufferObject, nullptr);
    return MemIdx.takeError();
  }
  AllocInfo.memoryTypeIndex = *MemIdx;

  VkBuffer CounterBuffer = nullptr;
  VkMemoryRequirements CounterMemReqs = {};
  VkDeviceSize CounterOffsetInBytes = 0;
  if (Desc.HasCounter) {
    VkBufferCreateInfo CounterBufferInfo = {};
    CounterBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    CounterBufferInfo.size = sizeof(uint32_t);
    CounterBufferInfo.usage = BufInfo.usage;
    CounterBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (auto Err = VK::toError(
            vkCreateBuffer(Device, &CounterBufferInfo, nullptr, &CounterBuffer),
            "Could not create counter buffer.")) {
      vkDestroyBuffer(Device, BufferObject, nullptr);
      return Err;
    }

    vkGetBufferMemoryRequirements(Device, CounterBuffer, &CounterMemReqs);

    CounterOffsetInBytes =
        llvm::alignTo(AllocInfo.allocationSize, CounterMemReqs.alignment);
    AllocInfo.allocationSize = CounterOffsetInBytes + CounterMemReqs.size;

    assert(MemReqs.memoryTypeBits == CounterMemReqs.memoryTypeBits &&
           "We are expecting the main resource and counter resource to have "
           "the same memory type.");
  }

  VkDeviceMemory DeviceMemory;
  if (auto Err = VK::toError(
          vkAllocateMemory(Device, &AllocInfo, nullptr, &DeviceMemory),
          "Failed to allocate device memory.")) {
    if (CounterBuffer)
      vkDestroyBuffer(Device, CounterBuffer, nullptr);
    vkDestroyBuffer(Device, BufferObject, nullptr);
    return Err;
  }

  if (auto Err =
          VK::toError(vkBindBufferMemory(Device, BufferObject, DeviceMemory, 0),
                      "Failed to bind device buffer memory.")) {
    if (CounterBuffer)
      vkDestroyBuffer(Device, CounterBuffer, nullptr);
    vkDestroyBuffer(Device, BufferObject, nullptr);
    vkFreeMemory(Device, DeviceMemory, nullptr);
    return Err;
  }

  if (CounterBuffer != nullptr) {
    if (auto Err =
            VK::toError(vkBindBufferMemory(Device, CounterBuffer, DeviceMemory,
                                           CounterOffsetInBytes),
                        "Failed to bind counter buffer memory.")) {
      if (CounterBuffer)
        vkDestroyBuffer(Device, CounterBuffer, nullptr);
      vkDestroyBuffer(Device, BufferObject, nullptr);
      vkFreeMemory(Device, DeviceMemory, nullptr);
      return Err;
    }
  }

  VkDeviceAddress DevAddr = 0;
  if (HasASSupport) {
    VkBufferDeviceAddressInfo AddrInfo = {};
    AddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    AddrInfo.buffer = BufferObject;
    DevAddr = vkGetBufferDeviceAddress(Device, &AddrInfo);
  }

  VkBufferView View = VK_NULL_HANDLE;
  if (Desc.AccessType == BufferShaderAccessType::Typed) {
    // Create buffer view
    VkBufferViewCreateInfo BufferViewCI = {};
    BufferViewCI.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    BufferViewCI.buffer = BufferObject;
    BufferViewCI.format = getVulkanFormat(Desc.AccessTypeParams.Fmt);
    BufferViewCI.range = VK_WHOLE_SIZE;

    if (auto Err = VK::toError(
            vkCreateBufferView(Device, &BufferViewCI, nullptr, &View),
            "Failed to create buffer view.")) {
      if (CounterBuffer)
        vkDestroyBuffer(Device, CounterBuffer, nullptr);
      vkDestroyBuffer(Device, BufferObject, nullptr);
      vkFreeMemory(Device, DeviceMemory, nullptr);
    }
  }

  return std::make_unique<VulkanBuffer>(Device, BufferObject, CounterBuffer,
                                        DeviceMemory, DevAddr, Name, Desc,
                                        SizeInBytes, View);
}

llvm::Expected<std::unique_ptr<offloadtest::Texture>>
VulkanDevice::createTexture(std::string Name, const TextureCreateDesc &Desc) {
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
  ImageInfo.tiling = Desc.Location == MemoryLocation::GpuOnly
                         ? VK_IMAGE_TILING_OPTIMAL
                         : VK_IMAGE_TILING_LINEAR;
  ImageInfo.usage = getVulkanImageUsage(Desc.Usage);
  ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImage Image;
  if (auto Err = VK::toError(vkCreateImage(Device, &ImageInfo, nullptr, &Image),
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
  if (auto Err = VK::toError(vkBindImageMemory(Device, Image, DeviceMemory, 0),
                             "Failed to bind image memory.")) {
    vkDestroyImage(Device, Image, nullptr);
    vkFreeMemory(Device, DeviceMemory, nullptr);
    return Err;
  }

  VkImageAspectFlags FullAspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  if (isDepthFormat(Desc.Fmt)) {
    FullAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (isStencilFormat(Desc.Fmt))
      FullAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  const VkImageSubresourceRange FullRange{
      FullAspectMask,
      0, /*baseMipLevel*/
      Desc.MipLevels,
      0, /*baseArrayLayer*/
      1, /*layerCount*/
  };

  VkImageViewCreateInfo ViewCi = {};
  ViewCi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewCi.image = Image;
  ViewCi.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ViewCi.format = getVulkanFormat(Desc.Fmt);
  ViewCi.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                       VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  ViewCi.subresourceRange.baseMipLevel = 0;
  ViewCi.subresourceRange.levelCount = Desc.MipLevels;
  ViewCi.subresourceRange.baseArrayLayer = 0;
  ViewCi.subresourceRange.layerCount = 1;
  ViewCi.subresourceRange.aspectMask = FullAspectMask;

  VkImageView View = VK_NULL_HANDLE;
  if (auto Err = VK::toError(vkCreateImageView(Device, &ViewCi, nullptr, &View),
                             "Failed to create image view.")) {
    vkDestroyImage(Device, Image, nullptr);
    vkFreeMemory(Device, DeviceMemory, nullptr);
    return Err;
  }

  VkImageLayout PreferredLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  if ((Desc.Usage & TextureUsage::Storage))
    PreferredLayout = VK_IMAGE_LAYOUT_GENERAL;

  auto Tex = std::make_unique<VulkanTexture>(
      Device, Image, DeviceMemory, View, Name, Desc, PreferredLayout, FullRange,
      ImageInfo.tiling, MemReqs.size);

  return Tex;
}

llvm::Expected<std::unique_ptr<Sampler>>
VulkanDevice::createSampler(std::string Name, const SamplerCreateDesc &Desc) {

  VkSamplerCreateInfo SamplerInfo = {};
  SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  SamplerInfo.magFilter = getVKFilter(Desc.MagFilter);
  SamplerInfo.minFilter = getVKFilter(Desc.MinFilter);
  SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  SamplerInfo.addressModeU = getVKAddressMode(Desc.Address);
  SamplerInfo.addressModeV = getVKAddressMode(Desc.Address);
  SamplerInfo.addressModeW = getVKAddressMode(Desc.Address);
  SamplerInfo.mipLodBias = Desc.MipLODBias;
  SamplerInfo.anisotropyEnable = VK_FALSE;
  SamplerInfo.maxAnisotropy = 1.0f;
  SamplerInfo.compareEnable =
      Desc.Kind == SamplerKind::SamplerComparison ? VK_TRUE : VK_FALSE;
  SamplerInfo.compareOp = getVKCompareOp(Desc.ComparisonOp);
  SamplerInfo.minLod = Desc.MinLOD;
  SamplerInfo.maxLod = Desc.MaxLOD;
  SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
  SamplerInfo.unnormalizedCoordinates = VK_FALSE;

  VkSampler Sampler;
  if (auto Err =
          VK::toError(vkCreateSampler(Device, &SamplerInfo, nullptr, &Sampler),
                      "Failed to create sampler."))
    return Err;

  return std::make_unique<VulkanSampler>(std::move(Name), Desc, Sampler,
                                         Device);
}

uint32_t VulkanDevice::getTextureUploadRowStrideInBytes(
    const TextureCreateDesc &Desc) const {
  const uint64_t TightRow =
      uint64_t(Desc.Width) * getFormatSizeInBytes(Desc.Fmt);
  return static_cast<uint32_t>(
      llvm::alignTo(TightRow, Props.limits.optimalBufferCopyRowPitchAlignment));
}

TextureUploadLayout
VulkanDevice::getTextureUploadLayout(const TextureCreateDesc &Desc) const {
  // copyBufferToTexture consumes a tightly-packed staging buffer.
  return computeTightTextureUploadLayout(Desc);
}

const Capabilities &VulkanDevice::getCapabilities() {
  if (Caps.empty())
    queryCapabilities();
  return Caps;
}

void VulkanDevice::printExtra(llvm::raw_ostream &OS) {
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

const VkPhysicalDeviceProperties &VulkanDevice::getProps() const {
  return Props;
}

void VulkanDevice::queryCapabilities() {
  const bool HasVulkan12 = Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 2, 0);
  const bool HasVulkan13 = Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 3, 0);
#ifdef VK_VERSION_1_4
  const bool HasVulkan14 = Props.apiVersion >= VK_MAKE_API_VERSION(0, 1, 4, 0);
#endif

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
#ifdef VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME
  VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT FeaturesImageAtomicInt64{};
  FeaturesImageAtomicInt64.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT;
  const bool HasShaderImageAtomicInt64Ext = isExtensionSupported(
      DeviceExtensions, VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);
#endif

  Features.pNext = &Features11;
  if (HasVulkan12)
    Features11.pNext = &Features12;
  if (HasVulkan13)
    Features12.pNext = &Features13;
#ifdef VK_VERSION_1_4
  if (HasVulkan14)
    Features13.pNext = &Features14;
#endif
#ifdef VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME
  // Append the VK_EXT_shader_image_atomic_int64 features struct to the
  // pNext chain, but only if the device advertises the extension --
  // otherwise drivers may reject the unknown sType. The chain above is
  // built version-by-version (11 -> 12 -> 13 -> 14), so the correct
  // attachment point is whichever Features1X struct is currently the
  // tail for this device's apiVersion.
  if (HasShaderImageAtomicInt64Ext) {
#ifdef VK_VERSION_1_4
    if (HasVulkan14)
      Features14.pNext = &FeaturesImageAtomicInt64;
    else
#endif
        if (HasVulkan13)
      Features13.pNext = &FeaturesImageAtomicInt64;
    else if (HasVulkan12)
      Features12.pNext = &FeaturesImageAtomicInt64;
    else
      Features11.pNext = &FeaturesImageAtomicInt64;
  }
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
#ifdef VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME
#define VULKAN_EXT_SHADER_IMAGE_ATOMIC_INT64_FEATURE_BOOL(Name)                \
  Caps.insert(std::make_pair(                                                  \
      #Name, makeCapability<bool>(#Name, HasShaderImageAtomicInt64Ext &&       \
                                             FeaturesImageAtomicInt64.Name)));
#endif
#include "VKFeatures.def"
}

llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
VulkanDevice::createCommandBuffer() {
  auto CBOrErr = VulkanCommandBuffer::create(
      Device, GraphicsQueue.QueueFamilyIdx, CmdBeginDebugUtilsLabel,
      CmdEndDebugUtilsLabel, CmdInsertDebugUtilsLabel, MeshShaderFns);
  if (!CBOrErr)
    return CBOrErr.takeError();
  (*CBOrErr)->Dev = this;
  return std::unique_ptr<offloadtest::CommandBuffer>(std::move(*CBOrErr));
}

llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
VulkanDevice::createRenderPass(const offloadtest::RenderPassDesc &Desc) {
  llvm::SmallVector<VkAttachmentDescription, 9> Attachments;
  llvm::SmallVector<VkAttachmentReference, 8> ColorRefs;

  for (const ColorAttachmentFormatDesc &Color : Desc.ColorAttachments) {
    VkAttachmentDescription AD = {};
    AD.format = getVulkanFormat(Color.Fmt);
    AD.samples = VK_SAMPLE_COUNT_1_BIT;
    AD.loadOp = getVkLoadOp(Color.Load);
    AD.storeOp = getVkStoreOp(Color.Store);
    AD.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    AD.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // If we are loading, the layout MUST be defined and in color attachment
    // optimal state.
    // If we are NOT loading (clearing or don't care), we are discarding the
    // original contents of the texture, and use an undefined layout. This
    // allows us to receive a texture in _any_ layout including uninitialized
    // textures.
    AD.initialLayout = Color.Load == LoadAction::Load
                           ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                           : VK_IMAGE_LAYOUT_UNDEFINED;
    AD.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference Ref = {};
    Ref.attachment = static_cast<uint32_t>(Attachments.size());
    Ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    Attachments.push_back(AD);
    ColorRefs.push_back(Ref);
  }

  VkAttachmentReference DepthReference = {};
  if (Desc.DepthStencil) {
    const auto &DS = *Desc.DepthStencil;
    VkAttachmentDescription AD = {};
    AD.format = getVulkanFormat(DS.Fmt);
    AD.samples = VK_SAMPLE_COUNT_1_BIT;
    AD.loadOp = getVkLoadOp(DS.DepthLoad);
    AD.storeOp = getVkStoreOp(DS.DepthStore);
    AD.stencilLoadOp = getVkLoadOp(DS.StencilLoad);
    AD.stencilStoreOp = getVkStoreOp(DS.StencilStore);
    AD.initialLayout =
        (DS.DepthLoad == LoadAction::Load || DS.StencilLoad == LoadAction::Load)
            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : VK_IMAGE_LAYOUT_UNDEFINED;
    AD.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    DepthReference.attachment = static_cast<uint32_t>(Attachments.size());
    DepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    Attachments.push_back(AD);
  }

  VkSubpassDescription Subpass = {};
  Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  Subpass.colorAttachmentCount = static_cast<uint32_t>(ColorRefs.size());
  Subpass.pColorAttachments = ColorRefs.data();
  Subpass.pDepthStencilAttachment =
      Desc.DepthStencil ? &DepthReference : nullptr;

  VkRenderPassCreateInfo RPCI = {};
  RPCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  RPCI.attachmentCount = static_cast<uint32_t>(Attachments.size());
  RPCI.pAttachments = Attachments.data();
  RPCI.subpassCount = 1;
  RPCI.pSubpasses = &Subpass;

  VkRenderPass Handle = VK_NULL_HANDLE;
  if (auto Err =
          VK::toError(vkCreateRenderPass(Device, &RPCI, nullptr, &Handle),
                      "Failed to create render pass."))
    return Err;
  return std::make_unique<VulkanRenderPass>(Device, Handle, Desc);
}

llvm::Expected<VulkanDevice::BufferRef>
VulkanDevice::createBufferWithDeviceAddress(VkDeviceSize Size,
                                            VkBufferUsageFlags ExtraUsage) {
  return createBuffer(ExtraUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, Size, nullptr,
                      VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
}

llvm::Expected<AccelerationStructureSizes> VulkanDevice::getBLASBuildSizes(
    llvm::ArrayRef<TriangleGeometryDesc> Triangles) {
  if (auto Err = validateBLASGeometry(Triangles))
    return Err;

  llvm::SmallVector<VkAccelerationStructureGeometryKHR> Geoms;
  Geoms.reserve(Triangles.size());

  llvm::SmallVector<uint32_t> MaxPrimCounts;
  MaxPrimCounts.reserve(Triangles.size());

  for (const auto &T : Triangles) {
    VkAccelerationStructureGeometryKHR Geom = {};
    Geom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    Geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    if (T.Opaque)
      Geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    auto &Tri = Geom.geometry.triangles;
    Tri.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    // Device addresses are not needed for the size query; they will be
    // populated at build time.
    Tri.vertexStride = T.VertexStride;
    Tri.maxVertex = T.VertexCount - 1;
    Tri.vertexFormat = getVulkanFormat(T.VertexFormat);
    Tri.indexType = T.IndexBuffer ? getVulkanIndexType(T.IdxFormat)
                                  : VK_INDEX_TYPE_NONE_KHR;

    Geoms.push_back(Geom);
    MaxPrimCounts.push_back(T.IndexBuffer ? T.IndexCount / 3
                                          : T.VertexCount / 3);
  }

  return queryBLASPrebuildSize(Geoms, MaxPrimCounts);
}

llvm::Expected<AccelerationStructureSizes>
VulkanDevice::getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) {
  if (auto Err = validateBLASGeometry(AABBs))
    return Err;

  llvm::SmallVector<VkAccelerationStructureGeometryKHR> Geoms;
  Geoms.reserve(AABBs.size());

  llvm::SmallVector<uint32_t> MaxPrimCounts;
  MaxPrimCounts.reserve(AABBs.size());

  for (const auto &A : AABBs) {
    VkAccelerationStructureGeometryKHR Geom = {};
    Geom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    Geom.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
    if (A.Opaque)
      Geom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

    auto &Aabbs = Geom.geometry.aabbs;
    Aabbs.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
    Aabbs.stride = A.AABBStride;

    Geoms.push_back(Geom);
    MaxPrimCounts.push_back(A.AABBCount);
  }

  return queryBLASPrebuildSize(Geoms, MaxPrimCounts);
}

AccelerationStructureSizes VulkanDevice::queryBLASPrebuildSize(
    llvm::ArrayRef<VkAccelerationStructureGeometryKHR> Geoms,
    llvm::ArrayRef<uint32_t> MaxPrimCounts) {
  VkAccelerationStructureBuildGeometryInfoKHR BuildInfo = {};
  BuildInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  BuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  BuildInfo.geometryCount = Geoms.size();
  BuildInfo.pGeometries = Geoms.data();

  VkAccelerationStructureBuildSizesInfoKHR SizesInfo = {};
  SizesInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  AS.GetBuildSizes(Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                   &BuildInfo, MaxPrimCounts.data(), &SizesInfo);

  return {SizesInfo.accelerationStructureSize, SizesInfo.buildScratchSize,
          SizesInfo.updateScratchSize};
}

llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
VulkanDevice::allocateAS(const AccelerationStructureSizes &Sizes,
                         VkAccelerationStructureTypeKHR Type,
                         const char *Kind) {
  if (!HasASSupport)
    return llvm::createStringError(
        std::errc::not_supported,
        "Ray tracing is not supported on this device.");

  auto BufOrErr = createBufferWithDeviceAddress(
      Sizes.ResultDataMaxSizeInBytes,
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
  if (!BufOrErr)
    return BufOrErr.takeError();

  VkAccelerationStructureCreateInfoKHR CreateInfo = {};
  CreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  CreateInfo.buffer = BufOrErr->Buffer;
  CreateInfo.size = Sizes.ResultDataMaxSizeInBytes;
  CreateInfo.type = Type;

  VkAccelerationStructureKHR AccelStruct = VK_NULL_HANDLE;
  if (auto Err =
          VK::toError(AS.Create(Device, &CreateInfo, nullptr, &AccelStruct),
                      "Failed to create " + llvm::Twine(Kind) + ".")) {
    vkDestroyBuffer(Device, BufOrErr->Buffer, nullptr);
    vkFreeMemory(Device, BufOrErr->Memory, nullptr);
    return Err;
  }
  VkAccelerationStructureDeviceAddressInfoKHR AddrInfo = {};
  AddrInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  AddrInfo.accelerationStructure = AccelStruct;
  const VkDeviceAddress DevAddr = AS.GetDeviceAddress(Device, &AddrInfo);

  return std::make_unique<VulkanAccelerationStructure>(
      Device, AccelStruct, BufOrErr->Buffer, BufOrErr->Memory, DevAddr,
      AS.Destroy, Sizes);
}

llvm::Expected<AccelerationStructureSizes>
VulkanDevice::getTLASBuildSizes(uint32_t InstanceCount) {
  VkAccelerationStructureGeometryKHR Geom = {};
  Geom.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  Geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  Geom.geometry.instances.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;

  VkAccelerationStructureBuildGeometryInfoKHR BuildInfo = {};
  BuildInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  BuildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  BuildInfo.geometryCount = 1;
  BuildInfo.pGeometries = &Geom;

  VkAccelerationStructureBuildSizesInfoKHR SizesInfo = {};
  SizesInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  AS.GetBuildSizes(Device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                   &BuildInfo, &InstanceCount, &SizesInfo);

  return AccelerationStructureSizes{SizesInfo.accelerationStructureSize,
                                    SizesInfo.buildScratchSize,
                                    SizesInfo.updateScratchSize};
}

llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
VulkanDevice::createBLAS(const AccelerationStructureSizes &Sizes) {
  return allocateAS(Sizes, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
                    "BLAS");
}

llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
VulkanDevice::createTLAS(const AccelerationStructureSizes &Sizes,
                         uint32_t /*InstanceCount*/) {
  return allocateAS(Sizes, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
                    "TLAS");
}

llvm::Expected<std::unique_ptr<DescriptorPool>>
VulkanDevice::createDescriptorPool() {
  return VulkanDescriptorPool::create(Device);
}

llvm::Expected<std::unique_ptr<DescriptorSetsBuilder>>
VulkanDevice::createDescriptorSetsBuilder(DescriptorPool &Pool,
                                          const PipelineState &PSO) {
  const VulkanDescriptorPool &PoolVK = llvm::cast<VulkanDescriptorPool>(Pool);
  const VulkanPipelineState &PipelineVK = llvm::cast<VulkanPipelineState>(PSO);

  llvm::SmallVector<VkDescriptorSet> DescriptorSets;
  if (!PipelineVK.SetLayouts.empty()) {
    VkDescriptorSetAllocateInfo DSAllocInfo = {};
    DSAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    DSAllocInfo.descriptorPool = PoolVK.Pool;
    DSAllocInfo.descriptorSetCount = PipelineVK.SetLayouts.size();
    DSAllocInfo.pSetLayouts = PipelineVK.SetLayouts.data();
    assert(DescriptorSets.empty());

    DescriptorSets.insert(DescriptorSets.begin(), PipelineVK.SetLayouts.size(),
                          VkDescriptorSet());
    if (auto Err = VK::toError(vkAllocateDescriptorSets(Device, &DSAllocInfo,
                                                        DescriptorSets.data()),
                               "Failed to allocate descriptor sets."))
      return Err;
  }

  return std::make_unique<VulkanDescriptorSetsBuilder>(
      Device, std::move(DescriptorSets), PipelineVK.DescCounts);
}

llvm::Expected<VulkanDevice::BufferRef>
VulkanDevice::createBuffer(VkBufferUsageFlags Usage,
                           VkMemoryPropertyFlags MemoryFlags, size_t Size,
                           void *Data, VkMemoryAllocateFlags AllocFlags) {
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

  VkMemoryAllocateFlagsInfo FlagsInfo = {};
  VkMemoryAllocateInfo AllocInfo = {};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.allocationSize = MemReqs.size;
  if (AllocFlags) {
    FlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    FlagsInfo.flags = AllocFlags;
    AllocInfo.pNext = &FlagsInfo;
  }

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
    if (auto Err =
            VK::toError(vkMapMemory(Device, Memory, 0, VK_WHOLE_SIZE, 0, &Dst),
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

llvm::Expected<VkSpecializationMapEntry>
VulkanDevice::parseSpecializationConstant(
    const SpecializationConstant &SpecConst,
    llvm::SmallVectorImpl<char> &SpecData) {
  VkSpecializationMapEntry Entry = {};
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

  return Entry;
}

llvm::Error VulkanDevice::parseSpecializationConstants(
    llvm::ArrayRef<SpecializationConstant> SpecializationConstants,
    llvm::SmallVectorImpl<VkSpecializationMapEntry> &SpecEntries,
    llvm::SmallVectorImpl<char> &SpecData, VkSpecializationInfo &SpecInfo) {

  if (SpecializationConstants.empty()) {
    SpecInfo = {};
    return llvm::Error::success();
  }

  llvm::DenseSet<uint32_t> SeenConstantIDs;
  for (const auto &SpecConst : SpecializationConstants) {
    if (!SeenConstantIDs.insert(SpecConst.ConstantID).second)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Test configuration contains multiple entries for "
          "specialization constant ID %u.",
          SpecConst.ConstantID);

    auto EntryOrErr = parseSpecializationConstant(SpecConst, SpecData);
    if (!EntryOrErr)
      return EntryOrErr.takeError();
    SpecEntries.push_back(*EntryOrErr);
  }

  SpecInfo.mapEntryCount = SpecEntries.size();
  SpecInfo.pMapEntries = SpecEntries.data();
  SpecInfo.dataSize = SpecData.size();
  SpecInfo.pData = SpecData.data();

  return llvm::Error::success();
}

llvm::Error VulkanDevice::executeProgram(Pipeline &P) {
  return executeUnitTest(*this, P);
}

// === Ray tracing pipeline + SBT + DispatchRays ============================

static VkRayTracingShaderGroupTypeKHR getRTGroupType(HitGroupType T) {
  switch (T) {
  case HitGroupType::Triangles:
    return VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  case HitGroupType::Procedural:
    return VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
  }
  llvm_unreachable("All HitGroupType cases handled");
}

llvm::Expected<std::unique_ptr<PipelineState>>
VulkanDevice::createPipelineRT(llvm::StringRef Name, const BindingsDesc &BD,
                               const RayTracingPipelineCreateDesc &Desc) {
  if (!HasRTPipelineSupport)
    return llvm::createStringError(
        std::errc::not_supported,
        "Device does not support VK_KHR_ray_tracing_pipeline");
  if (!Desc.Library)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "RayTracingPipelineCreateDesc.Library is "
                                   "null — backend needs a DXIL/SPIR-V blob.");

  // Single shader module backs every RT entry point — the DXIL library
  // compiles to one SPIR-V module with multiple OpEntryPoints.
  auto ModOrErr = createShaderModule(Desc.Library, "raytracing library");
  if (!ModOrErr)
    return ModOrErr.takeError();
  VkShaderModule Module = *ModOrErr;
  auto ModuleCleanup =
      llvm::scope_exit([&] { vkDestroyShaderModule(Device, Module, nullptr); });

  // Pipeline layout: every RT stage may consume any binding from the global
  // descriptor sets (mirrors a DX12 global root signature).
  const VkShaderStageFlags AllRTStages =
      VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR |
      VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
      VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR;
  llvm::SmallVector<VkDescriptorSetLayout> SetLayouts;
  VkPipelineLayout Layout = VK_NULL_HANDLE;
  DescriptorCounts DescCounts = {};
  if (auto Err =
          createPipelineLayout(BD, AllRTStages, SetLayouts, Layout, DescCounts))
    return Err;
  auto LayoutCleanup = llvm::scope_exit([&] {
    if (Layout != VK_NULL_HANDLE)
      vkDestroyPipelineLayout(Device, Layout, nullptr);
    for (auto *L : SetLayouts)
      vkDestroyDescriptorSetLayout(Device, L, nullptr);
  });

  llvm::SmallVector<VkPipelineShaderStageCreateInfo> StageCIs;
  StageCIs.reserve(Desc.Shaders.size());
  llvm::StringMap<uint32_t> EntryToStageIdx;
  for (const auto &Sh : Desc.Shaders) {
    VkPipelineShaderStageCreateInfo CI{};
    CI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    CI.stage = getShaderStageFlag(Sh.Stage);
    CI.module = Module;
    CI.pName = Sh.EntryPoint.c_str();
    EntryToStageIdx[Sh.EntryPoint] = static_cast<uint32_t>(StageCIs.size());
    StageCIs.push_back(CI);
  }

  llvm::SmallVector<VkRayTracingShaderGroupCreateInfoKHR> Groups;
  llvm::StringMap<uint32_t> NameToGroup;
  auto AddGeneralGroup = [&](llvm::StringRef Key, uint32_t StageIdx) {
    VkRayTracingShaderGroupCreateInfoKHR G{};
    G.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    G.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    G.generalShader = StageIdx;
    G.closestHitShader = VK_SHADER_UNUSED_KHR;
    G.anyHitShader = VK_SHADER_UNUSED_KHR;
    G.intersectionShader = VK_SHADER_UNUSED_KHR;
    NameToGroup[Key] = static_cast<uint32_t>(Groups.size());
    Groups.push_back(G);
  };

  uint32_t NumRG = 0, NumMS = 0, NumHG = 0, NumCL = 0;
  for (const auto &Sh : Desc.Shaders)
    if (Sh.Stage == Stages::RayGeneration) {
      AddGeneralGroup(Sh.EntryPoint, EntryToStageIdx[Sh.EntryPoint]);
      ++NumRG;
    }
  for (const auto &Sh : Desc.Shaders)
    if (Sh.Stage == Stages::Miss) {
      AddGeneralGroup(Sh.EntryPoint, EntryToStageIdx[Sh.EntryPoint]);
      ++NumMS;
    }
  for (const auto &HG : Desc.HitGroups) {
    VkRayTracingShaderGroupCreateInfoKHR G{};
    G.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    G.type = getRTGroupType(HG.Type);
    G.generalShader = VK_SHADER_UNUSED_KHR;
    auto FindIdx = [&](const std::string &Entry) -> uint32_t {
      auto It = EntryToStageIdx.find(Entry);
      return It == EntryToStageIdx.end() ? VK_SHADER_UNUSED_KHR : It->second;
    };
    G.closestHitShader = FindIdx(HG.ClosestHit);
    G.anyHitShader = HG.AnyHit ? FindIdx(*HG.AnyHit) : VK_SHADER_UNUSED_KHR;
    G.intersectionShader =
        HG.Intersection ? FindIdx(*HG.Intersection) : VK_SHADER_UNUSED_KHR;
    NameToGroup[HG.Name] = static_cast<uint32_t>(Groups.size());
    Groups.push_back(G);
    ++NumHG;
  }
  for (const auto &Sh : Desc.Shaders)
    if (Sh.Stage == Stages::Callable) {
      AddGeneralGroup(Sh.EntryPoint, EntryToStageIdx[Sh.EntryPoint]);
      ++NumCL;
    }

  VkRayTracingPipelineCreateInfoKHR PipelineCI{};
  PipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  PipelineCI.stageCount = static_cast<uint32_t>(StageCIs.size());
  PipelineCI.pStages = StageCIs.data();
  PipelineCI.groupCount = static_cast<uint32_t>(Groups.size());
  PipelineCI.pGroups = Groups.data();
  PipelineCI.maxPipelineRayRecursionDepth = Desc.Config.MaxTraceRecursionDepth;
  PipelineCI.layout = Layout;

  VkPipeline Pipeline = VK_NULL_HANDLE;
  if (auto Err =
          VK::toError(RT.CreatePipelines(Device, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                         1, &PipelineCI, nullptr, &Pipeline),
                      "Failed to create ray tracing pipeline."))
    return Err;

  auto State = std::make_unique<VKRayTracingPipelineState>(
      Name, Device, Pipeline, Layout, std::move(SetLayouts), DescCounts);
  State->ShaderGroupIndices = std::move(NameToGroup);
  State->NumRaygenGroups = NumRG;
  State->NumMissGroups = NumMS;
  State->NumHitGroups = NumHG;
  State->NumCallableGroups = NumCL;
  // Ownership transferred — disable cleanup.
  Layout = VK_NULL_HANDLE;
  SetLayouts.clear();
  return State;
}

llvm::Expected<std::unique_ptr<ShaderBindingTable>>
VulkanDevice::createShaderBindingTable(const PipelineState &PSO,
                                       const ShaderBindingTableDesc &Desc) {
  if (!HasRTPipelineSupport)
    return llvm::createStringError(
        std::errc::not_supported,
        "Device does not support VK_KHR_ray_tracing_pipeline");
  if (!llvm::isa<VKRayTracingPipelineState>(&PSO))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "createShaderBindingTable requires a RayTracing PipelineState");
  const auto &VKPSO = llvm::cast<VKRayTracingPipelineState>(PSO);

  const uint32_t HandleSize = RTPipelineProps.shaderGroupHandleSize;
  const SBTLayout Layout =
      computeSBTLayout(HandleSize, RTPipelineProps.shaderGroupHandleAlignment,
                       RTPipelineProps.shaderGroupBaseAlignment, Desc);
  const VkDeviceSize TotalSize = Layout.TotalSize;
  // Vulkan dispatches a single raygen per vkCmdTraceRaysKHR; the descriptor
  // only carries one raygen entry, so its region holds exactly one record.
  const llvm::ArrayRef<SBTEntry> RGEntries(&Desc.RayGen, 1);

  // Pull all shader group handles at once. Vulkan returns them in pipeline
  // order matching the order groups were given to vkCreateRayTracingPipelines.
  llvm::SmallVector<uint8_t> AllHandles(VKPSO.totalGroupCount() * HandleSize);
  if (auto Err = VK::toError(
          RT.GetGroupHandles(Device, VKPSO.Pipeline, 0, VKPSO.totalGroupCount(),
                             AllHandles.size(), AllHandles.data()),
          "vkGetRayTracingShaderGroupHandlesKHR failed."))
    return Err;

  // Allocate the SBT in a host-visible coherent buffer (PR2 simplification —
  // a staging-copy to a device-local buffer is a follow-up optimization).
  VkBufferCreateInfo BufInfo{};
  BufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  BufInfo.size = TotalSize;
  BufInfo.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  BufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkBuffer Buffer = VK_NULL_HANDLE;
  if (auto Err = VK::toError(vkCreateBuffer(Device, &BufInfo, nullptr, &Buffer),
                             "Failed to create SBT buffer."))
    return Err;

  VkMemoryRequirements MemReqs;
  vkGetBufferMemoryRequirements(Device, Buffer, &MemReqs);
  VkMemoryAllocateFlagsInfo AllocFlagsInfo{};
  AllocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  AllocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
  VkMemoryAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.pNext = &AllocFlagsInfo;
  AllocInfo.allocationSize = MemReqs.size;
  auto MemIdx = getMemoryIndex(PhysicalDevice, MemReqs.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  if (!MemIdx) {
    vkDestroyBuffer(Device, Buffer, nullptr);
    return MemIdx.takeError();
  }
  AllocInfo.memoryTypeIndex = *MemIdx;
  VkDeviceMemory Memory = VK_NULL_HANDLE;
  if (auto Err =
          VK::toError(vkAllocateMemory(Device, &AllocInfo, nullptr, &Memory),
                      "Failed to allocate SBT memory.")) {
    vkDestroyBuffer(Device, Buffer, nullptr);
    return Err;
  }
  if (auto Err = VK::toError(vkBindBufferMemory(Device, Buffer, Memory, 0),
                             "Failed to bind SBT memory.")) {
    vkFreeMemory(Device, Memory, nullptr);
    vkDestroyBuffer(Device, Buffer, nullptr);
    return Err;
  }

  void *MappedRaw = nullptr;
  if (auto Err = VK::toError(
          vkMapMemory(Device, Memory, 0, VK_WHOLE_SIZE, 0, &MappedRaw),
          "Failed to map SBT memory.")) {
    vkFreeMemory(Device, Memory, nullptr);
    vkDestroyBuffer(Device, Buffer, nullptr);
    return Err;
  }
  auto *Mapped = static_cast<uint8_t *>(MappedRaw);
  std::memset(Mapped, 0, TotalSize);

  // Resolve each SBT entry's ShaderName → shader-group index, then write
  // [handle][localRootData][pad] into the region at the right offset.
  auto WriteEntries = [&](uint8_t *Region, llvm::ArrayRef<SBTEntry> Entries,
                          uint32_t Stride) -> llvm::Error {
    for (size_t I = 0; I < Entries.size(); ++I) {
      const auto &E = Entries[I];
      auto It = VKPSO.ShaderGroupIndices.find(E.ShaderName);
      if (It == VKPSO.ShaderGroupIndices.end())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "SBT references unknown shader/hit-group name: '%s'",
            E.ShaderName.c_str());
      uint8_t *Dst = Region + I * Stride;
      std::memcpy(Dst, AllHandles.data() + It->second * HandleSize, HandleSize);
      if (!E.LocalRootData.empty())
        std::memcpy(Dst + HandleSize, E.LocalRootData.data(),
                    E.LocalRootData.size());
    }
    return llvm::Error::success();
  };

  auto WriteRegion = [&](const SBTRegionLayout &R,
                         llvm::ArrayRef<SBTEntry> Entries) -> llvm::Error {
    return WriteEntries(Mapped + R.Offset, Entries, R.Stride);
  };
  auto CleanupAndReturn = [&](llvm::Error Err) {
    vkUnmapMemory(Device, Memory);
    vkFreeMemory(Device, Memory, nullptr);
    vkDestroyBuffer(Device, Buffer, nullptr);
    return Err;
  };
  if (auto Err = WriteRegion(Layout.RayGen, RGEntries))
    return CleanupAndReturn(std::move(Err));
  if (auto Err = WriteRegion(Layout.Miss, Desc.Miss))
    return CleanupAndReturn(std::move(Err));
  if (auto Err = WriteRegion(Layout.HitGroup, Desc.HitGroup))
    return CleanupAndReturn(std::move(Err));
  if (auto Err = WriteRegion(Layout.Callable, Desc.Callable))
    return CleanupAndReturn(std::move(Err));
  vkUnmapMemory(Device, Memory);

  VkBufferDeviceAddressInfo AddrInfo{};
  AddrInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  AddrInfo.buffer = Buffer;
  const VkDeviceAddress Base = vkGetBufferDeviceAddress(Device, &AddrInfo);

  // VkStridedDeviceAddressRegionKHR uses a zero deviceAddress to signal an
  // empty region — matching what the SBT layout helper records as Size == 0.
  auto MakeRegion = [&](const SBTRegionLayout &R) {
    return VkStridedDeviceAddressRegionKHR{R.Size ? Base + R.Offset : 0,
                                           R.Stride, R.Size};
  };
  const VkStridedDeviceAddressRegionKHR RG = MakeRegion(Layout.RayGen);
  const VkStridedDeviceAddressRegionKHR MS = MakeRegion(Layout.Miss);
  const VkStridedDeviceAddressRegionKHR HG = MakeRegion(Layout.HitGroup);
  const VkStridedDeviceAddressRegionKHR CL = MakeRegion(Layout.Callable);
  return std::make_unique<VKShaderBindingTable>(Device, Buffer, Memory, RG, MS,
                                                HG, CL);
}

} // namespace offloadtest

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
  bool DebugUtilsEnabled = false;
  if (Config.EnableDebugLayer || Config.EnableValidationLayer) {
    const llvm::StringRef DebugUtilsExtensionName = "VK_EXT_debug_utils";
    if (isExtensionSupported(AvailableExtensions, DebugUtilsExtensionName)) {
      EnabledInstanceExtensions.push_back(DebugUtilsExtensionName.data());
      DebugUtilsEnabled = true;
    }
  }

  CreateInfo.ppEnabledLayerNames = EnabledLayers.data();
  CreateInfo.enabledLayerCount = EnabledLayers.size();
  CreateInfo.ppEnabledExtensionNames = EnabledInstanceExtensions.data();
  CreateInfo.enabledExtensionCount = EnabledInstanceExtensions.size();

  VkInstance Instance = VK_NULL_HANDLE;
  if (auto Err = VK::toError(vkCreateInstance(&CreateInfo, NULL, &Instance),
                             "Failed to create Vulkan instance"))
    return Err;

  VkDebugUtilsMessengerEXT DebugMessenger =
      DebugUtilsEnabled ? registerDebugUtilCallback(Instance) : VK_NULL_HANDLE;

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
