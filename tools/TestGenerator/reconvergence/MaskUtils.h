#ifndef MASKUTILS_H_
#define MASKUTILS_H_
#include <bitset>
#include <cassert>
#include <cstdint>
#include <limits>

#include "Ballot.h"
#include "Ballots.h"
#include "VectorUtils.h"

namespace reconvergence {

typedef std::bitset<128> bitset_inv_t;

inline Ballot waveSizeToMask(uint32_t waveSize, uint32_t /*waveCount*/) {
  Ballot b;
  assert(waveSize <= b.size());
  for (uint32_t i = 0; i < waveSize; ++i)
    b.set(i);
  return b;
}

inline Ballots ballotsFromU64(uint64_t maskValue, uint32_t waveSize,
                              uint32_t waveCount) {
  Ballot b(maskValue);
  b &= waveSizeToMask(waveSize, waveCount);
  Ballots result(waveCount);
  for (uint32_t g = 0; g < waveCount; ++g)
    result.at(g) = b;
  return result;
}

inline Ballots ballotsFromBallot(Ballot b, uint32_t waveSize,
                                 uint32_t waveCount) {
  b &= waveSizeToMask(waveSize, waveCount);
  Ballots result(waveCount);
  for (uint32_t g = 0; g < waveCount; ++g)
    result.at(g) = b;
  return result;
}

// Pick out the mask for the wave that invocationID is a member of
inline Ballot bitsetToBallot(const Ballots &bitset, uint32_t waveSize,
                             uint32_t invocationID) {
  return bitset.at(invocationID / waveSize) &
         waveSizeToMask(waveSize, bitset.waveCount());
}

inline Ballot bitsetToBallot(uint64_t value, uint32_t waveCount,
                             uint32_t waveSize, uint32_t invocationID) {
  Ballots bs = ballotsFromU64(value, waveSize, waveCount);
  return bitsetToBallot(bs, waveSize, invocationID);
}

template <uint32_t N> static uint32_t findLSB(const std::bitset<N> &value) {
  for (uint32_t i = 0u; i < N; ++i) {
    if (value.test(i))
      return i;
  }
  return std::numeric_limits<uint32_t>::max();
}

inline Ballots bitsetElect(const Ballots &value) {
  Ballots ret(value.waveCount());
  for (uint32_t g = 0u; g < value.waveCount(); ++g) {
    const uint32_t lsb = findLSB<Ballots::waveInvocationSize>(value.at(g));
    if (lsb != std::numeric_limits<uint32_t>::max()) {
      ret.at(g).set(lsb);
    }
  }
  return ret;
}

} // namespace reconvergence

#endif // MASKUTILS_H_
