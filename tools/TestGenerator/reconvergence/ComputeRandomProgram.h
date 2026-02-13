#ifndef COMPUTERANDOMPROGRAM_H_
#define COMPUTERANDOMPROGRAM_H_

#include <cstdint>
#include <memory>
#include <sstream>
#include <vector>

#include "Ballot.h"
#include "Ballots.h"
#include "MaskUtils.h"
#include "MathUtils.h"
#include "RandomProgram.h"
#include "TestCase.h"
#include "VectorUtils.h"

namespace reconvergence {

class ComputeRandomProgram : public RandomProgram {
public:
  ComputeRandomProgram(const TestCase &testCase)
      : RandomProgram(testCase, testCase.getThreadgroupSizeX() *
                                    testCase.getThreadgroupSizeY()) {}
  virtual ~ComputeRandomProgram() = default;

  virtual uint32_t simulate(bool /*countOnly*/, uint32_t /*waveSize*/,
                            add_ref<std::vector<uint64_t>> /*ref*/) override {
    return 0;
  }

  struct ComputePrerequisites : Prerequisites {
    const uint32_t m_waveSize;
    ComputePrerequisites(uint32_t waveSize) : m_waveSize(waveSize) {}
  };

  virtual void printBallotHlsl(add_ref<std::stringstream> css,
                               add_cref<FlowState>,
                               bool endWithSemicolon = false) override {
    printIndent(css);

    // When inside loop(s), use partitionBallot rather than WaveActiveBallot to
    // compute a ballot, to make sure the ballot is "diverged enough". Don't do
    // this for subgroup_uniform_control_flow, since we only validate results
    // that must be fully reconverged.
    if (loopNesting > 0) {
      css << "OutputB[(outLoc++)*invocationStride + gIndex] = "
          << getPartitionBallotTextHlsl();
    } else {
      css << "OutputB[(outLoc++)*invocationStride + gIndex] = "
             "WaveActiveBallot(true)";
    }
    if (endWithSemicolon) {
      css << ";\n";
    }
  }

protected:
  virtual void
  simulateBallot(const bool countOnly, add_cref<Ballots> activeMask,
                 const uint32_t /*unusedPrimitiveID*/, const int32_t opsIndex,
                 add_ref<std::vector<uint32_t>> outLoc,
                 add_ref<std::vector<UVec4>> ref,
                 std::shared_ptr<Prerequisites> prerequisites,
                 add_ref<uint32_t> /*logFailureCount*/, const OPType /*reason*/,
                 const UVec4 * /*cmp*/) override {
    const uint32_t waveCount = activeMask.waveCount();
    const uint32_t waveSize =
        std::static_pointer_cast<ComputePrerequisites>(prerequisites)
            ->m_waveSize;

    for (uint32_t id = 0; id < invocationStride; ++id) {
      if (activeMask.test((Ballots::findBit(id, waveSize)))) {
        if (countOnly) {
          outLoc[id]++;
        } else {
          if (ops[opsIndex].caseValue) {
            // Emit a magic value to indicate that we shouldn't validate this
            // ballot
            ref[(outLoc[id]++) * invocationStride + id] =
                bitsetToBallot(0x12345678, waveCount, waveSize, id);
          } else {
            ref[(outLoc[id]++) * invocationStride + id] =
                bitsetToBallot(activeMask, waveSize, id);
          }
        }
      }
    }
  }

  virtual void simulateStore(const bool countOnly, add_cref<Ballots> activeMask,
                             const uint32_t /*unusedPrimitiveID*/,
                             const uint64_t storeValue,
                             add_ref<std::vector<uint32_t>> outLoc,
                             add_ref<std::vector<UVec4>> ref,
                             std::shared_ptr<Prerequisites> prerequisites,
                             add_ref<uint32_t> /*logFailureCount*/,
                             const OPType /*reason*/,
                             const UVec4 * /*cmp*/) override {
    const uint32_t waveSize =
        std::static_pointer_cast<ComputePrerequisites>(prerequisites)
            ->m_waveSize;
    for (uint32_t id = 0; id < invocationStride; ++id) {
      if (activeMask.test(Ballots::findBit(id, waveSize))) {
        if (countOnly)
          outLoc[id]++;
        else
          ref[(outLoc[id]++) * invocationStride + id] = Ballot(UVec4(
              static_cast<uint32_t>(storeValue & 0xFFFFFFFF), 0u, 0u, 0u));
      }
    }
  }

  virtual std::shared_ptr<Prerequisites> /*outputP*/
  makePrerequisites(add_cref<std::vector<uint32_t>> outputP,
                    const uint32_t waveSize, const uint32_t primitiveStride,
                    add_ref<std::vector<WaveState>> stateStack,
                    add_ref<std::vector<uint32_t>> outLoc,
                    add_ref<uint32_t> waveCount) override {
    auto prerequisites = std::make_shared<ComputePrerequisites>(waveSize);
    waveCount = ROUNDUP(invocationStride, waveSize) / waveSize;
    stateStack.resize(10u, WaveState(waveCount));
    outLoc.resize(primitiveStride, 0u);
    add_ref<Ballots> activeMask(stateStack.at(0).activeMask);
    for (uint32_t id = 0; id < invocationStride; ++id) {
      activeMask.set(Ballots::findBit(id, waveSize));
    }
    return prerequisites;
  }
};

} // namespace reconvergence

#endif // COMPUTERANDOMPROGRAM_H_
