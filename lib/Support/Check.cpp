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
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

static bool isDenorm(float F) { return std::fpclassify(F) == FP_SUBNORMAL; }

static bool isnanFloat16(uint16_t val) {
  return (val & 0x7c00) == 0x7c00 && (val & 0x03ff) != 0;
}

static bool compareFloatULP(const float &FSrc, const float &FRef,
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
  int Diff = *((const uint32_t *)&FSrc) - *((const uint32_t *)&FRef);
  unsigned int AbsDiff = Diff < 0 ? -Diff : Diff;
  return AbsDiff <= ULPTolerance;
}

static bool compareHalfULP(const uint16_t &FSrc, const uint16_t &FRef,
                           unsigned ULPTolerance) {
  if (FSrc == FRef)
    return true;
  if (isnanFloat16(FSrc))
    return isnanFloat16(FRef);
  // 16-bit floating point numbers must preserve denorms
  int Diff = FSrc - FRef;
  unsigned int AbsDiff = Diff < 0 ? -Diff : Diff;
  return AbsDiff <= ULPTolerance;
}

static bool testBufferExact(offloadtest::Buffer *B1, offloadtest::Buffer *B2) {
  if (B1->size() != B2->size())
    return false;
  for (uint32_t I = 0; I < B1->size(); ++I) {
    if (B1->Data[I] != B2->Data[I])
      return false;
  }
  return true;
}

static bool testBufferFuzzy(offloadtest::Buffer *B1, offloadtest::Buffer *B2,
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
  case offloadtest::DataFormat::Float16: {
    if (B1->Size != B2->Size)
      return false;
    llvm::MutableArrayRef<uint16_t> Arr1(
        reinterpret_cast<uint16_t *>(B1->Data.get()),
        B1->Size / sizeof(uint16_t));
    assert(B2->Format == offloadtest::DataFormat::Float16 &&
           "Buffer types must be the same");
    llvm::MutableArrayRef<uint16_t> Arr2(
        reinterpret_cast<uint16_t *>(B2->Data.get()),
        B2->Size / sizeof(uint16_t));
    for (unsigned I = 0; I < Arr1.size(); ++I) {
      if (!compareHalfULP(Arr1[I], Arr2[I], ULPT))
        return false;
    }
    return true;
  }
  default:
    llvm_unreachable("Only float types are supported by the fuzzy test.");
  }
  return false;
}

llvm::Error verifyResult(offloadtest::Result R) {
  switch (R.Rule) {
  case offloadtest::Rule::BufferExact: {
    if (testBufferExact(R.ActualPtr, R.ExpectedPtr))
      return llvm::Error::success();
    llvm::SmallString<256> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "Test failed: " << R.Name << "\nExpected:\n";
    llvm::yaml::Output YAMLOS(OS);
    YAMLOS << *R.ExpectedPtr;
    OS << "Got:\n";
    YAMLOS << *R.ActualPtr;
    return llvm::createStringError(Str.c_str());
  }
  case offloadtest::Rule::BufferFuzzy: {
    if (testBufferFuzzy(R.ActualPtr, R.ExpectedPtr, R.ULPT, R.DM))
      return llvm::Error::success();
    llvm::SmallString<256> Str;
    llvm::raw_svector_ostream OS(Str);
    OS << "Test failed: " << R.Name << "\nExpected:\n";
    llvm::yaml::Output YAMLOS(OS);
    YAMLOS << *R.ExpectedPtr;
    OS << "Got:\n";
    YAMLOS << *R.ActualPtr;
    return llvm::createStringError(Str.c_str());
  }
  }
}
