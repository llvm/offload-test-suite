//===- VK/Buffer.cpp - Vulkan Buffer API ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/Buffer.h"

#include "llvm/Support/Error.h"

#include <system_error>

using namespace offloadtest;

size_t VulkanBuffer::getSizeInBytes() const { return SizeInBytes; }

size_t VulkanBuffer::querySparseTileSizeInBytes(const Device & /*Dev*/) const {
  VkMemoryRequirements MemReqs;
  vkGetBufferMemoryRequirements(Dev, Buffer, &MemReqs);
  return MemReqs.alignment;
}

llvm::Expected<void *> VulkanBuffer::map() {
  if (Desc.Location == MemoryLocation::GpuOnly)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Cannot map a GpuOnly buffer.");
  void *Ptr = nullptr;
  if (vkMapMemory(Dev, Memory, 0, SizeInBytes, 0, &Ptr) != VK_SUCCESS)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed to map buffer.");
  // HOST_CACHED memory that is *not* HOST_COHERENT (GpuToCpu) needs explicit
  // invalidation so the CPU sees the GPU-side writes.
  if (Desc.Location == MemoryLocation::GpuToCpu) {
    VkMappedMemoryRange Range = {};
    Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    Range.memory = Memory;
    Range.offset = 0;
    Range.size = VK_WHOLE_SIZE;
    vkInvalidateMappedMemoryRanges(Dev, 1, &Range);
  }
  return Ptr;
}

void VulkanBuffer::unmap() { vkUnmapMemory(Dev, Memory); }

VulkanBuffer::~VulkanBuffer() {
  if (View != nullptr)
    vkDestroyBufferView(Dev, View, nullptr);
  if (CounterBuffer != nullptr)
    vkDestroyBuffer(Dev, CounterBuffer, nullptr);
  vkDestroyBuffer(Dev, Buffer, nullptr);
  vkFreeMemory(Dev, Memory, nullptr);
}

const BufferCreateDesc &VulkanBuffer::getDesc() const { return Desc; }

bool VulkanBuffer::classof(const offloadtest::Buffer *B) {
  return B->getAPI() == GPUAPI::Vulkan;
}
