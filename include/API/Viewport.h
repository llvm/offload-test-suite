//===- Viewport.h - Viewport and scissor descriptions ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VIEWPORT_H
#define OFFLOADTEST_API_VIEWPORT_H

#include <cstdint>

namespace offloadtest {

// D3D12's limit; Vulkan multiViewport and Metal 2 support at least 16.
constexpr uint32_t MaxViewports = 16;

struct Viewport {
  float X = 0.0f, Y = 0.0f;
  float Width = 0.0f, Height = 0.0f;
  float MinDepth = 0.0f, MaxDepth = 1.0f;
};

struct ScissorRect {
  int32_t X = 0, Y = 0;
  uint32_t Width = 0, Height = 0;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VIEWPORT_H
