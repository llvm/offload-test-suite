//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
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
                            std::vector<uint64_t> & /*ref*/) override {
    return 0;
  }

  struct ComputePrerequisites : Prerequisites {
    const uint32_t m_waveSize;
    ComputePrerequisites(uint32_t waveSize) : m_waveSize(waveSize) {}
  };

  virtual void printBallotHlsl(std::stringstream &css, const FlowState &,
                               bool endWithSemicolon = false) override {
    printIndent(css);

    // When inside loop(s), use partitionBallot rather than WaveActiveBallot to
    // compute a ballot, to make sure the ballot is "diverged enough". Don't do
    // this for subgroup_uniform_control_flow, since we only validate results
    // that must be fully reconverged.
    if (loopNesting > 0) {
      css << "OutputB[gIndex][outLoc++] = " << getPartitionBallotTextHlsl();
    } else {
      css << "OutputB[gIndex][outLoc++] = "
             "WaveActiveBallot(true)";
    }
    if (endWithSemicolon) {
      css << ";\n";
    }
  }

protected:
  virtual void simulateBallot(const Ballots &activeMask,
                              const uint32_t /*unusedPrimitiveID*/,
                              const int32_t opsIndex,
                              std::vector<uint32_t> &outLoc,
                              std::vector<std::vector<UVec4>> &ref,
                              std::shared_ptr<Prerequisites> prerequisites,
                              uint32_t & /*logFailureCount*/,
                              const OPType /*reason*/,
                              const UVec4 * /*cmp*/) override {
    const uint32_t waveCount = activeMask.waveCount();
    const uint32_t waveSize =
        std::static_pointer_cast<ComputePrerequisites>(prerequisites)
            ->m_waveSize;

    for (uint32_t id = 0; id < invocationStride; ++id) {
      if (activeMask.test((Ballots::findBit(id, waveSize)))) {
        if (ops[opsIndex].caseValue) {
          // Emit a magic value to indicate that we shouldn't validate this
          // ballot
          ref[id].push_back(
              bitsetToBallot(0x12345678, waveCount, waveSize, id));
        } else {
          ref[id].push_back(bitsetToBallot(activeMask, waveSize, id));
        }
        outLoc[id]++;
      }
    }
  }

  virtual void
  simulateStore(const Ballots &activeMask, const uint32_t /*unusedPrimitiveID*/,
                const uint64_t storeValue, std::vector<uint32_t> &outLoc,
                std::vector<std::vector<UVec4>> &ref,
                std::shared_ptr<Prerequisites> prerequisites,
                uint32_t & /*logFailureCount*/, const OPType /*reason*/,
                const UVec4 * /*cmp*/) override {
    const uint32_t waveSize =
        std::static_pointer_cast<ComputePrerequisites>(prerequisites)
            ->m_waveSize;
    for (uint32_t id = 0; id < invocationStride; ++id) {
      if (activeMask.test(Ballots::findBit(id, waveSize))) {
        ref[id].push_back(Ballot(
            UVec4(static_cast<uint32_t>(storeValue & 0xFFFFFFFF), 0u, 0u, 0u)));
        outLoc[id]++;
      }
    }
  }

  virtual std::shared_ptr<Prerequisites> /*outputP*/
  makePrerequisites(const std::vector<uint32_t> & /*outputP*/,
                    const uint32_t waveSize, const uint32_t primitiveStride,
                    std::vector<WaveState> &stateStack,
                    std::vector<uint32_t> &outLoc,
                    uint32_t &waveCount) override {
    auto prerequisites = std::make_shared<ComputePrerequisites>(waveSize);
    waveCount = ROUNDUP(invocationStride, waveSize) / waveSize;
    stateStack.resize(10u, WaveState(waveCount));
    outLoc.resize(primitiveStride, 0u);
    Ballots &activeMask(stateStack.at(0).activeMask);
    for (uint32_t id = 0; id < invocationStride; ++id) {
      activeMask.set(Ballots::findBit(id, waveSize));
    }
    return prerequisites;
  }
};

} // namespace reconvergence

#endif // COMPUTERANDOMPROGRAM_H_
