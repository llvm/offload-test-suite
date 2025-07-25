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
#include "Support/Pipeline.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"
#include <cmath>
#include <sstream>

constexpr uint16_t Float16BitSign = 0x8000;
constexpr uint16_t Float16BitExp = 0x7c00;
constexpr uint16_t Float16BitMantissa = 0x03ff;

static float convertFloat16ToFloat(const uint16_t F) {
  const llvm::APInt API(16, F);
  llvm::detail::IEEEFloat IEF(llvm::APFloat::IEEEhalf(), API);
  bool LostInfo;
  // rounding mode should not matter since we are up converting
  IEF.convert(llvm::APFloat::IEEEsingle(),
              llvm::APFloatBase::rmNearestTiesToEven, &LostInfo);
  return IEF.convertToFloat();
}

// limited to float, double, and long double
template <typename T> static bool isDenorm(T F) {
  return std::fpclassify(F) == FP_SUBNORMAL;
}

static bool isFloat16NAN(uint16_t Val) {
  return (Val & Float16BitExp) == Float16BitExp &&
         (Val & Float16BitMantissa) != 0;
}

static bool compareDoubleEpsilon(const double &FSrc, const double &FRef,
                                 double Epsilon, offloadtest::DenormMode DM) {
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
  // epsilon for any operation
  return std::abs(FSrc - FRef) < Epsilon;
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

static bool compareFloatEpsilon(const float &FSrc, const float &FRef,
                                float Epsilon, offloadtest::DenormMode DM) {
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
  // epsilon for any operation
  return std::abs(FSrc - FRef) < Epsilon;
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

static bool compareFloat16Epsilon(const uint16_t &FSrc, const uint16_t &FRef,
                                  float Epsilon) {
  // Treat +0 and -0 as equal
  if ((FSrc & ~Float16BitSign) == 0 && (FRef & ~Float16BitSign) == 0)
    return true;
  if (FSrc == FRef)
    return true;
  if (isFloat16NAN(FSrc) || isFloat16NAN(FRef))
    return isFloat16NAN(FRef) && isFloat16NAN(FSrc);

  const float FSrc32 = convertFloat16ToFloat(FSrc);
  const float FRef32 = convertFloat16ToFloat(FRef);
  return std::abs(FSrc32 - FRef32) < Epsilon;
}

static bool compareFloat16ULP(const uint16_t &FSrc, const uint16_t &FRef,
                              unsigned ULPTolerance) {
  // Treat +0 and -0 as equal
  if ((FSrc & ~Float16BitSign) == 0 && (FRef & ~Float16BitSign) == 0)
    return true;
  if (FSrc == FRef)
    return true;
  if (isFloat16NAN(FSrc) || isFloat16NAN(FRef))
    return isFloat16NAN(FRef) && isFloat16NAN(FSrc);

  // Map to monotonic ordering for correct ULP diff
  auto ToOrdered = [](uint16_t H) -> int {
    return (H & Float16BitSign) ? (~H & 0xFFFF) : (H | Float16BitSign);
  };

  // 16-bit floating point numbers must preserve denorms
  const int IntFSrc = ToOrdered(FSrc);
  const int IntFRef = ToOrdered(FRef);
  const int Diff = IntFSrc - IntFRef;
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
static bool testAll(std::function<bool(const T &, const T &)> ComparisonFn,
                    llvm::ArrayRef<T> Arr1, llvm::ArrayRef<T> Arr2) {
  if (Arr1.size() != Arr2.size())
    return false;

  for (size_t I = 0, E = Arr1.size(); I < E; ++I) {
    if (!ComparisonFn(Arr1[I], Arr2[I]))
      return false;
  }
  return true;
}

template <typename T>
static bool
testBufferFloat(std::function<bool(const T &, const T &)> ComparisonFn,
                offloadtest::Buffer *B1, offloadtest::Buffer *B2) {
  assert(B1->Format == B2->Format && "Buffer types must be the same");
  switch (B1->Format) {
  case offloadtest::DataFormat::Float64: {
    const llvm::ArrayRef<double> Arr1(
        reinterpret_cast<double *>(B1->Data.get()), B1->Size / sizeof(double));
    const llvm::ArrayRef<double> Arr2(
        reinterpret_cast<double *>(B2->Data.get()), B2->Size / sizeof(double));
    return testAll<double>(ComparisonFn, Arr1, Arr2);
  }
  case offloadtest::DataFormat::Float32: {
    const llvm::ArrayRef<float> Arr1(reinterpret_cast<float *>(B1->Data.get()),
                                     B1->Size / sizeof(float));
    const llvm::ArrayRef<float> Arr2(reinterpret_cast<float *>(B2->Data.get()),
                                     B2->Size / sizeof(float));
    return testAll<float>(ComparisonFn, Arr1, Arr2);
  }
  case offloadtest::DataFormat::Float16: {
    const llvm::ArrayRef<uint16_t> Arr1(
        reinterpret_cast<uint16_t *>(B1->Data.get()),
        B1->Size / sizeof(uint16_t));
    const llvm::ArrayRef<uint16_t> Arr2(
        reinterpret_cast<uint16_t *>(B2->Data.get()),
        B2->Size / sizeof(uint16_t));
    return testAll<uint16_t>(ComparisonFn, Arr1, Arr2);
  }
  default:
    llvm_unreachable("Only float types are supported by the fuzzy test.");
  }
  return false;
}

static bool testBufferFloatEpsilon(offloadtest::Buffer *B1,
                                   offloadtest::Buffer *B2, double Epsilon,
                                   offloadtest::DenormMode DM) {

  switch (B1->Format) {
  case offloadtest::DataFormat::Float64: {
    auto Fn = [Epsilon, DM](const double &FS, const double &FR) {
      return compareDoubleEpsilon(FS, FR, Epsilon, DM);
    };
    return testBufferFloat<double>(Fn, B1, B2);
  }
  case offloadtest::DataFormat::Float32: {
    auto Fn = [Epsilon, DM](const float &FS, const float &FR) {
      return compareFloatEpsilon(FS, FR, (float)Epsilon, DM);
    };
    return testBufferFloat<float>(Fn, B1, B2);
  }
  case offloadtest::DataFormat::Float16: {
    auto Fn = [Epsilon](const uint16_t &FS, const uint16_t &FR) {
      return compareFloat16Epsilon(FS, FR, (float)Epsilon);
    };
    return testBufferFloat<uint16_t>(Fn, B1, B2);
  }
  default:
    llvm_unreachable("Only float types are supported by the fuzzy test.");
  }
  return false;
}

static bool testBufferFloatULP(offloadtest::Buffer *B1, offloadtest::Buffer *B2,
                               unsigned ULPT, offloadtest::DenormMode DM) {

  switch (B1->Format) {
  case offloadtest::DataFormat::Float64: {
    auto Fn = [ULPT, DM](const double &FS, const double &FR) {
      return compareDoubleULP(FS, FR, ULPT, DM);
    };
    return testBufferFloat<double>(Fn, B1, B2);
  }
  case offloadtest::DataFormat::Float32: {
    auto Fn = [ULPT, DM](const float &FS, const float &FR) {
      return compareFloatULP(FS, FR, ULPT, DM);
    };
    return testBufferFloat<float>(Fn, B1, B2);
  }
  case offloadtest::DataFormat::Float16: {
    auto Fn = [ULPT](const uint16_t &FS, const uint16_t &FR) {
      return compareFloat16ULP(FS, FR, ULPT);
    };
    return testBufferFloat<uint16_t>(Fn, B1, B2);
  }
  default:
    llvm_unreachable("Only float types are supported by the fuzzy test.");
  }
  return false;
}

template <typename T>
static std::string bitPatternAsHex64(const T &Val,
                                     offloadtest::Rule ComparisonRule) {
  static_assert(sizeof(T) <= sizeof(uint64_t), "Type too large for Hex64");

  std::ostringstream Oss;
  if (ComparisonRule == offloadtest::Rule::BufferExact)
    Oss << std::hex << Val;
  else
    Oss << std::hexfloat << Val;
  return Oss.str();
}

template <typename T>
std::string formatBuffer(offloadtest::Buffer *B, offloadtest::Rule rule) {
  llvm::MutableArrayRef<T> arr(reinterpret_cast<T *>(B->Data.get()),
                               B->Size / sizeof(T));
  if (arr.empty())
    return "";

  std::string result = "[ " + bitPatternAsHex64(arr[0], rule);
  for (size_t i = 1; i < arr.size(); ++i)
    result += ", " + bitPatternAsHex64(arr[i], rule);
  result += " ]";
  return result;
}

static const std::string getBufferStr(offloadtest::Buffer *B,
                                      offloadtest::Rule Rule) {
  using DF = offloadtest::DataFormat;
  switch (B->Format) {
  case DF::Hex8:
    return formatBuffer<llvm::yaml::Hex8>(B, Rule);
  case DF::Hex16:
    return formatBuffer<llvm::yaml::Hex16>(B, Rule);
  case DF::Hex32:
    return formatBuffer<llvm::yaml::Hex32>(B, Rule);
  case DF::Hex64:
    return formatBuffer<llvm::yaml::Hex64>(B, Rule);
  case DF::UInt16:
    return formatBuffer<uint16_t>(B, Rule);
  case DF::UInt32:
    return formatBuffer<uint32_t>(B, Rule);
  case DF::UInt64:
    return formatBuffer<uint64_t>(B, Rule);
  case DF::Int16:
    return formatBuffer<int16_t>(B, Rule);
  case DF::Int32:
    return formatBuffer<int32_t>(B, Rule);
  case DF::Int64:
    return formatBuffer<int64_t>(B, Rule);
  case DF::Float16:
    return formatBuffer<llvm::yaml::Hex16>(B,
                                           Rule); // assuming no native float16
  case DF::Float32:
    return formatBuffer<float>(B, Rule);
  case DF::Float64:
    return formatBuffer<double>(B, Rule);
  case DF::Bool:
    return formatBuffer<uint32_t>(B,
                                  Rule); // Because sizeof(bool) is 1 but HLSL
                                         // represents a bool using 4 bytes.
  }
}

llvm::Error verifyResult(offloadtest::Result R) {
  llvm::SmallString<256> Str;
  llvm::raw_svector_ostream OS(Str);
  OS << "Test failed: " << R.Name << "\n";

  switch (R.ComparisonRule) {
  case offloadtest::Rule::BufferExact: {
    if (testBufferExact(R.ActualPtr, R.ExpectedPtr))
      return llvm::Error::success();
    OS << "Comparison Rule: BufferExact\n";
    break;
  }
  case offloadtest::Rule::BufferFloatULP: {
    if (testBufferFloatULP(R.ActualPtr, R.ExpectedPtr, R.ULPT, R.DM))
      return llvm::Error::success();
    OS << "Comparison Rule: BufferFloatULP\nULP: " << R.ULPT << "\n";
    break;
  }
  case offloadtest::Rule::BufferFloatEpsilon: {
    if (testBufferFloatEpsilon(R.ActualPtr, R.ExpectedPtr, R.Epsilon, R.DM))
      return llvm::Error::success();

    std::ostringstream Oss;
    Oss << std::defaultfloat << R.Epsilon;
    OS << "Comparison Rule: BufferFloatEpsilon\nEpsilon: " << Oss.str() << "\n";
    break;
  }
  }

  OS << "Expected:\n";
  llvm::yaml::Output YAMLOS(OS);
  YAMLOS << *R.ExpectedPtr;
  OS << "Got:\n";
  YAMLOS << *R.ActualPtr;

  // Now print exact hex64 representations of each element of the
  // actual and expected buffers.

  const std::string ExpectedBufferStr =
      getBufferStr(R.ExpectedPtr, R.ComparisonRule);
  const std::string ActualBufferStr =
      getBufferStr(R.ActualPtr, R.ComparisonRule);

  OS << "Full Hex 64bit representation of Expected Buffer Values:\n"
     << ExpectedBufferStr << "\n";
  OS << "Full Hex 64bit representation of Actual Buffer Values:\n"
     << ActualBufferStr << "\n";

  return llvm::createStringError(Str.c_str());
}
