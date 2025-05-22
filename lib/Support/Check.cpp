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

template <typename T> static bool isDenorm(T F) {
  return std::fpclassify(F) == FP_SUBNORMAL;
}

static bool isFloat16NAN(uint16_t Val) {
  return (Val & 0x7c00) == 0x7c00 && (Val & 0x03ff) != 0;
}

static bool compareDoubleULP(const double &FSrc, const double &FRef,
                             unsigned ULPTolerance,
                             offloadtest::DenormMode DM) {
  if (FSrc == FRef)
    return true;
  if (std::isnan(FSrc) || std::isnan(FRef))
    return std::isnan(FRef) && std::isnan(FSrc);
  if (DM == offloadtest::DenormMode::Any) {
    // If denorm expected, output can be sign preserved zero. Otherwise output
    // should pass the regular ulp testing.
    if (isDenorm(FRef) && FSrc == 0 && std::signbit(FSrc) == std::signbit(FRef))
      return true;
  }
  // For FTZ or Preserve mode, we should get the expected number within
  // ULPTolerance for any operations.
  const int64_t Diff = *((const uint64_t *)&FSrc) - *((const uint64_t *)&FRef);
  const uint64_t AbsDiff = Diff < 0 ? -Diff : Diff;
  return AbsDiff <= ULPTolerance;
}

static bool compareFloatULP(const float &FSrc, const float &FRef,
                            unsigned ULPTolerance, offloadtest::DenormMode DM) {
  if (FSrc == FRef)
    return true;
  if (std::isnan(FSrc) || std::isnan(FRef))
    return std::isnan(FRef) && std::isnan(FSrc);
  if (DM == offloadtest::DenormMode::Any) {
    // If denorm expected, output can be sign preserved zero. Otherwise output
    // should pass the regular ulp testing.
    if (isDenorm(FRef) && FSrc == 0 && std::signbit(FSrc) == std::signbit(FRef))
      return true;
  }
  // For FTZ or Preserve mode, we should get the expected number within
  // ULPTolerance for any operations.
  const int Diff = *((const uint32_t *)&FSrc) - *((const uint32_t *)&FRef);
  const unsigned int AbsDiff = Diff < 0 ? -Diff : Diff;
  return AbsDiff <= ULPTolerance;
}

static bool compareFloat16ULP(const uint16_t &FSrc, const uint16_t &FRef,
                              unsigned ULPTolerance) {
  if (FSrc == FRef)
    return true;
  if (isFloat16NAN(FSrc) || isFloat16NAN(FRef))
    return isFloat16NAN(FRef) && isFloat16NAN(FSrc);
  // 16-bit floating point numbers must preserve denorms
  const int Diff = FSrc - FRef;
  const unsigned int AbsDiff = Diff < 0 ? -Diff : Diff;
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

template <typename T>
static bool testAll(std::function<bool(const T &, const T &)> fn,
                    llvm::ArrayRef<T> Arr1, llvm::ArrayRef<T> Arr2) {
  if (Arr1.size() != Arr2.size())
    return false;

  for (unsigned I = 0, E = Arr1.size(); I < E; ++I) {
    if (!fn(Arr1[I], Arr2[I]))
      return false;
  }
  return true;
}

static bool testBufferFuzzy(offloadtest::Buffer *B1, offloadtest::Buffer *B2,
                            unsigned ULPT, offloadtest::DenormMode DM) {
  assert(B1->Format == B2->Format && "Buffer types must be the same");
  switch (B1->Format) {
  case offloadtest::DataFormat::Float64: {
    const llvm::ArrayRef<double> Arr1(
        reinterpret_cast<double *>(B1->Data.get()), B1->Size / sizeof(double));
    const llvm::ArrayRef<double> Arr2(
        reinterpret_cast<double *>(B2->Data.get()), B2->Size / sizeof(double));
    auto fn = [ULPT, DM](const double &FS, const double &FR) {
      return compareDoubleULP(FS, FR, ULPT, DM);
    };
    return testAll<double>(fn, Arr1, Arr2);
  }
  case offloadtest::DataFormat::Float32: {
    const llvm::ArrayRef<float> Arr1(reinterpret_cast<float *>(B1->Data.get()),
                                     B1->Size / sizeof(float));
    const llvm::ArrayRef<float> Arr2(reinterpret_cast<float *>(B2->Data.get()),
                                     B2->Size / sizeof(float));
    auto fn = [ULPT, DM](const float &FS, const float &FR) {
      return compareFloatULP(FS, FR, ULPT, DM);
    };
    return testAll<float>(fn, Arr1, Arr2);
  }
  case offloadtest::DataFormat::Float16: {
    const llvm::ArrayRef<uint16_t> Arr1(
        reinterpret_cast<uint16_t *>(B1->Data.get()),
        B1->Size / sizeof(uint16_t));
    const llvm::ArrayRef<uint16_t> Arr2(
        reinterpret_cast<uint16_t *>(B2->Data.get()),
        B2->Size / sizeof(uint16_t));
    auto fn = [ULPT](const uint16_t &FS, const uint16_t &FR) {
      return compareFloat16ULP(FS, FR, ULPT);
    };
    return testAll<uint16_t>(fn, Arr1, Arr2);
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
