#ifndef EXPERIMENTAL_USERS_CLUCIE_MATHUTILS_H_
#define EXPERIMENTAL_USERS_CLUCIE_MATHUTILS_H_

#include <type_traits>
#include <utility>

namespace reconvergence {

#define ROUNDUP(x__, multipler__)                                              \
  ((((x__) + ((multipler__) - 1)) / (multipler__)) * (multipler__))
#define ROUNDDOWN(x__, multipler__) (((x__) / (multipler__)) * (multipler__))

//! Compute absolute value of x.
template <typename T> inline T abs(T x) { return x < T(0) ? -x : x; }

//! Get minimum of x and y.
template <typename T> inline T min(T x, T y) { return x <= y ? x : y; }

//! Get maximum of x and y.
template <typename T> inline T max(T x, T y) { return x >= y ? x : y; }

//! Clamp x in range a <= x <= b.
template <typename T> inline T clamp(T x, T a, T b) {
  assert(a <= b);
  return x < a ? a : (x > b ? b : x);
}

//! Test if x is in bounds a <= x < b.
template <typename T> inline bool inBounds(T x, T a, T b) {
  return a <= x && x < b;
}

//! Test if x is in range a <= x <= b.
template <typename T> inline bool inRange(T x, T a, T b) {
  return a <= x && x <= b;
}

//! Return T with low n bits set
template <typename T> inline T rightSetMask(T n) {
  DE_ASSERT(n < T(sizeof(T) * 8));
  T one = T(1);
  return T((one << n) - one);
}

//! Return T with low n bits reset
template <typename T> inline T rightZeroMask(T n) {
  return T(~rightSetMask(n));
}

//! Return T with high n bits set
template <typename T> inline T leftSetMask(T n) {
  const T tlen = T(sizeof(T) * 8);
  return T(~rightSetMask(tlen >= n ? tlen - n : T(0)));
}

//! Return T with high n bits reset
template <typename T> inline T leftZeroMask(T n) { return T(~leftSetMask(n)); }

//! Round x up to a multiple of y.
template <typename T> inline T roundUp(T x, T y) {
  DE_ASSERT(y != T(0));
  const T mod = x % y;
  return x + ((mod == T(0)) ? T(0) : (y - mod));
}

//! Round x down to a multiple of y.
template <typename T> inline T roundDown(T x, T y) {
  DE_ASSERT(y != T(0));
  return (x / y) * y;
}

//! Find the greatest common divisor of x and y.
template <typename T> T gcd(T x, T y) {
  DE_ASSERT(std::is_integral<T>::value && std::is_unsigned<T>::value);

  // Euclidean algorithm.
  while (y != T{0}) {
    T mod = x % y;
    x = y;
    y = mod;
  }

  return x;
}

//! Find the least common multiple of x and y.
template <typename T> T lcm(T x, T y) {
  DE_ASSERT(std::is_integral<T>::value && std::is_unsigned<T>::value);

  T prod = x * y;
  DE_ASSERT(x == 0 || prod / x == y); // Check overflow just in case.
  return (prod) / gcd(x, y);
}

//! Helper for DE_CHECK() macros.
void throwRuntimeError(const char *message, const char *expr, const char *file,
                       int line);

//! Default deleter.
template <typename T> struct DefaultDeleter {
  inline DefaultDeleter(void) {}
  template <typename U> inline DefaultDeleter(const DefaultDeleter<U> &) {}
  template <typename U>
  inline DefaultDeleter<T> &operator=(const DefaultDeleter<U> &) {
    return *this;
  }
  inline void operator()(T *ptr) const { delete ptr; }
};

//! A deleter for arrays
template <typename T> struct ArrayDeleter {
  inline ArrayDeleter(void) {}
  template <typename U> inline ArrayDeleter(const ArrayDeleter<U> &) {}
  template <typename U>
  inline ArrayDeleter<T> &operator=(const ArrayDeleter<U> &) {
    return *this;
  }
  inline void operator()(T *ptr) const { delete[] ptr; }
};

//! Get required memory alignment for type
template <typename T> size_t alignOf(void) {
  struct PaddingCheck {
    uint8_t b;
    T t;
  };
  return (size_t)offsetof(PaddingCheck, t);
}

//! Similar to DE_LENGTH_OF_ARRAY but constexpr and without auxiliar helpers.
template <typename T, size_t N> constexpr size_t arrayLength(const T (&)[N]) {
  return N;
}

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_MATHUTILS_H_
