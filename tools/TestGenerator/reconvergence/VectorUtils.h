#ifndef VECTORUTILS_H_
#define VECTORUTILS_H_

#include <cstdint>
#include <type_traits>

namespace reconvergence {

template <class R> using add_ref = typename std::add_lvalue_reference<R>::type;

template <class R>
using add_cref =
    typename std::add_lvalue_reference<typename std::add_const<R>::type>::type;

template <typename T, int Size> class Vector {
public:
  Vector() {}
  Vector(T v) {
    for (int i = 0; i < Size; ++i)
      m_data[i] = v;
  }
  Vector(T v0, T v1, T v2, T v3) {
    m_data[0] = v0;
    m_data[1] = v1;
    m_data[2] = v2;
    m_data[3] = v3;
  }
  T &operator[](int ndx) { return m_data[ndx]; }
  const T &operator[](int ndx) const { return m_data[ndx]; }
  T &x() { return m_data[0]; }
  T &y() { return m_data[1]; }
  T &z() { return m_data[2]; }
  T &w() { return m_data[3]; }
  const T &x() const { return m_data[0]; }
  const T &y() const { return m_data[1]; }
  const T &z() const { return m_data[2]; }
  const T &w() const { return m_data[3]; }

private:
  T m_data[Size];
};

typedef Vector<uint32_t, 4> UVec4;

} // namespace reconvergence

#endif // VECTORUTILS_H_
