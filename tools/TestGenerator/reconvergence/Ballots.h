#ifndef EXPERIMENTAL_USERS_CLUCIE_BALLOTS_H_
#define EXPERIMENTAL_USERS_CLUCIE_BALLOTS_H_

#include <bitset>
#include <cassert>
#include <cstdint>
#include <vector>

#include "Ballot.h"
#include "VectorUtils.h"

namespace reconvergence {
struct Ballots : protected std::vector<std::bitset<128>> {
  typedef std::vector<value_type> super;

  static const constexpr uint32_t subgroupInvocationSize =
      static_cast<uint32_t>(value_type().size());

  Ballots() : super() {}

  explicit Ballots(uint32_t subgroupCount, const value_type &ballot = {})
      : super(subgroupCount) {
    if (ballot.any())
      *this = ballot;
  }

  Ballots(const Ballots &other) : super(upcast(other)) {}

  using super::operator[];

  using super::at;

  /**
   * @brief size method
   * @return Returns the number of bits that the Ballots holds.
   */
  uint32_t size() const {
    return static_cast<uint32_t>(super::size() * subgroupInvocationSize);
  }

  /**
   * @brief count method
   * @return Returns the number of bits that are set to true.
   */
  uint32_t count() const {
    uint32_t n = 0u;
    for (const value_type &b : *this)
      n += static_cast<uint32_t>(b.count());
    return n;
  }

  /**
   * @brief count method
   * @return Returns the number of bits that are set to true in given subgroup.
   */
  uint32_t count(uint32_t subgroup) const {
    assert(subgroup < subgroupCount());
    return static_cast<uint32_t>(at(subgroup).count());
  }

  uint32_t subgroupCount() const {
    return static_cast<uint32_t>(super::size());
  }

  bool test(uint32_t bit) const {
    assert(bit < size());
    return at(bit / subgroupInvocationSize).test(bit % subgroupInvocationSize);
  }

  bool set(uint32_t bit, bool value = true) {
    assert(bit <= size());
    const bool before = test(bit);
    at(bit / subgroupInvocationSize).set((bit % subgroupInvocationSize), value);
    return before;
  }

  void full() {
    const uint32_t bb = size();
    for (uint32_t b = 0u; b < bb; ++b)
      set(b);
  }

  Ballots &setn(uint32_t bits) {
    for (uint32_t i = 0u; i < bits; ++i)
      set(i);
    return *this;
  }

  bool all() const {
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g) {
      if (false == at(g).all())
        return false;
    }
    return (gg != 0u);
  }

  bool none() const {
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g) {
      if (false == at(g).none())
        return false;
    }
    return (gg != 0u);
  }

  bool any() const {
    bool res = false;
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g)
      res |= super::at(g).any();
    return res;
  }

  static uint32_t findBit(uint32_t otherFullyQualifiedInvocationID,
                          uint32_t otherSubgroupSize) {
    return (((otherFullyQualifiedInvocationID / otherSubgroupSize) *
             subgroupInvocationSize) +
            (otherFullyQualifiedInvocationID % otherSubgroupSize));
  }

  inline const super &upcast(const Ballots &other) const {
    return static_cast<const super &>(other);
  }

  Ballots &operator&=(const Ballots &other) {
    assert(subgroupCount() == other.subgroupCount());
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g)
      super::at(g) = super::at(g) & upcast(other).at(g);
    return *this;
  }

  Ballots operator&(const Ballots &other) const {
    Ballots res(*this);
    res &= other;
    return res;
  }

  Ballots &operator|=(const Ballots &other) {
    assert(subgroupCount() == other.subgroupCount());
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g)
      super::at(g) = super::at(g) | upcast(other).at(g);
    return *this;
  }

  Ballots operator|(const Ballots &other) const {
    Ballots res(*this);
    res |= other;
    return res;
  }

  Ballots &operator<<=(uint32_t bits) { return ((*this) = ((*this) << bits)); }

  Ballots operator<<(uint32_t bits) const {
    Ballots res(subgroupCount());
    if (bits < size() && bits != 0u) {
      for (uint32_t b = 0; b < size() - bits; ++b)
        res.set((b + bits), test(b));
    }
    return res;
  }

  Ballots operator~() const {
    Ballots res(*this);
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g)
      res.at(g) = super::at(g).operator~();
    return res;
  }

  bool operator==(const Ballots &other) const {
    if (super::size() == upcast(other).size()) {
      const uint32_t gg = subgroupCount();
      for (uint32_t g = 0u; g < gg; ++g) {
        if (at(g) != other[g])
          return false;
      }
      return true;
    }
    return false;
  }

  Ballots &operator=(const Ballots &other) {
    assert((subgroupCount() == other.subgroupCount()));
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g)
      at(g) = other.at(g);
    return *this;
  }

  Ballots &operator=(const value_type &forAllGroups) {
    assert(super::size() >= 1u);
    const uint32_t gg = subgroupCount();
    for (uint32_t g = 0u; g < gg; ++g)
      at(g) = forAllGroups;
    return *this;
  }
};
} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_BALLOTS_H_
