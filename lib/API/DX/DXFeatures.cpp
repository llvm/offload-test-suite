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
#include "llvm/Support/ScopedPrinter.h"

using namespace offloadtest;
using namespace offloadtest::directx;
using namespace llvm;

#define SHADER_MODEL_ENUM(NewCase, Str, Value) {#Str, NewCase},
static const EnumEntry<directx::ShaderModel> ShaderModelNames[]{
#include "DXFeatures.def"
};

static ArrayRef<EnumEntry<ShaderModel>> getShaderModels() {
  return ArrayRef(ShaderModelNames);
}

#define ROOT_SIGNATURE_ENUM(NewCase, Str, Value) {#Str, NewCase},
static const EnumEntry<directx::RootSignature> RootSignatureNames[]{
#include "DXFeatures.def"
};

static ArrayRef<EnumEntry<RootSignature>> getRootSignatures() {
  return ArrayRef(RootSignatureNames);
}

template <typename T>
static std::string enumEntryToString(ArrayRef<EnumEntry<T>> EnumValues,
                                     T Value) {
  for (const EnumEntry<T> &I : EnumValues)
    if (I.Value == Value)
      return I.Name.str();
  llvm_unreachable("All cases must be covered");
}

std::string CapabilityPrinter<directx::ShaderModel>::toString(
    const directx::ShaderModel &V) {
  return enumEntryToString(getShaderModels(), V);
}

std::string CapabilityPrinter<directx::RootSignature>::toString(
    const directx::RootSignature &V) {
  return enumEntryToString(getRootSignatures(), V);
}
