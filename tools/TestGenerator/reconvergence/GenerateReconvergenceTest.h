//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "TestCase.h"

namespace reconvergence {

enum class TestExpectation {
  PASS,
  FAIL,
};

struct TestExpectationConfig {
  TestExpectation Expectation;
  std::string Condition;
  std::string Comment;
};

class ReconvergenceTestGenerator {
public:
  ReconvergenceTestGenerator(
      const std::string &OutputDir,
      const std::map<std::string, std::vector<TestExpectationConfig>>
          &TestExpectations = {})
      : OutputDir(OutputDir), TestExpectations(TestExpectations) {}
  void createRandomizedTests(uint32_t TotalSeedGroup,
                             const std::vector<uint32_t> &TestsCountPerLevel,
                             uint32_t WaveSize, uint32_t ThreadgroupSizeX,
                             uint32_t ThreadgroupSizeY);

  bool SaveShader = false;

private:
  std::string OutputDir;
  std::map<std::string, std::vector<TestExpectationConfig>> TestExpectations;
  TestCase createSingleTest(uint32_t Seed, uint32_t MaxNestingLevel,
                            uint32_t WaveSize, uint32_t ThreadgroupSizeX,
                            uint32_t ThreadgroupSizeY);

  void saveShader(const TestCase &Test);
  void saveTestConfig(const TestCase &Test);
};
} // namespace reconvergence
