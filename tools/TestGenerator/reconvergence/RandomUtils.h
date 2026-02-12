#ifndef EXPERIMENTAL_USERS_CLUCIE_RANDOMUTILS_H_
#define EXPERIMENTAL_USERS_CLUCIE_RANDOMUTILS_H_

#include <cstdint>
#include <memory>
#include "llvm/Support/RandomNumberGenerator.h"

namespace llvm {
class Module;
class LLVMContext;
} // namespace llvm

namespace reconvergence {

struct Random {
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;

  Random();
  ~Random();
  Random(Random&&) = default;
  Random& operator=(Random&&) = default;
};

void Random_init(Random *rnd, uint32_t seed);
uint32_t Random_getUint32(Random *rnd);
uint64_t Random_getUint64(Random *rnd);
float Random_getFloat(Random *rnd);
double Random_getDouble(Random *rnd);
bool Random_getBool(Random *rnd);

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_RANDOMUTILS_H_
