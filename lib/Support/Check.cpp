//===- Check.cpp - Check test results  ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Support/Check.h"

inline bool isDenorm(float F) {
  return (std::numeric_limits<float>::denorm_min() <= F &&
          F < std::numeric_limits<float>::min()) ||
         (-std::numeric_limits<float>::min() < F &&
          F <= -std::numeric_limits<float>::denorm_min());
}

bool compareFloatULP(const float &FSrc, const float &FRef,
                     unsigned ULPTolerance, offloadtest::DenormMode DM) {
  if (FSrc == FRef)
    return true;
  if (std::isnan(FSrc))
    return std::isnan(FRef);
  if (DM == offloadtest::DenormMode::Any) {
    // If denorm expected, output can be sign preserved zero. Otherwise output
    // should pass the regular ulp testing.
    if (isDenorm(FRef) && FSrc == 0 && std::signbit(FSrc) == std::signbit(FRef))
      return true;
  }
  // For FTZ or Preserve mode, we should get the expected number within
  // ULPTolerance for any operations.
  int diff = *((const uint32_t *)&FSrc) - *((const uint32_t *)&FRef);
  unsigned int uDiff = diff < 0 ? -diff : diff;
  return uDiff <= ULPTolerance;
}

bool getResult(offloadtest::Result R) {
  switch (R.Rule) {
  case offloadtest::Rule::BufferExact: {
    return testBufferExact(R.ActualPtr, R.ExpectedPtr);
  }
  case offloadtest::Rule::BufferFuzzy: {
    return testBufferFuzzy(R.ActualPtr, R.ExpectedPtr, R.ULPT, R.DM);
  }
  }
}

bool testBufferExact(offloadtest::Buffer *B1, offloadtest::Buffer *B2) {
  if (B1->size() != B2->size())
    return false;
  for (uint32_t I = 0; I < B1->size(); ++I) {
    if (B1->Data[I] != B2->Data[I])
      return false;
  }
  return true;
}

bool testBufferFuzzy(offloadtest::Buffer *B1, offloadtest::Buffer *B2,
                     unsigned ULPT, offloadtest::DenormMode DM) {
  switch (B1->Format) {
  case offloadtest::DataFormat::Float32: {
    if (B1->Size != B2->Size)
      return false;
    llvm::MutableArrayRef<float> Arr1(reinterpret_cast<float *>(B1->Data.get()),
                                      B1->Size / sizeof(float));
    assert(B2->Format == offloadtest::DataFormat::Float32 &&
           "Buffer types must be the same");
    llvm::MutableArrayRef<float> Arr2(reinterpret_cast<float *>(B2->Data.get()),
                                      B2->Size / sizeof(float));
    for (unsigned I = 0; I < Arr1.size(); ++I) {
      if (!compareFloatULP(Arr1[I], Arr2[I], ULPT, DM))
        return false;
    }
    return true;
  }
  default:
    llvm_unreachable("Only float types are supported by the fuzzy test.");
  }
  return false;
  // Call the appropriate function based on the type
}
