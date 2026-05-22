//===- RenderPass.h - Offload API Render Pass -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the RenderPass abstract base class. A RenderPass describes the
// formats and load / store actions of one or more attachments. It carries no
// reference to specific textures: those are bound at encoder creation time.
// Backends that have a corresponding native object (Vulkan's VkRenderPass)
// build it once at creation time so it can be reused across encoders.
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_RENDERPASS_H
#define OFFLOADTEST_API_RENDERPASS_H

#include "API/API.h"
#include "API/Enums.h"
#include "API/Resources.h"

#include "llvm/ADT/SmallVector.h"

#include <optional>

namespace offloadtest {

struct ColorAttachmentFormatDesc {
  Format Fmt;
  LoadAction Load = LoadAction::Clear;
  StoreAction Store = StoreAction::Store;
};

struct DepthStencilAttachmentFormatDesc {
  Format Fmt;
  LoadAction DepthLoad = LoadAction::Clear;
  StoreAction DepthStore = StoreAction::Store;
  LoadAction StencilLoad = LoadAction::DontCare;
  StoreAction StencilStore = StoreAction::DontCare;
};

struct RenderPassDesc {
  llvm::SmallVector<ColorAttachmentFormatDesc, 8> ColorAttachments;
  std::optional<DepthStencilAttachmentFormatDesc> DepthStencil;
};

class RenderPass {
  GPUAPI API;

public:
  virtual ~RenderPass();
  RenderPass(const RenderPass &) = delete;
  RenderPass &operator=(const RenderPass &) = delete;

  GPUAPI getAPI() const { return API; }

protected:
  explicit RenderPass(GPUAPI API) : API(API) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_RENDERPASS_H
