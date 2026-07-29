//===- Sampler.h - Offload API Sampler ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_SAMPLER_H
#define OFFLOADTEST_API_SAMPLER_H

#include "API/API.h"
#include "API/Resources.h"

namespace offloadtest {

enum class FilterMode { Nearest, Linear };

enum class AddressMode { Clamp, Repeat, Mirror, Border, MirrorOnce };

enum class CompareFunction {
  Never,
  Less,
  Equal,
  LessEqual,
  Greater,
  NotEqual,
  GreaterEqual,
  Always
};

enum class SamplerKind { Sampler, SamplerComparison };

struct SamplerCreateDesc {
  FilterMode MinFilter = FilterMode::Linear;
  FilterMode MagFilter = FilterMode::Linear;
  AddressMode Address = AddressMode::Clamp;
  float MinLOD = 0.0f;
  float MaxLOD = std::numeric_limits<float>::max();
  float MipLODBias = 0.0f;
  CompareFunction ComparisonOp = CompareFunction::Never;
  SamplerKind Kind = SamplerKind::Sampler;
};

class Sampler {
  GPUAPI API;

public:
  virtual ~Sampler();
  Sampler(const Sampler &) = delete;
  Sampler &operator=(const Sampler &) = delete;

  GPUAPI getAPI() const { return API; }
  virtual const SamplerCreateDesc &getDesc() const = 0;

protected:
  explicit Sampler(GPUAPI API) : API(API) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_SAMPLER_H
