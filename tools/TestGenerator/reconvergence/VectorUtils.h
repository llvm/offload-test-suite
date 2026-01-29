#ifndef EXPERIMENTAL_USERS_CLUCIE_VECTORUTILS_H_
#define EXPERIMENTAL_USERS_CLUCIE_VECTORUTILS_H_

#include <cstdint>
#include <type_traits>

namespace reconvergence {

template <class R> using add_ref = typename std::add_lvalue_reference<R>::type;

template <class R>
using add_cref =
    typename std::add_lvalue_reference<typename std::add_const<R>::type>::type;

template <class X> using add_ptr = std::add_pointer_t<X>;

template <class X> using add_cptr = std::add_pointer_t<std::add_const_t<X>>;

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

typedef Vector<float, 1> Vec1;
typedef Vector<float, 2> Vec2;
typedef Vector<float, 3> Vec3;
typedef Vector<float, 4> Vec4;

typedef Vector<int, 1> IVec1;
typedef Vector<int, 2> IVec2;
typedef Vector<int, 3> IVec3;
typedef Vector<int, 4> IVec4;

typedef Vector<uint32_t, 1> UVec1;
typedef Vector<uint32_t, 2> UVec2;
typedef Vector<uint32_t, 3> UVec3;
typedef Vector<uint32_t, 4> UVec4;

typedef Vector<int64_t, 1> I64Vec1;
typedef Vector<int64_t, 2> I64Vec2;
typedef Vector<int64_t, 3> I64Vec3;
typedef Vector<int64_t, 4> I64Vec4;

typedef Vector<uint64_t, 1> U64Vec1;
typedef Vector<uint64_t, 2> U64Vec2;
typedef Vector<uint64_t, 3> U64Vec3;
typedef Vector<uint64_t, 4> U64Vec4;

typedef Vector<bool, 1> BVec1;
typedef Vector<bool, 2> BVec2;
typedef Vector<bool, 3> BVec3;
typedef Vector<bool, 4> BVec4;

typedef Vector<double, 2> DVec2;
typedef Vector<double, 3> DVec3;
typedef Vector<double, 4> DVec4;

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_VECTORUTILS_H_
