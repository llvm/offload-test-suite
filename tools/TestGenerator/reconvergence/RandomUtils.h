#ifndef RANDOMUTILS_H_
#define RANDOMUTILS_H_

#include "llvm/Support/RandomNumberGenerator.h"
#include <cstdint>
#include <memory>

namespace llvm {
class Module;
class LLVMContext;
} // namespace llvm

namespace reconvergence {

struct Random {
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;

  Random();
  ~Random();
  Random(Random &&) = default;
  Random &operator=(Random &&) = default;
};

void Random_init(Random *Rnd, uint32_t Seed);
uint32_t Random_getUint32(Random *Rnd);
uint64_t Random_getUint64(Random *Rnd);

} // namespace reconvergence

#endif // RANDOMUTILS_H_
