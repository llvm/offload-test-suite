#include "RandomUtils.h"

#include <cmath>
#include <limits>
#include <string>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include <cassert>

namespace reconvergence {

//--------------------------------------------------------------------
// \brief Initialize a random number generator with a given seed.
// \param rnd    RNG to initialize.
// \param seed    Seed value used for random values.
//--------------------------------------------------------------------

Random::Random() = default;
Random::~Random() = default;

void Random_init(Random *rnd, uint32_t seed) {
  // We need a temporary Context and Module to create the RNG.
  // The RNG is self-contained and doesn't depend on them after creation.
  llvm::LLVMContext Context;
  // Use the seed as the module name/salt to ensure deterministic behavior per
  // seed
  llvm::Module Module(std::to_string(seed), Context);

  // createRNG uses the module ID as salt mixed with global seed.
  rnd->RNG = Module.createRNG("reconvergence-test-generator");
}

//--------------------------------------------------------------------
// \brief Get a pseudo random uint32.
// \param rnd    Pointer to RNG.
// \return Random uint32 number.
//--------------------------------------------------------------------
uint32_t Random_getUint32(Random *rnd) {
  // llvm::RandomNumberGenerator returns uint64_t
  // Just casting it to uint32_t.
  return static_cast<uint32_t>((*rnd->RNG)());
}

//--------------------------------------------------------------------
// \brief Get a pseudo random uint64.
// \param rnd    Pointer to RNG.
// \return Random uint64 number.
//--------------------------------------------------------------------
uint64_t Random_getUint64(Random *rnd) { return (*rnd->RNG)(); }

//--------------------------------------------------------------------
// \brief Get a pseudo random float in range [0, 1].
// \param rnd    Pointer to RNG.
// \return Random float number.
//--------------------------------------------------------------------
float Random_getFloat(Random *rnd) {
  uint64_t val = (*rnd->RNG)();
  return static_cast<float>(val) /
         static_cast<float>(llvm::RandomNumberGenerator::max());
}

//--------------------------------------------------------------------
// \brief Get a pseudo random double in range [0, 1].
// \param rnd    Pointer to RNG.
// \return Random double number.
//--------------------------------------------------------------------
double Random_getDouble(Random *rnd) {
  uint64_t val = (*rnd->RNG)();
  return static_cast<double>(val) /
         static_cast<double>(llvm::RandomNumberGenerator::max());
}

//--------------------------------------------------------------------
// \brief Get a pseudo random boolean value (false or true).
// \param rnd    Pointer to RNG.
// \return Random boolean value.
//--------------------------------------------------------------------
bool Random_getBool(Random *rnd) { return ((*rnd->RNG)() % 2) != 0; }

} // namespace reconvergence
