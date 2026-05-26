//===- Enums.h - Offload API Device API -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_ENUMS_H
#define OFFLOADTEST_API_ENUMS_H

namespace offloadtest {

enum class ResourceKind {
  Buffer,
  StructuredBuffer,
  ByteAddressBuffer,
  Texture2D,
  RWBuffer,
  RWStructuredBuffer,
  RWByteAddressBuffer,
  RWTexture2D,
  ConstantBuffer,
  Sampler,
  SampledTexture2D,
};

enum ShaderContainerType {
  DXIL,
  SPIRV,
  Metal,
};

/// Action applied to an attachment when a render pass begins.
enum class LoadAction {
  Load,     ///< Preserve existing contents.
  Clear,    ///< Clear to the texture's OptimizedClearValue at encoder time.
  DontCare, ///< Contents are undefined; the driver may discard.
};

/// Action applied to an attachment when a render pass ends.
enum class StoreAction {
  Store,    ///< Write the rendered contents back to memory.
  DontCare, ///< Contents may be discarded after the pass.
};

enum class PrimitiveTopology { TriangleList, PointList };

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENUMS_H
