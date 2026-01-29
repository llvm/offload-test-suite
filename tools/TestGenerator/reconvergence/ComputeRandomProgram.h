#ifndef EXPERIMENTAL_USERS_CLUCIE_COMPUTERANDOMPROGRAM_H_
#define EXPERIMENTAL_USERS_CLUCIE_COMPUTERANDOMPROGRAM_H_

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
      : RandomProgram(testCase, testCase.getWorkgroupSizeX() *
                                    testCase.getWorkgroupSizeY()) {}
  virtual ~ComputeRandomProgram() = default;

  virtual uint32_t simulate(bool countOnly, uint32_t subgroupSize,
                            add_ref<std::vector<uint64_t>> ref) override {
    return 0;
  }

  struct ComputePrerequisites : Prerequisites {
    const uint32_t m_subgroupSize;
    ComputePrerequisites(uint32_t subgroupSize)
        : m_subgroupSize(subgroupSize) {}
  };

  virtual void printBallotHlsl(add_ref<std::stringstream> css,
                               add_cref<FlowState>,
                               bool endWithSemicolon = false) override {
    printIndent(css);

    // When inside loop(s), use partitionBallot rather than subgroupBallot to
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
  virtual void simulateBallot(const bool countOnly,
                              add_cref<Ballots> activeMask,
                              const uint32_t unusedPrimitiveID,
                              const int32_t opsIndex,
                              add_ref<std::vector<uint32_t>> outLoc,
                              add_ref<std::vector<UVec4>> ref,
                              std::shared_ptr<Prerequisites> prerequisites,
                              add_ref<uint32_t> logFailureCount,
                              const OPType reason, const UVec4 *cmp) override {
    const uint32_t subgroupCount = activeMask.subgroupCount();
    const uint32_t subgroupSize =
        std::static_pointer_cast<ComputePrerequisites>(prerequisites)
            ->m_subgroupSize;

    for (uint32_t id = 0; id < invocationStride; ++id) {
      if (activeMask.test((Ballots::findBit(id, subgroupSize)))) {
        if (countOnly) {
          outLoc[id]++;
        } else {
          if (ops[opsIndex].caseValue) {
            // Emit a magic value to indicate that we shouldn't validate this
            // ballot
            ref[(outLoc[id]++) * invocationStride + id] =
                bitsetToBallot(0x12345678, subgroupCount, subgroupSize, id);
          } else {
            ref[(outLoc[id]++) * invocationStride + id] =
                bitsetToBallot(activeMask, subgroupSize, id);
          }
        }
      }
    }
  }

  virtual void simulateStore(const bool countOnly, add_cref<Ballots> activeMask,
                             const uint32_t unusedPrimitiveID,
                             const uint64_t storeValue,
                             add_ref<std::vector<uint32_t>> outLoc,
                             add_ref<std::vector<UVec4>> ref,
                             std::shared_ptr<Prerequisites> prerequisites,
                             add_ref<uint32_t> logFailureCount,
                             const OPType reason, const UVec4 *cmp) override {
    const uint32_t subgroupSize =
        std::static_pointer_cast<ComputePrerequisites>(prerequisites)
            ->m_subgroupSize;
    for (uint32_t id = 0; id < invocationStride; ++id) {
      if (activeMask.test(Ballots::findBit(id, subgroupSize))) {
        if (countOnly)
          outLoc[id]++;
        else
          ref[(outLoc[id]++) * invocationStride + id] = Ballot(UVec4(
              static_cast<uint32_t>(storeValue & 0xFFFFFFFF), 0u, 0u, 0u));
      }
    }
  }

  virtual std::shared_ptr<Prerequisites>
  makePrerequisites(add_cref<std::vector<uint32_t>> outputP,
                    const uint32_t subgroupSize, const uint32_t primitiveStride,
                    add_ref<std::vector<SubgroupState2>> stateStack,
                    add_ref<std::vector<uint32_t>> outLoc,
                    add_ref<uint32_t> subgroupCount) override {
    auto prerequisites = std::make_shared<ComputePrerequisites>(subgroupSize);
    subgroupCount = ROUNDUP(invocationStride, subgroupSize) / subgroupSize;
    stateStack.resize(10u, SubgroupState2(subgroupCount));
    outLoc.resize(primitiveStride, 0u);
    add_ref<Ballots> activeMask(stateStack.at(0).activeMask);
    for (uint32_t id = 0; id < invocationStride; ++id) {
      activeMask.set(Ballots::findBit(id, subgroupSize));
    }
    return prerequisites;
  }
};

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_COMPUTERANDOMPROGRAM_H_
