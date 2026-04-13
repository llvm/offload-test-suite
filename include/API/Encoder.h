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

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstddef>
#include <cstdint>

namespace offloadtest {

class Buffer;

/// Determines whether an encoder automatically inserts barriers between
/// commands.
enum class EncoderMode {
  /// Automatically insert a barrier between every command. Recommended default
  /// for most use cases.
  Serial,
  /// No automatic barriers. The caller is responsible for inserting barriers.
  Parallel,
};

/// Base class for all command encoders. An encoder records commands into a
/// command buffer. Call endEncoding() when done recording.
class CommandEncoder {
  GPUAPI API;
  EncoderMode Mode;
  bool Ended = false;

protected:
  /// Backend-specific cleanup. Called exactly once, either explicitly via
  /// endEncoding() or implicitly from the most-derived destructor.
  virtual void endEncodingImpl() = 0;

public:
  CommandEncoder(GPUAPI API, EncoderMode Mode) : API(API), Mode(Mode) {}
  virtual ~CommandEncoder();
  CommandEncoder(const CommandEncoder &) = delete;
  CommandEncoder &operator=(const CommandEncoder &) = delete;

  GPUAPI getAPI() const { return API; }
  EncoderMode getMode() const { return Mode; }
  bool isSerial() const { return Mode == EncoderMode::Serial; }
  bool isEnded() const { return Ended; }

  /// Copy \p Size bytes from \p Src at \p SrcOffset to \p Dst at
  /// \p DstOffset.
  virtual llvm::Error copyBufferToBuffer(Buffer &Src, size_t SrcOffset,
                                         Buffer &Dst, size_t DstOffset,
                                         size_t Size) = 0;

  /// Fill \p Size bytes of \p Dst starting at \p Offset with \p Value.
  /// \p Offset and \p Size must be multiples of 4.
  virtual llvm::Error fillBuffer(Buffer &Dst, size_t Offset, size_t Size,
                                 uint8_t Value) = 0;

  /// Begin a named debug group. Visible in GPU debuggers (PIX, RenderDoc,
  /// Xcode). Must be balanced by a corresponding popDebugGroup() call.
  virtual void pushDebugGroup(llvm::StringRef Label) {}

  /// End the most recently pushed debug group.
  virtual void popDebugGroup() {}

  /// Insert a point-in-time debug marker.
  virtual void insertDebugSignpost(llvm::StringRef Label) {}

  /// Finish recording. No further commands may be recorded after this call.
  /// Idempotent: safe to call more than once. If not called explicitly, the
  /// most-derived destructor invokes it as a safeguard against leaked open
  /// encoders.
  void endEncoding() {
    if (Ended)
      return;
    endEncodingImpl();
    Ended = true;
  }
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

  /// Dispatch a compute grid indirectly. The buffer at \p Offset must contain
  /// three uint32_t values: {GroupCountX, GroupCountY, GroupCountZ}.
  /// ThreadsPerGroup specifies the workgroup dimensions (required by some
  /// backends; others derive it from the pipeline state).
  virtual llvm::Error dispatchIndirect(Buffer &ArgBuffer, size_t Offset,
                                       uint32_t ThreadsPerGroupX,
                                       uint32_t ThreadsPerGroupY,
                                       uint32_t ThreadsPerGroupZ) = 0;

  /// Insert a memory barrier covering all outstanding scopes.
  virtual void barrier() = 0;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENCODER_H
