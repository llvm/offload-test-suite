//===- VK/Buffer.h - Offload API VK Buffer API ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_BUFFER_H
#define OFFLOADTEST_API_VK_BUFFER_H

#include "API/Buffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace offloadtest {

class VulkanBuffer : public offloadtest::Buffer {
public:
  VkDevice Dev; // Needed for clean-up
  VkBuffer Buffer;
  VkBuffer CounterBuffer;
  VkDeviceMemory Memory;
  VkDeviceAddress DeviceAddress;
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;
  VkBufferView View;

  VulkanBuffer(VkDevice Dev, VkBuffer Buffer, VkBuffer CounterBuffer,
               VkDeviceMemory Memory, VkDeviceAddress DeviceAddress,
               llvm::StringRef Name, BufferCreateDesc Desc, size_t SizeInBytes,
               VkBufferView View)
      : offloadtest::Buffer(GPUAPI::Vulkan), Dev(Dev), Buffer(Buffer),
        CounterBuffer(CounterBuffer), Memory(Memory),
        DeviceAddress(DeviceAddress), Name(Name), Desc(Desc),
        SizeInBytes(SizeInBytes), View(View) {}

  VulkanBuffer(const VulkanBuffer &) = delete;
  VulkanBuffer(VulkanBuffer &&) = delete;
  VulkanBuffer &operator=(const VulkanBuffer &) = delete;
  VulkanBuffer &operator=(VulkanBuffer &&) = delete;

  size_t getSizeInBytes() const override;

  size_t querySparseTileSizeInBytes(const Device & /*Dev*/) const override;

  llvm::Expected<void *> map() override;

  void unmap() override;

  ~VulkanBuffer() override;

  // Only valid when the buffer was created with VK_BUFFER_USAGE_SHADER_DEVICE_
  // ADDRESS_BIT, which the device adds whenever ray tracing is supported.
  VkDeviceAddress getDeviceAddress() const { return DeviceAddress; }

  const BufferCreateDesc &getDesc() const override;

  static bool classof(const offloadtest::Buffer *B);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_BUFFER_H
