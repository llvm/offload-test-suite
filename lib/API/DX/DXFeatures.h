//===- DXFeatures.h - DirectX Features ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DXFEATURES_H
#define OFFLOADTEST_API_DXFEATURES_H

#include "API/Capabilities.h"

namespace offloadtest {
namespace directx {

#define SHADER_MODEL_ENUM(NewCase, Str, Value) NewCase = Value,
enum ShaderModel {
#include "DXFeatures.def"
};

#define ROOT_SIGNATURE_ENUM(NewCase, Str, Value) NewCase = Value,
enum RootSignature {
#include "DXFeatures.def"
};

#define MESH_SHADER_TIER_ENUM(NewCase, Str, Value) NewCase = Value,
enum MeshShaderTier {
#include "DXFeatures.def"
};

#define RAYTRACING_TIER_ENUM(NewCase, Str, Value) NewCase = Value,
enum RaytracingTier {
#include "DXFeatures.def"
};

} // namespace directx

template <> struct CapabilityPrinter<directx::ShaderModel> {
  static std::string toString(const directx::ShaderModel &V);
};

template <> struct CapabilityPrinter<directx::RootSignature> {
  static std::string toString(const directx::RootSignature &V);
};

template <> struct CapabilityPrinter<directx::MeshShaderTier> {
  static std::string toString(const directx::MeshShaderTier &V);
};

template <> struct CapabilityPrinter<directx::RaytracingTier> {
  static std::string toString(const directx::RaytracingTier &V);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DXFEATURES_H
