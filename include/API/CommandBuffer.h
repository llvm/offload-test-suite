//===- CommandBuffer.h - Offload Command Buffer API -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the CommandBuffer base class for recording and submitting GPU work.
// Each backend (DirectX, Vulkan, Metal) provides a concrete subclass that
// wraps the native command recording objects. LLVM-style RTTI is provided for
// downcasting to the backend-specific type.
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_COMMANDBUFFER_H
#define OFFLOADTEST_API_COMMANDBUFFER_H

#include "API/API.h"
#include "API/Encoder.h"

#include "llvm/Support/Error.h"

#include <memory>

namespace offloadtest {

class CommandBuffer {
  GPUAPI Kind;

public:
  explicit CommandBuffer(GPUAPI Kind) : Kind(Kind) {}
  virtual ~CommandBuffer();
  CommandBuffer(const CommandBuffer &) = delete;
  CommandBuffer &operator=(const CommandBuffer &) = delete;

  GPUAPI getKind() const { return Kind; }

  /// Create a compute command encoder for recording dispatch commands.
  /// When \p Mode is Parallel, the backend ensures all prior work is visible
  /// (i.e., an implicit full barrier precedes the parallel encoding pass).
  virtual llvm::Expected<std::unique_ptr<ComputeEncoder>>
  createComputeEncoder(EncoderMode Mode) {
    return llvm::createStringError(
        std::errc::not_supported,
        "createComputeEncoder not implemented for this backend");
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_COMMANDBUFFER_H
