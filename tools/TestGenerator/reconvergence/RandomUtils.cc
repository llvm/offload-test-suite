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
// \param Rnd    RNG to initialize.
// \param Seed    Seed value used for random values.
//--------------------------------------------------------------------

Random::Random() = default;
Random::~Random() = default;

void Random_init(Random *Rnd, uint32_t Seed) {
  // We need a temporary Context and Module to create the RNG.
  // The RNG is self-contained and doesn't depend on them after creation.
  llvm::LLVMContext Context;
  // Use the seed as the module name/salt to ensure deterministic behavior per
  // seed
  const llvm::Module Module(std::to_string(Seed), Context);

  // createRNG uses the module ID as salt mixed with global seed.
  Rnd->RNG = Module.createRNG("reconvergence-test-generator");
}

//--------------------------------------------------------------------
// \brief Get a pseudo random uint32.
// \param Rnd    Pointer to RNG.
// \return Random uint32 number.
//--------------------------------------------------------------------
uint32_t Random_getUint32(Random *Rnd) {
  // llvm::RandomNumberGenerator returns uint64_t
  // Just casting it to uint32_t.
  return static_cast<uint32_t>((*Rnd->RNG)());
}

//--------------------------------------------------------------------
// \brief Get a pseudo random uint64.
// \param Rnd    Pointer to RNG.
// \return Random uint64 number.
//--------------------------------------------------------------------
uint64_t Random_getUint64(Random *Rnd) { return (*Rnd->RNG)(); }

} // namespace reconvergence
