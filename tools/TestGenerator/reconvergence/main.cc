#include <iostream>
#include <map>

#include "GenerateReconvergenceTest.h"

int main(int /*argc*/, char * /*argv*/[]) {
  std::cout << "Starting test generation..." << std::endl;
  reconvergence::ReconvergenceTestGenerator TestGenerator;
  const std::map<uint32_t, uint32_t> NestingLevelToTestsCount = {
      {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}};
  const std::vector<uint32_t> WaveSizes = {4, 8, 16, 32};
  for (const uint32_t WaveSize : WaveSizes) {
    TestGenerator.createRandomizedTests(
        /*TotalMaxNestingLevel=*/6,
        /*TotalSeedGroup=*/8, NestingLevelToTestsCount, WaveSize,
        /*ThreadgroupSizeX=*/7,
        /*ThreadgroupSizeY=*/13);
  }
  return 0;
}
