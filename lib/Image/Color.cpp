//===- Color.cpp - Color Description ----------------------------*- C++ -*-===//
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

#include <math.h>

using namespace offloadtest;

constexpr Color D65WhitePoint =
    Color(95.047, 100.000, 108.883, ColorSpace::XYZ);

static Color multiply(const Color LHS, double Mat[9], ColorSpace NewSpace) {
  double X, Y, Z;
  X = (LHS.R * Mat[0]) + (LHS.G * Mat[1]) + (LHS.B * Mat[2]);
  Y = (LHS.R * Mat[3]) + (LHS.G * Mat[4]) + (LHS.B * Mat[5]);
  Z = (LHS.R * Mat[6]) + (LHS.G * Mat[7]) + (LHS.B * Mat[8]);
  return Color(X, Y, Z, NewSpace);
}

static Color clampColor(const Color C) {
  return Color(std::clamp(C.R, 0.0, 1.0), std::clamp(C.G, 0.0, 1.0),
               std::clamp(C.B, 0.0, 1.0), C.Space);
}

static Color RGBToXYZ(const Color Old) {
  // Matrix assumes D65 white point.
  // Source: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
  double Mat[] = {0.4124564, 0.3575761, 0.1804375, 0.2126729, 0.7151522,
                  0.0721750, 0.0193339, 0.1191920, 0.9503041};
  return multiply(Old, Mat, ColorSpace::XYZ);
}

static Color XYZToRGB(const Color Old) {
  // Matrix assumes D65 white point.
  // Source: http://brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
  double Mat[] = {3.2404542, -1.5371385, -0.4985314, -0.9692660, 1.8760108,
                  0.0415560, 0.0556434,  -0.2040259, 1.0572252};
  return clampColor(multiply(Old, Mat, ColorSpace::RGB));
}

static double convertXYZ(double Val) {
  constexpr double E = 216.0 / 24389.0;
  constexpr double K = 24389.0 / 27.0;
  return Val > E ? pow(Val, 1.0 / 3.0) : (K * Val + 16.0) / 116.0;
}

static Color XYZToLAB(const Color Old) {
  double X = convertXYZ(Old.R / D65WhitePoint.R);
  double Y = convertXYZ(Old.G / D65WhitePoint.G);
  double Z = convertXYZ(Old.B / D65WhitePoint.B);

  double L = fmax(0.0, 116.0 * Y - 16.0);
  double A = 500 * (X - Y);
  double B = 200 * (Y - Z);
  return Color(L, A, B, ColorSpace::LAB);
}

static double convertLAB(double Val) {
  double ValPow3 = pow(Val, 3.0);
  if (ValPow3 > 0.008856)
    return ValPow3;

  constexpr double V = 16.0 / 116.0;
  constexpr double K = (24389.0 / 27.0) / 116.0;
  return (Val - V) / K;
}

static Color LABToXYZ(const Color Old) {
  double Y = (Old.R + 16) / 116;
  double X = Old.G / 500 + Y;
  double Z = Y - Old.B / 200;

  X = convertLAB(X) * D65WhitePoint.R;
  Y = convertLAB(Y) * D65WhitePoint.G;
  Z = convertLAB(Z) * D65WhitePoint.B;

  return Color(X, Y, Z, ColorSpace::XYZ);
}

Color Color::translateSpaceImpl(ColorSpace NewCS) {
  if (Space == NewCS)
    return *this;
  Color Tmp = *this;
  if (Space == ColorSpace::RGB)
    Tmp = RGBToXYZ(*this);
  else if (Space == ColorSpace::LAB)
    Tmp = LABToXYZ(*this);
  // Tmp is now in XYZ space.
  if (NewCS == ColorSpace::RGB)
    return XYZToRGB(Tmp);
  if (NewCS == ColorSpace::LAB)
    return XYZToLAB(Tmp);
  return Tmp;
}
