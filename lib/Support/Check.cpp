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
#include <map>
#include <sstream>
#include <tuple>

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
  if (B1->ArraySize != B2->ArraySize || B1->size() != B2->size())
    return false;

  for (auto B1Data = B1->Data.begin(), B2Data = B2->Data.begin();
       B1Data != B1->Data.end() && B2Data != B2->Data.end();
       ++B1Data, ++B2Data) {
    for (uint32_t I = 0; I < B1->size(); ++I) {
      if (B1Data->get()[I] != B2Data->get()[I])
        return false;
    }
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
static bool testAllArray(std::function<bool(const T &, const T &)> ComparisonFn,
                         offloadtest::Buffer *B1, offloadtest::Buffer *B2) {
  if (B1->ArraySize != B2->ArraySize || B1->size() != B2->size())
    return false;

  for (auto B1Data = B1->Data.begin(), B2Data = B2->Data.begin();
       B1Data != B1->Data.end() && B2Data != B2->Data.end();
       ++B1Data, ++B2Data) {
    const llvm::ArrayRef<T> Arr1(reinterpret_cast<T *>(B1Data->get()),
                                 B1->Size / sizeof(T));
    const llvm::ArrayRef<T> Arr2(reinterpret_cast<T *>(B2Data->get()),
                                 B2->Size / sizeof(T));
    if (!testAll<T>(ComparisonFn, Arr1, Arr2))
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
  case offloadtest::DataFormat::Float64:
    return testAllArray<double>(ComparisonFn, B1, B2);
  case offloadtest::DataFormat::Float32:
    return testAllArray<float>(ComparisonFn, B1, B2);
  case offloadtest::DataFormat::Float16: {
    return testAllArray<uint16_t>(ComparisonFn, B1, B2);
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

static bool testBufferParticipantPattern(offloadtest::Buffer *B1,
                                         offloadtest::Buffer *B2,
                                         unsigned GroupSize,
                                         std::string &ErrorMsg) {
  // Expect 3 x uint32_t: (combinedId, maskLow, maskHigh)
  if (GroupSize == 0) {
    ErrorMsg = "Invalid GroupSize (must be > 0)";
    return false;
  }

  // Basic structural checks similar to testBufferExact
  if (B1->ArraySize != B2->ArraySize || B1->size() != B2->size()) {
    ErrorMsg = "Mismatched buffer shape (ArraySize or per-chunk size differs)";
    return false;
  }

  // We operate on 32-bit words
  if ((B1->size() % sizeof(uint32_t)) != 0) {
    ErrorMsg = "Chunk size is not a multiple of 4 bytes";
    return false;
  }
  if ((B2->size() % sizeof(uint32_t)) != 0) {
    ErrorMsg = "Expected chunk size is not a multiple of 4 bytes";
    return false;
  }

  const uint32_t WordsPerChunk =
      static_cast<uint32_t>(B1->size() / sizeof(uint32_t));
  if (WordsPerChunk % GroupSize != 0) {
    ErrorMsg = "Words per chunk must be a multiple of GroupSize";
    return false;
  }

  using PatternTuple = std::tuple<uint32_t, uint32_t, uint32_t>;
  std::map<PatternTuple, unsigned> ActualPatterns;
  std::map<PatternTuple, unsigned> ExpectedPatterns;

  auto read_u32 = [](const char *Base, uint32_t WordIndex) -> uint32_t {
    uint32_t V;
    std::memcpy(&V, Base + WordIndex * sizeof(uint32_t), sizeof(uint32_t));
    return V;
  };

  // Accumulate patterns from all chunks
  auto* B1It = B1->Data.begin();
  auto* B2It = B2->Data.begin();
  for (; B1It != B1->Data.end() && B2It != B2->Data.end(); ++B1It, ++B2It) {
    const char *ABuf = B1It->get(); // unique_ptr<char[]> -> char*
    const char *EBuf = B2It->get();

    for (uint32_t I = 0; I + GroupSize <= WordsPerChunk; I += GroupSize) {
      if (GroupSize == 3) {
        // Actual
        PatternTuple Ap(read_u32(ABuf, I + 0), read_u32(ABuf, I + 1),
                        read_u32(ABuf, I + 2));
        ++ActualPatterns[Ap];

        // Expected
        const PatternTuple Ep(read_u32(EBuf, I + 0), read_u32(EBuf, I + 1),
                        read_u32(EBuf, I + 2));
        ++ExpectedPatterns[Ep];
      } else {
        // If you plan to support other group sizes later, handle here.
      }
    }
  }

  // Compare pattern multisets
  std::stringstream Ss;
  bool HasError = false;

  if (ActualPatterns.size() != ExpectedPatterns.size()) {
    Ss << "Pattern kind count mismatch: actual has " << ActualPatterns.size()
       << " unique patterns, expected has " << ExpectedPatterns.size()
       << " unique patterns\n";
    HasError = true;
  }

  // Missing / count-mismatched patterns
  for (const auto &[pattern, expCount] : ExpectedPatterns) {
    auto It = ActualPatterns.find(pattern);
    if (It == ActualPatterns.end()) {
      if (!HasError)
        Ss << "Pattern differences found:\n";
      HasError = true;
      Ss << "  Missing pattern (combineId=" << std::get<0>(pattern)
         << ", maskLow=0x" << std::hex << std::get<1>(pattern)
         << ", maskHigh=0x" << std::get<2>(pattern) << std::dec
         << ") - expected count: " << expCount << ", actual count: 0\n";
    } else if (It->second != expCount) {
      if (!HasError)
        Ss << "Pattern differences found:\n";
      HasError = true;
      Ss << "  Pattern (combineId=" << std::get<0>(pattern) << ", maskLow=0x"
         << std::hex << std::get<1>(pattern) << ", maskHigh=0x"
         << std::get<2>(pattern) << std::dec
         << ") - expected count: " << expCount
         << ", actual count: " << It->second << "\n";
    }
  }

  // Unexpected patterns
  for (const auto &[pattern, actCount] : ActualPatterns) {
    if (ExpectedPatterns.find(pattern) == ExpectedPatterns.end()) {
      if (!HasError)
        Ss << "Pattern differences found:\n";
      HasError = true;
      Ss << "  Unexpected pattern (combineId=" << std::get<0>(pattern)
         << ", maskLow=0x" << std::hex << std::get<1>(pattern)
         << ", maskHigh=0x" << std::get<2>(pattern) << std::dec
         << ") - expected count: 0, actual count: " << actCount << "\n";
    }
  }

  if (HasError) {
    ErrorMsg = Ss.str();
    return false;
  }
  return true;
}

template <typename T>
static std::string bitPatternAsHex64(const T &Val,
                                     offloadtest::Rule ComparisonRule) {
  static_assert(sizeof(T) <= sizeof(uint64_t), "Type too large for Hex64");

  std::ostringstream Oss;
  if (ComparisonRule == offloadtest::Rule::BufferExact)
    Oss << "0x" << std::hex << Val;
  else
    Oss << std::hexfloat << Val;
  return Oss.str();
}

template <typename T>
static void formatBuffer(llvm::ArrayRef<T> Arr, offloadtest::Rule Rule,
                         llvm::raw_svector_ostream &Result) {
  if (Arr.empty())
    return;

  Result << "[ " << bitPatternAsHex64(Arr[0], Rule);
  for (size_t I = 1; I < Arr.size(); ++I)
    Result << ", " << bitPatternAsHex64(Arr[I], Rule);
  Result << " ]";
}

template <typename T>
static void formatBufferArray(offloadtest::Buffer *B, offloadtest::Rule Rule,
                              llvm::raw_svector_ostream &Result) {
  assert(B->ArraySize > 1 && "Buffer must be an array to format as array");
  for (const auto &DataPtr : B->Data) {
    if (DataPtr != *B->Data.begin())
      Result << "\n";
    Result << " - ";
    formatBuffer(llvm::ArrayRef<T>(reinterpret_cast<T *>(DataPtr.get()),
                                   B->Size / sizeof(T)),
                 Rule, Result);
  }
}

template <typename T>
static std::string formatBuffer(offloadtest::Buffer *B,
                                offloadtest::Rule Rule) {
  llvm::SmallString<256> Str;
  llvm::raw_svector_ostream Result(Str);

  if (B->ArraySize > 1)
    formatBufferArray<T>(B, Rule, Result);
  else
    formatBuffer(llvm::ArrayRef<T>(reinterpret_cast<T *>(B->Data.back().get()),
                                   B->Size / sizeof(T)),
                 Rule, Result);

  return std::string(Result.str());
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
    break;
  }
  case offloadtest::Rule::BufferParticipantPattern: {
    std::string ErrorMsg;
    if (testBufferParticipantPattern(R.ActualPtr, R.ExpectedPtr, R.GroupSize,
                                     ErrorMsg))
      return llvm::Error::success();
    // Return error with detailed message
    return llvm::make_error<llvm::StringError>(
        "BufferParticipantPattern test failed for " + R.Name + ":\n" + ErrorMsg,
        std::error_code());

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
