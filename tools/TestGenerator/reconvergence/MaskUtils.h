#ifndef EXPERIMENTAL_USERS_CLUCIE_MASKUTILS_H_
#define EXPERIMENTAL_USERS_CLUCIE_MASKUTILS_H_
#include <bitset>
#include <cstdint>
#include <limits>

#include "Ballot.h"
#include "Ballots.h"
#include "VectorUtils.h"

namespace reconvergence {

typedef std::bitset<128> bitset_inv_t;

inline uint64_t subgroupSizeToMask(uint32_t subgroupSize) {
  if (subgroupSize == 64)
    return ~0ULL;
  else
    return (1ULL << subgroupSize) - 1;
}

inline Ballot subgroupSizeToMask(uint32_t subgroupSize,
                                 uint32_t subgroupCount) {
  Ballot b;
  assert(subgroupSize <= b.size());
  for (uint32_t i = 0; i < subgroupSize; ++i)
    b.set(i);
  return b;
}

// Take a 64-bit integer, mask it to the subgroup size, and then
// replicate it for each subgroup
inline bitset_inv_t bitsetFromU64(uint64_t mask, uint32_t subgroupSize) {
  mask &= subgroupSizeToMask(subgroupSize);
  bitset_inv_t result(mask);
  for (uint32_t i = 0; i < result.size() / subgroupSize - 1; ++i) {
    result = (result << subgroupSize) | bitset_inv_t(mask);
  }
  return result;
}

inline Ballots ballotsFromU64(uint64_t maskValue, uint32_t subgroupSize,
                              uint32_t subgroupCount) {
  Ballot b(maskValue);
  b &= subgroupSizeToMask(subgroupSize, subgroupCount);
  Ballots result(subgroupCount);
  for (uint32_t g = 0; g < subgroupCount; ++g)
    result.at(g) = b;
  return result;
}

inline Ballots ballotsFromBallot(Ballot b, uint32_t subgroupSize,
                                 uint32_t subgroupCount) {
  b &= subgroupSizeToMask(subgroupSize, subgroupCount);
  Ballots result(subgroupCount);
  for (uint32_t g = 0; g < subgroupCount; ++g)
    result.at(g) = b;
  return result;
}

// Pick out the mask for the subgroup that invocationID is a member of
inline uint64_t bitsetToU64(const bitset_inv_t &bitset, uint32_t subgroupSize,
                            uint32_t invocationID) {
  bitset_inv_t copy(bitset);
  copy >>= (invocationID / subgroupSize) * subgroupSize;
  copy &= bitset_inv_t(subgroupSizeToMask(subgroupSize));
  uint64_t mask = copy.to_ullong();
  mask &= subgroupSizeToMask(subgroupSize);
  return mask;
}

// Pick out the mask for the subgroup that invocationID is a member of
inline Ballot bitsetToBallot(const Ballots &bitset, uint32_t subgroupSize,
                             uint32_t invocationID) {
  return bitset.at(invocationID / subgroupSize) &
         subgroupSizeToMask(subgroupSize, bitset.subgroupCount());
}

inline Ballot bitsetToBallot(uint64_t value, uint32_t subgroupCount,
                             uint32_t subgroupSize, uint32_t invocationID) {
  Ballots bs = ballotsFromU64(value, subgroupSize, subgroupCount);
  return bitsetToBallot(bs, subgroupSize, invocationID);
}

inline int findLSB(uint64_t value) {
  for (int i = 0; i < 64; i++) {
    if (value & (1ULL << i))
      return i;
  }
  return -1;
}

template <uint32_t N> static uint32_t findLSB(const std::bitset<N> &value) {
  for (uint32_t i = 0u; i < N; ++i) {
    if (value.test(i))
      return i;
  }
  return std::numeric_limits<uint32_t>::max();
}

// For each subgroup, pick out the elected invocationID, and accumulate
// a bitset of all of them
inline bitset_inv_t bitsetElect(const bitset_inv_t &value,
                                int32_t subgroupSize) {
  bitset_inv_t ret; // zero initialized

  for (int32_t i = 0; i < (int32_t)value.size(); i += subgroupSize) {
    uint64_t mask = bitsetToU64(value, subgroupSize, i);
    int lsb = findLSB(mask);
    ret |= bitset_inv_t(lsb == -1 ? 0 : (1ULL << lsb)) << i;
  }
  return ret;
}

inline Ballots bitsetElect(const Ballots &value) {
  Ballots ret(value.subgroupCount());
  for (uint32_t g = 0u; g < value.subgroupCount(); ++g) {
    const uint32_t lsb = findLSB<Ballots::subgroupInvocationSize>(value.at(g));
    if (lsb != std::numeric_limits<uint32_t>::max()) {
      ret.at(g).set(lsb);
    }
  }
  return ret;
}

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_MASKUTILS_H_
