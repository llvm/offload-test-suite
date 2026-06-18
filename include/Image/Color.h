//===- Color.h - Color Description ------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_IMAGE_COLOR_H
#define OFFLOADTEST_IMAGE_COLOR_H

#include <algorithm>
#include <assert.h>
#include <math.h>
#include <tuple>
#include <type_traits>

namespace offloadtest {

namespace ColorUtils {
template <typename T>
std::enable_if_t<std::is_integral_v<T>, double> toInt(double Val) {
  constexpr double Max = static_cast<double>(std::numeric_limits<T>::max());
  constexpr double Base = Max + 1.0;
  double Conv = std::clamp(floor(Val * Base), 0.0, Max);
  return static_cast<T>(Conv);
}

template <typename T>
std::enable_if_t<std::is_arithmetic_v<T>, double> toDouble(T Val) {
  if constexpr (std::is_floating_point_v<T>)
    return Val;
  return static_cast<double>(Val) /
         static_cast<double>(std::numeric_limits<T>::max());
}

template <typename NewTy, typename OldTy> NewTy convertColor(OldTy Val) {
  if constexpr (std::is_same_v<NewTy, OldTy>)
    return Val;
  double Dbl = toDouble(Val);
  assert(Dbl >= 0.0 && Dbl <= 1.0 && "Value should be normalized");
  if constexpr (std::is_integral_v<NewTy>)
    return toInt<NewTy>(Dbl);
  return static_cast<NewTy>(Dbl);
}
} // namespace ColorUtils

enum class ColorSpace { RGB, XYZ, LAB };

template <typename T> class ColorBase {
public:
  T R, G, B;

  ColorSpace Space;

  constexpr ColorBase(T Rx, T Gy, T Bz, ColorSpace CS = ColorSpace::RGB)
      : R(Rx), G(Gy), B(Bz), Space(CS) {}
  ColorBase() = delete;
  ColorBase(const ColorBase &) = default;
  ColorBase(ColorBase &&) = default;
  ColorBase &operator=(const ColorBase &) = default;
  ColorBase &operator=(ColorBase &&) = default;

  template <typename NewTy> ColorBase<NewTy> getAs() {
    ColorBase<NewTy> Val = {ColorUtils::convertColor<NewTy>(R),
                            ColorUtils::convertColor<NewTy>(G),
                            ColorUtils::convertColor<NewTy>(B), Space};
    return Val;
  }

  bool operator==(const ColorBase &RHS) const {
    return std::tie(R, G, B, Space) == std::tie(RHS.R, RHS.G, RHS.B, RHS.Space);
  }
};

class Color : public ColorBase<double> {
public:
  constexpr Color(double Rx, double Gy, double Bz,
                  ColorSpace CS = ColorSpace::RGB)
      : ColorBase(Rx, Gy, Bz, CS) {}
  Color() = delete;
  Color(const Color &) = default;
  Color(Color &&) = default;
  Color &operator=(const Color &) = default;
  Color &operator=(Color &&) = default;

  Color translateSpace(ColorSpace NewCS) {
    if (NewCS == Space)
      return *this;
    return translateSpaceImpl(NewCS);
  }

  Color operator-(const Color &C) const {
    assert(Space == C.Space && "Subtracting colors in different spaces!");
    return Color(abs(R - C.R), abs(G - C.G), abs(B - C.B), Space);
  }

  static double cie75Distance(Color L, Color R) {
    const Color LCol = L.translateSpace(ColorSpace::LAB);
    const Color RCol = R.translateSpace(ColorSpace::LAB);
    const Color Res = LCol - RCol;
    return sqrt((Res.R * Res.R) + (Res.G * Res.G) + (Res.B * Res.B));
  }

private:
  Color translateSpaceImpl(ColorSpace NewCS);
};

} // namespace offloadtest

#endif // OFFLOADTEST_IMAGE_COLOR_H
