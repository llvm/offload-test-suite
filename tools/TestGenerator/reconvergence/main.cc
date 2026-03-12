//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "GenerateReconvergenceTest.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::opt<std::string>
    OutputDir("output_dir", cl::desc("Output directory for generated tests"),
              cl::Required);

static cl::opt<std::string> CountsPerLevel(
    "counts_per_level",
    cl::desc("Single integer (applied to all) or comma-separated list of test "
             "counts per nesting level"),
    cl::Required);

static cl::opt<unsigned> MaxNestingLevel("max_nesting_level",
                                         cl::desc("Maximum nesting level"),
                                         cl::init(6));

static cl::opt<unsigned> TotalSeedGroup("total_seed_group",
                                        cl::desc("Total seed group count"),
                                        cl::init(8));

int main(int argc, char *argv[]) {
  cl::ParseCommandLineOptions(argc, argv, "Reconvergence Test Generator\n");

  if (MaxNestingLevel < 2) {
    std::cerr << "Error: MaxNestingLevel must be >= 2" << std::endl;
    return 1;
  }

  std::vector<uint32_t> Counts;
  std::stringstream CountsStream(CountsPerLevel);
  std::string CountItem;
  while (std::getline(CountsStream, CountItem, ',')) {
    Counts.push_back(std::stoi(CountItem));
  }

  std::map<uint32_t, uint32_t> NestingLevelToTestsCount;
  if (Counts.size() == 1) {
    for (uint32_t Level = 2; Level <= MaxNestingLevel; ++Level) {
      NestingLevelToTestsCount[Level] = Counts[0];
    }
  } else {
    // Expect one count per level from 2 to MaxNestingLevel
    const size_t Expected = MaxNestingLevel - 1;
    if (Counts.size() != Expected) {
      std::cerr << "Error: Expected " << Expected << " counts (for levels 2.."
                << MaxNestingLevel << "), got " << Counts.size() << std::endl;
      return 1;
    }
    for (size_t CountIndex = 0; CountIndex < Counts.size(); ++CountIndex) {
      NestingLevelToTestsCount[static_cast<uint32_t>(CountIndex) + 2] =
          Counts[CountIndex];
    }
  }

  std::cout << "Starting test generation into " << OutputDir.getValue() << "..."
            << std::endl;
  reconvergence::ReconvergenceTestGenerator TestGenerator(OutputDir);

  // Wave size must be less than or equal to 128 with current ballot
  // implementation.
  // AMD: 64
  // NVIDIA: 32
  // Intel: 16
  // WARP: 4
  // M1: 32
  // Llvmpipe: 8
  const std::vector<uint32_t> WaveSizes = {4, 8, 16, 32, 64};
  for (const uint32_t WaveSize : WaveSizes) {
    TestGenerator.createRandomizedTests(MaxNestingLevel, TotalSeedGroup,
                                        NestingLevelToTestsCount, WaveSize,
                                        /*ThreadgroupSizeX=*/7,
                                        /*ThreadgroupSizeY=*/13);
  }
  return 0;
}
