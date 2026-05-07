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

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <memory>

namespace offloadtest {

class RenderPass;
class Texture;

/// Description used to begin a render pass for one render encoder. The
/// RenderPass itself carries the format / load / store policy and is
/// expected to be created up-front via Device::createRenderPass; this
/// struct binds it to the actual textures for a single encoder's lifetime.
/// The texture count and order must match the RenderPass's attachment
/// layout: one texture per color attachment, plus the depth-stencil
/// texture iff the RenderPass has one.
struct RenderPassBeginDesc {
  RenderPass *Pass = nullptr;
  llvm::SmallVector<Texture *, 8> ColorAttachments;
  Texture *DepthStencil = nullptr;
};

class CommandBuffer {
  GPUAPI Kind;

public:
  explicit CommandBuffer(GPUAPI Kind) : Kind(Kind) {}
  virtual ~CommandBuffer();
  CommandBuffer(const CommandBuffer &) = delete;
  CommandBuffer &operator=(const CommandBuffer &) = delete;

  GPUAPI getKind() const { return Kind; }

  /// Create a compute command encoder for recording dispatch commands.
  /// Barriers are automatically inserted between commands.
  virtual llvm::Expected<std::unique_ptr<ComputeEncoder>>
  createComputeEncoder() {
    return llvm::createStringError(
        std::errc::not_supported,
        "createComputeEncoder not implemented for this backend");
  }

  /// Create a render command encoder for recording draw commands. The
  /// returned encoder begins the render pass referenced by \p Desc.Pass
  /// against the textures in \p Desc; the pass ends when the encoder's
  /// endEncoding() runs (explicitly or from its destructor).
  virtual llvm::Expected<std::unique_ptr<RenderEncoder>>
  createRenderEncoder(const RenderPassBeginDesc &Desc) {
    (void)Desc;
    return llvm::createStringError(
        std::errc::not_supported,
        "createRenderEncoder not implemented for this backend");
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_COMMANDBUFFER_H
