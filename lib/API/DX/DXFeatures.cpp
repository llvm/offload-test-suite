//===- DX/DXFeatures.cpp - DirectX Device API -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "DXFeatures.h"
#include "llvm/ADT/Enum.h"

using namespace offloadtest;
using namespace offloadtest::directx;
using namespace llvm;

#define SHADER_MODEL_ENUM(NewCase, Str, Value) {{#Str}, NewCase},
static constexpr EnumStringDef<directx::ShaderModel> ShaderModelDefs[]{
#include "DXFeatures.def"
};
static constexpr auto ShaderModelNames = BUILD_ENUM_STRINGS(ShaderModelDefs);

#define ROOT_SIGNATURE_ENUM(NewCase, Str, Value) {{#Str}, NewCase},
static constexpr EnumStringDef<directx::RootSignature> RootSignatureDefs[]{
#include "DXFeatures.def"
};
static constexpr auto RootSignatureNames =
    BUILD_ENUM_STRINGS(RootSignatureDefs);

#define MESH_SHADER_TIER_ENUM(NewCase, Str, Value) {{#Str}, NewCase},
static constexpr EnumStringDef<directx::MeshShaderTier> MeshShaderTierDefs[]{
#include "DXFeatures.def"
};
static constexpr auto MeshShaderTierNames =
    BUILD_ENUM_STRINGS(MeshShaderTierDefs);

#define RAYTRACING_TIER_ENUM(NewCase, Str, Value) {{#Str}, NewCase},
static constexpr EnumStringDef<directx::RaytracingTier> RaytracingTierDefs[]{
#include "DXFeatures.def"
};
static constexpr auto RaytracingTierNames =
    BUILD_ENUM_STRINGS(RaytracingTierDefs);

std::string CapabilityPrinter<directx::ShaderModel>::toString(
    const directx::ShaderModel &V) {
  return EnumStrings(ShaderModelNames).toString(V).str();
}

std::string CapabilityPrinter<directx::RootSignature>::toString(
    const directx::RootSignature &V) {
  return EnumStrings(RootSignatureNames).toString(V).str();
}

std::string CapabilityPrinter<directx::MeshShaderTier>::toString(
    const directx::MeshShaderTier &V) {
  return EnumStrings(MeshShaderTierNames).toString(V).str();
}

std::string CapabilityPrinter<directx::RaytracingTier>::toString(
    const directx::RaytracingTier &V) {
  return EnumStrings(RaytracingTierNames).toString(V).str();
}