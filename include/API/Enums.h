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

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENUMS_H
