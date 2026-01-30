#ifndef EXPERIMENTAL_USERS_CLUCIE_RANDOMUTILS_H_
#define EXPERIMENTAL_USERS_CLUCIE_RANDOMUTILS_H_

#include <cstdint>

namespace reconvergence {

typedef struct deRandom_s {
  uint32_t x; /*!< Current random state. */
  uint32_t y;
  uint32_t z;
  uint32_t w;
} deRandom;

void deRandom_init(deRandom *rnd, uint32_t seed);
uint32_t deRandom_getUint32(deRandom *rnd);
uint64_t deRandom_getUint64(deRandom *rnd);
float deRandom_getFloat(deRandom *rnd);
double deRandom_getDouble(deRandom *rnd);
bool deRandom_getBool(deRandom *rnd);

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_RANDOMUTILS_H_
