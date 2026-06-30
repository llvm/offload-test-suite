//===- DX/RenderPass.h - Offload API DX Render Pass API -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_RENDERPASS_H
#define OFFLOADTEST_API_DX_RENDERPASS_H

#include "API/RenderPass.h"

namespace offloadtest {

class DXRenderPass final : public offloadtest::RenderPass {
public:
  offloadtest::RenderPassDesc Desc;

  explicit DXRenderPass(offloadtest::RenderPassDesc Desc)
      : RenderPass(GPUAPI::DirectX), Desc(std::move(Desc)) {}

  static bool classof(const offloadtest::RenderPass *RP) {
    return RP->getAPI() == GPUAPI::DirectX;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_RENDERPASS_H
