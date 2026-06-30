//===- DX/Sampler.cpp - DirectX Sampler API -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/DX/Sampler.h"

using namespace offloadtest;

DXSampler::DXSampler(llvm::StringRef Name, SamplerCreateDesc Desc,
                     D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    : offloadtest::Sampler(GPUAPI::DirectX), Handle(Handle), Name(Name),
      Desc(Desc) {}

const SamplerCreateDesc &DXSampler::getDesc() const { return Desc; }

bool DXSampler::classof(const offloadtest::Sampler *S) {
  return S->getAPI() == GPUAPI::DirectX;
}
