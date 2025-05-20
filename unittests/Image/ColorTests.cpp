//===- ColorTests.cpp - Color Tests -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/Color.h"

#include "gtest/gtest.h"

#include <cstdint>

using namespace offloadtest;

TEST(ColorTests, RoundTrip) {
  {
    Color RGB = Color(1.0, 0.5, 0.0);
    Color XYZ = RGB.translateSpace(ColorSpace::XYZ);
    Color LAB = RGB.translateSpace(ColorSpace::LAB);

    Color XYZPrime = LAB.translateSpace(ColorSpace::XYZ);
    Color RGBPrime = LAB.translateSpace(ColorSpace::RGB);

    // The floating point rounding errors across these transformations are
    // significant, so rather than comparing the float values we normalize them
    // back into integer color values to compare.
    EXPECT_EQ(XYZ.getAs<uint16_t>(), XYZPrime.getAs<uint16_t>());
    EXPECT_EQ(RGB.getAs<uint16_t>(), RGBPrime.getAs<uint16_t>());
  }

  {
    Color RGB = Color(0.125, 0.896, 0.652);
    Color XYZ = RGB.translateSpace(ColorSpace::XYZ);
    Color LAB = RGB.translateSpace(ColorSpace::LAB);

    Color XYZPrime = LAB.translateSpace(ColorSpace::XYZ);
    Color RGBPrime = LAB.translateSpace(ColorSpace::RGB);

    // The floating point rounding errors across these transformations are
    // significant, so rather than comparing the float values we normalize them
    // back into integer color values to compare.
    EXPECT_EQ(XYZ.getAs<uint16_t>(), XYZPrime.getAs<uint16_t>());
    EXPECT_EQ(RGB.getAs<uint16_t>(), RGBPrime.getAs<uint16_t>());
  }
}
