//===- Encoder.h - Offload API Command Encoder Abstraction ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_ENCODER_H
#define OFFLOADTEST_API_ENCODER_H

#include "API/API.h"

#include "llvm/Support/Error.h"

#include <cstdint>

namespace offloadtest {

/// Base class for all command encoders. An encoder records commands into a
/// command buffer. Call endEncoding() when done recording. Barriers are
/// automatically inserted between commands.
class CommandEncoder {
  GPUAPI API;

public:
  explicit CommandEncoder(GPUAPI API) : API(API) {}
  virtual ~CommandEncoder();
  CommandEncoder(const CommandEncoder &) = delete;
  CommandEncoder &operator=(const CommandEncoder &) = delete;

  GPUAPI getAPI() const { return API; }

  /// Finish recording. No further commands may be recorded after this call.
  virtual void endEncoding() = 0;
};

/// Encoder for recording compute dispatch commands.
class ComputeEncoder : public CommandEncoder {
public:
  using CommandEncoder::CommandEncoder;

  /// Dispatch a compute grid. ThreadsPerGroup specifies the workgroup
  /// dimensions; GroupCount specifies how many groups to launch in each
  /// dimension. Total threads = ThreadsPerGroup * GroupCount per axis.
  virtual llvm::Error dispatch(uint32_t GroupCountX, uint32_t GroupCountY,
                               uint32_t GroupCountZ, uint32_t ThreadsPerGroupX,
                               uint32_t ThreadsPerGroupY,
                               uint32_t ThreadsPerGroupZ) = 0;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENCODER_H
