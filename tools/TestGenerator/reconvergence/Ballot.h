#ifndef EXPERIMENTAL_USERS_CLUCIE_BALLOT_H_
#define EXPERIMENTAL_USERS_CLUCIE_BALLOT_H_

#include <bitset>
#include <cstdint>
#include <ostream>

#include "VectorUtils.h"

namespace reconvergence {

struct Ballot : public std::bitset<128> {
  typedef std::bitset<128> super;

  Ballot() : super() {}

  Ballot(add_cref<super> ballot, uint32_t printbits = 128u)
      : super(ballot), m_bits(printbits) {}

  Ballot(add_cref<UVec4> ballot, uint32_t printbits = 128u)
      : super(), m_bits(printbits) {
    *this = ballot;
  }

  Ballot(uint64_t val, uint32_t printbits = 128u)
      : super(val), m_bits(printbits) {}

  static Ballot withSetBit(uint32_t bit) {
    Ballot b;
    b.set(bit);
    return b;
  }

  constexpr uint32_t size() const {
    return static_cast<uint32_t>(super::size());
  }

  operator UVec4() const {
    UVec4 result;
    super ballot(*this);
    const super mask = 0xFFFFFFFF;
    for (uint32_t k = 0; k < 4u; ++k) {
      result[k] = static_cast<uint32_t>((ballot & mask).to_ulong());
      ballot >>= 32;
    }
    return result;
  }

  add_ref<Ballot> operator=(add_cref<UVec4> vec) {
    for (uint32_t k = 0; k < 4u; ++k) {
      (*this) <<= 32;
      (*this) |= vec[3 - k];
    }
    return *this;
  }

protected:
  uint32_t m_bits;
};
} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_BALLOT_H_
