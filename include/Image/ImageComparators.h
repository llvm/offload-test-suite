//===- ImageComparators.h - Image Comparators -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_IMAGE_IMAGECOMPARATOR_H
#define OFFLOADTEST_IMAGE_IMAGECOMPARATOR_H

#include "Image/Color.h"
#include "Image/Image.h"

#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

namespace offloadtest {

struct CompareCheck {
  enum CheckType {
    None,
    Furthest,
    RMS,
    DiffRMS,
    PixelPercent,
    Intervals,
  } Type = None;
  double Val = 0;
  std::vector<double> Vals = {};
};

class ImageComparatorDistance : public ImageComparatorBase {
  double Furthest = 0.0;
  double RMS = 0.0;
  double DiffRMS = 0.0;
  uint64_t Count = 0;
  uint64_t VisibleDiffs = 0;
  uint64_t Histogram[10] = {0};

  llvm::SmallVector<CompareCheck> Checks;

  llvm::SmallString<256> ErrStr;

  // 2.3 corresponds to a "just noticeable distance" for the CIE76
  // difference algorithm. If no pixels are worse than that, there should be
  // no noticeable difference in the image.
  static constexpr double VisibleDiff = 2.3;

public:
  ImageComparatorDistance() {
    Checks.push_back(CompareCheck{CompareCheck::Furthest, VisibleDiff});
  }
  ImageComparatorDistance(llvm::ArrayRef<CompareCheck> C) : Checks(C) {}
  void processPixel(Color L, Color R) override {
    const double Distance = Color::cie75Distance(L, R);
    if (Distance > VisibleDiff) {
      VisibleDiffs += 1;
      DiffRMS += Distance;

      // A difference >10 is a more than 10% off in the L*a*b color space.
      const int Idx =
          static_cast<int>(std::clamp(Distance - VisibleDiff, 0.0, 9.0));
      Histogram[Idx] += 1;
    }

    Furthest = std::max(Furthest, Distance);
    RMS += Distance;
    Count += 1;
  }

  void print(llvm::raw_ostream &OS) override {
    const double CountDbl = static_cast<double>(Count);
    OS << "RMS Difference: " << RMS << "\n";
    OS << "Furthest Pixel Difference: " << Furthest << "\n";
    OS << "Pixels with visible differences: " << VisibleDiffs << " "
       << (static_cast<double>(VisibleDiffs) / CountDbl * 100.0) << "%\n";
    OS << "RMS Different Pixels Only: " << DiffRMS << "\n";
    OS << "Total Pixels: " << Count << "\n";
    OS << "Histogram Data:\n";
    for (int I = 0; I < 10; ++I) {
      OS << "\t[" << I << "]: " << Histogram[I] << " "
         << (static_cast<double>(Histogram[I]) / CountDbl * 100.0) << "\n";
    }
    if (!ErrStr.empty())
      OS << "Error: " << ErrStr << "\n";
  }

  bool evaluateCheck(const CompareCheck &C, llvm::raw_ostream &Err);

  bool result() override {
    RMS = sqrt(RMS / static_cast<double>(Count));
    DiffRMS = sqrt(DiffRMS / static_cast<double>(VisibleDiffs));
    llvm::raw_svector_ostream Err(ErrStr);

    for (const auto &C : Checks) {
      if (!evaluateCheck(C, Err))
        return false;
    }

    return true;
  }
};

class ImageComparatorDiffImage : public ImageComparatorBase {
  Image DiffImg;
  float *DiffPtr;
  llvm::StringRef OutputFilename;

public:
  virtual ~ImageComparatorDiffImage() {}
  ImageComparatorDiffImage(uint32_t Height, uint32_t Width, llvm::StringRef OF)
      : DiffImg(Height, Width, 4, 3, true), OutputFilename(OF) {
    DiffPtr = reinterpret_cast<float *>(DiffImg.data());
  }
  void processPixel(Color L, Color R) override {
    // TODO: I should probably instead use a color distance for this too...
    DiffPtr[0] = abs(L.R - R.R);
    DiffPtr[1] = abs(L.G - R.G);
    DiffPtr[2] = abs(L.B - R.B);
    DiffPtr += 3;
  }

  void print(llvm::raw_ostream &) override {
    llvm::consumeError(Image::writePNG(DiffImg, OutputFilename));
  }
};
} // namespace offloadtest

LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::CompareCheck)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<offloadtest::CompareCheck> {
  static void mapping(IO &I, offloadtest::CompareCheck &C);
};

template <>
struct ScalarEnumerationTraits<offloadtest::CompareCheck::CheckType> {
  static void enumeration(IO &I, offloadtest::CompareCheck::CheckType &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::CompareCheck::Val)
    ENUM_CASE(Furthest);
    ENUM_CASE(RMS);
    ENUM_CASE(DiffRMS);
    ENUM_CASE(PixelPercent);
    ENUM_CASE(Intervals);
#undef ENUM_CASE
  }
};
} // namespace yaml
} // namespace llvm

#endif // OFFLOADTEST_IMAGE_IMAGECOMPARATOR_H
