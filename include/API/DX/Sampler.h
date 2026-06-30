//===- DX/Sampler.h - Offload API DX Sampler API --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_SAMPLER_H
#define OFFLOADTEST_API_DX_SAMPLER_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/Sampler.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXSampler : public offloadtest::Sampler {
public:
  D3D12_CPU_DESCRIPTOR_HANDLE Handle = {};
  std::string Name;
  SamplerCreateDesc Desc;

  DXSampler(llvm::StringRef Name, SamplerCreateDesc Desc,
            D3D12_CPU_DESCRIPTOR_HANDLE Handle);

  const SamplerCreateDesc &getDesc() const override;

  static bool classof(const offloadtest::Sampler *S);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_SAMPLER_H