//===- VK/Queue.h - Offload API VK Queue API ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VK_QUEUE_H
#define OFFLOADTEST_API_VK_QUEUE_H

#include "API/Device.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

namespace offloadtest {

class VulkanFence : public offloadtest::Fence {
public:
  VulkanFence(VkDevice Device, VkSemaphore Semaphore, llvm::StringRef Name)
      : Name(Name), Device(Device), Semaphore(Semaphore) {}

  std::string Name;
  VkDevice Device;
  VkSemaphore Semaphore;

  static llvm::Expected<std::unique_ptr<VulkanFence>>
  create(VkDevice Device, llvm::StringRef Name);

  ~VulkanFence();

  uint64_t getFenceValue() override;

  llvm::Error waitForCompletion(uint64_t SignalValue) override;
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

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Buffer & /*Resource*/,
                     llvm::ArrayRef<TileMapping> /*Mappings*/) override {
    return llvm::createStringError(
        std::errc::not_supported,
        "Vulkan backend does not yet support tile mappings.");
  }

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Texture & /*Resource*/,
                     llvm::ArrayRef<TileMapping> /*Mappings*/) override {
    return llvm::createStringError(
        std::errc::not_supported,
        "Vulkan backend does not yet support tile mappings.");
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VK_QUEUE_H
