//===- VK/Queue.cpp - Vulkan Queue API ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/VK/Queue.h"
#include "API/VK/CommandBuffer.h"
#include "Support/VkError.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>

using namespace offloadtest;

llvm::Expected<std::unique_ptr<VulkanFence>>
VulkanFence::create(VkDevice Device, llvm::StringRef Name) {
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

VulkanFence::~VulkanFence() { vkDestroySemaphore(Device, Semaphore, nullptr); }

uint64_t VulkanFence::getFenceValue() {
  uint64_t Value = 0;
  [[maybe_unused]] const VkResult Ret =
      vkGetSemaphoreCounterValue(Device, Semaphore, &Value);
  assert(!Ret && "vkGetSemaphoreCounterValue failed but should never fail.");
  return Value;
}

llvm::Error VulkanFence::waitForCompletion(uint64_t SignalValue) {
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

  // Each command buffer defaults to src=HOST, which is only correct for
  // standalone submissions.  Multi-CB batches would need inter-CB barriers.
  if (CBs.size() > 1)
    llvm::errs()
        << "Warning: submitting multiple command buffers in a single batch; "
           "encoder barriers do not account for inter-command-buffer "
           "dependencies.\n";
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
