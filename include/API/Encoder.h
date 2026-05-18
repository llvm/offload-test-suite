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

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstddef>
#include <cstdint>

namespace offloadtest {

class Buffer;
class Texture;
class PipelineState;
class Texture;
class AccelerationStructure;
struct BLASBuildRequest;
struct TLASBuildRequest;

/// Tagged-pointer alias for an acceleration-structure build request. Each
/// request carries its own target AS (`Req->AS`), so a batch is just an
/// array of these pointers. The caller is responsible for ensuring no item
/// in a batch has a memory dependency on another (e.g. a TLAS that reads a
/// BLAS being built in the same batch must be in a separate batch — that
/// barrier is inserted between batchBuildAS() calls automatically).
using ASBuildItem =
    llvm::PointerUnion<const BLASBuildRequest *, const TLASBuildRequest *>;

/// Base class for all command encoders. An encoder records commands into a
/// command buffer. Call endEncoding() when done recording. Barriers are
/// automatically inserted between commands.
class CommandEncoder {
  GPUAPI API;
  bool Ended = false;

protected:
  /// Backend-specific cleanup. Called exactly once, either explicitly via
  /// endEncoding() or implicitly from the most-derived destructor.
  virtual void endEncodingImpl() = 0;

public:
  explicit CommandEncoder(GPUAPI API) : API(API) {}
  virtual ~CommandEncoder();
  CommandEncoder(const CommandEncoder &) = delete;
  CommandEncoder &operator=(const CommandEncoder &) = delete;

  GPUAPI getAPI() const { return API; }
  bool isEnded() const { return Ended; }

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

  /// Dispatch a compute grid. GroupCount specifies how many workgroups to
  /// launch in each dimension. The workgroup size is derived from \p PSO
  /// (e.g. the shader's numthreads attribute), which is also bound for the
  /// dispatch.
  virtual llvm::Error dispatch(const PipelineState &PSO, uint32_t GroupCountX,
                               uint32_t GroupCountY, uint32_t GroupCountZ) = 0;

  /// Copy \p Size bytes from \p Src at \p SrcOffset to \p Dst at
  /// \p DstOffset.
  virtual llvm::Error copyBufferToBuffer(Buffer &Src, size_t SrcOffset,
                                         Buffer &Dst, size_t DstOffset,
                                         size_t Size) = 0;

  /// Copy a buffer into a texture. The caller is expected to set up correct
  /// striding using the stride acquired from
  /// `Device::getTextureUploadRowStrideInBytes`.
  virtual llvm::Error copyBufferToTexture(Buffer &Src, Texture &Dst) = 0;

  virtual llvm::Error copyCounterToBuffer(Buffer &Src, Buffer &Dst) = 0;

  virtual llvm::Error copyTextureToBuffer(Texture &Src, Buffer &Dst) = 0;

  /// Build a batch of acceleration structures in a single barrier slot. All
  /// items in `Items` must be independent — no item may depend on another's
  /// build output. Backends may issue this as one native batch call (Vulkan)
  /// or as a sequence of single-AS calls without intermediate barriers (DX12,
  /// Metal). A barrier covering AS-build writes is implicitly emitted before
  /// any subsequent command that reads from the freshly-built structures.
  virtual llvm::Error batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) = 0;
};

struct Viewport {
  float X = 0.0f, Y = 0.0f;
  float Width = 0.0f, Height = 0.0f;
  float MinDepth = 0.0f, MaxDepth = 1.0f;
};

struct ScissorRect {
  int32_t X = 0, Y = 0;
  uint32_t Width = 0, Height = 0;
};

class RenderEncoder : public CommandEncoder {
public:
  using CommandEncoder::CommandEncoder;

  virtual void setViewport(const Viewport &VP) = 0;
  virtual void setScissor(const ScissorRect &Rect) = 0;

  virtual void setVertexBuffer(uint32_t Slot, Buffer *VB, size_t Offset,
                               uint32_t Stride) = 0;

  virtual llvm::Error drawInstanced(const PipelineState &PSO,
                                    uint32_t VertexCount,
                                    uint32_t InstanceCount,
                                    uint32_t FirstVertex = 0,
                                    uint32_t FirstInstance = 0) = 0;

  virtual llvm::Error dispatchMesh(const PipelineState &PSO,
                                   uint32_t GroupCountX, uint32_t GroupCountY,
                                   uint32_t GroupCountZ) = 0;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENCODER_H
