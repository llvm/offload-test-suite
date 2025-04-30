//===- ImageComparators.cpp - Image Comparators -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/ImageComparators.h"

using namespace llvm;
using namespace offloadtest;

void yaml::MappingTraits<CompareCheck>::mapping(IO &I, CompareCheck &C) {
  I.mapRequired("Type", C.Type);
  if (C.Type == CompareCheck::Intervals)
    I.mapRequired("Vals", C.Vals);
  else
    I.mapRequired("Val", C.Val);
}

bool ImageComparatorDistance::evaluateCheck(const CompareCheck &C,
                                            llvm::raw_ostream &Err) {
  switch (C.Type) {
  case CompareCheck::Furthest:
    if (Furthest > C.Val) {
      Err << "Furthest color distance check failed: " << Furthest
          << " above threshold " << C.Val;
      return false;
    }
    return true;
  case CompareCheck::RMS:
    if (RMS > C.Val) {
      Err << "RMS check failed. RMS " << RMS << " above threshold " << C.Val;
      return false;
    }
    return true;
  case CompareCheck::DiffRMS:
    if (DiffRMS > C.Val) {
      Err << "Differing RMS check failed. RMS of differing pixels " << DiffRMS
          << " above threshold " << C.Val;
      return false;
    }
    return true;
  case CompareCheck::PixelPercent: {
    double const DiffPercent = (static_cast<double>(VisibleDiffs) /
                          static_cast<double>(Count) * 100.0);
    if (DiffPercent > C.Val) {
      Err << "PixelPercent check failed. Difference percent " << DiffPercent
          << " above threshold " << C.Val;
      return false;
    }
    return true;
  }
  case CompareCheck::Intervals: {
    double const DblCount = static_cast<double>(Count);
    for (uint32_t I = 0; I < 10; ++I) {
      if (I >= C.Vals.size()) {
        if (Histogram[I] == 0)
          continue;
        Err << "Interval[" << I
            << "]: Contains non-zero value: " << Histogram[I];
        return false;
      }

      double const DiffPercent =
          (static_cast<double>(Histogram[I]) / DblCount * 100.0);
      if (DiffPercent > C.Vals[I]) {
        Err << "Interval[" << I << "]: Out of range. Maximum: " << C.Vals[I]
            << " Actual: " << DiffPercent;
        return false;
      }
    }
    for (int I = C.Vals.size(); I < 10; ++I) {
    }
    return true;
  }
  case CompareCheck::None:
    llvm_unreachable("Check run with no rule");
  }
}
