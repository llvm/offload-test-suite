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

/// Per-draw rasterizer shading rate (D3D12 VRS Tier 1). Tier 1 hardware
/// supports the four base rates (1x1, 1x2, 2x1, 2x2); the additional rates
/// (2x4, 4x2, 4x4) require AdditionalShadingRatesSupported.
enum class ShadingRate {
  Rate_1x1,
  Rate_1x2,
  Rate_2x1,
  Rate_2x2,
  Rate_2x4,
  Rate_4x2,
  Rate_4x4,
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENUMS_H
