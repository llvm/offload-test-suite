#include <iostream>
#include <map>

#include "GenerateReconvergenceTest.h"

int main(int argc, char *argv[]) {
  std::cout << "Starting test generation..." << std::endl;
  reconvergence::ReconvergenceTestGenerator test_generator;
  // std::map<uint32_t, uint32_t> nestingLevelToTestsCount = {
  //     {2, 250}, {3, 250}, {4, 250}, {5, 100}, {6, 50}};
  std::map<uint32_t, uint32_t> nestingLevelToTestsCount = {
      {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}};
  std::vector<uint32_t> subgroupSizes = {4, 8, 16, 32};
  for (uint32_t subgroupSize : subgroupSizes) {
    test_generator.createRandomizedTests(
        /*maxNestingLevel=*/6,
        /*totalSeedGroup=*/8, nestingLevelToTestsCount, subgroupSize,
        /*workgroupSizeX=*/7,
        /*workgroupSizeY=*/13);
  }
  return 0;
}
