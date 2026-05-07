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
class PipelineState;

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

  /// Copy \p Size bytes from \p Src at \p SrcOffset to \p Dst at
  /// \p DstOffset.
  virtual llvm::Error copyBufferToBuffer(Buffer &Src, size_t SrcOffset,
                                         Buffer &Dst, size_t DstOffset,
                                         size_t Size) = 0;

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
  /// launch in each dimension. The workgroup size is derived from the bound
  /// pipeline state (e.g. the shader's numthreads attribute).
  virtual llvm::Error dispatch(uint32_t GroupCountX, uint32_t GroupCountY,
                               uint32_t GroupCountZ) = 0;
};

/// Primitive topology used at draw time. The bound pipeline's topology
/// category (point / line / triangle) must match the value passed to draw().
enum class PrimitiveTopology {
  TriangleList,
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

/// Encoder for recording rasterization draw commands within a render pass.
/// Created via CommandBuffer::createRenderEncoder(const RenderPassDesc &).
///
/// State model: every encoder starts with no viewport, scissor, or vertex
/// buffer bindings. The caller is required to set viewport and scissor (and
/// any vertex buffers needed by the pipeline) before the first draw — there
/// is no carry-over from a previous pass. The pipeline state is passed
/// directly to draw() so a draw cannot be issued with no pipeline bound or
/// a stale pipeline from another pass.
class RenderEncoder : public CommandEncoder {
public:
  using CommandEncoder::CommandEncoder;

  virtual void setViewport(const Viewport &VP) = 0;
  virtual void setScissor(const ScissorRect &Rect) = 0;

  /// Bind a vertex buffer at \p Slot with the given per-vertex \p Stride in
  /// bytes. \p Stride is required at set time (DX12 carries it on the buffer
  /// view, not the PSO). Pass nullptr to unbind; \p Stride is ignored when
  /// unbinding.
  virtual void setVertexBuffer(uint32_t Slot, Buffer *VB, size_t Offset,
                               uint32_t Stride) = 0;

  /// Non-indexed instanced draw. Mirrors DX12 DrawInstanced / vkCmdDraw /
  /// MTL drawPrimitives. The pipeline is bound as part of the call.
  /// Resource bindings (root descriptor tables, descriptor sets, argument
  /// buffers) must be set up on the underlying command buffer before draw —
  /// the abstraction for those is still WIP.
  virtual llvm::Error
  drawInstanced(const PipelineState &PSO, PrimitiveTopology Topology,
                uint32_t VertexCount, uint32_t InstanceCount,
                uint32_t FirstVertex = 0, uint32_t FirstInstance = 0) = 0;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENCODER_H
