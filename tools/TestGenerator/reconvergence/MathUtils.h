#ifndef MATHUTILS_H_
#define MATHUTILS_H_

#include <type_traits>
#include <utility>

namespace reconvergence {

#define ROUNDUP(x__, multipler__)                                              \
  ((((x__) + ((multipler__) - 1)) / (multipler__)) * (multipler__))

//! Get maximum of x and y.
template <typename T> inline T max(T x, T y) { return x >= y ? x : y; }

} // namespace reconvergence

#endif // MATHUTILS_H_
