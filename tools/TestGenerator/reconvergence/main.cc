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
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"

using namespace llvm;

namespace {

struct FailedTestYamlEntry {
  std::string Name;
  std::string Expectation;
  std::string Condition;
  std::string Comment;
};

struct FailedTestsYamlDocument {
  std::vector<FailedTestYamlEntry> Tests;
};

} // namespace

LLVM_YAML_IS_SEQUENCE_VECTOR(FailedTestYamlEntry)

namespace llvm::yaml {

template <> struct MappingTraits<FailedTestYamlEntry> {
  static void mapping(IO &Io, FailedTestYamlEntry &Entry) {
    Io.mapRequired("name", Entry.Name);
    Io.mapRequired("expectation", Entry.Expectation);
    Io.mapOptional("condition", Entry.Condition, std::string());
    Io.mapOptional("comment", Entry.Comment, std::string());
  }
};

template <> struct MappingTraits<FailedTestsYamlDocument> {
  static void mapping(IO &Io, FailedTestsYamlDocument &Document) {
    Io.mapRequired("tests", Document.Tests);
  }
};

} // namespace llvm::yaml

static std::map<std::string, std::vector<reconvergence::TestExpectationConfig>>
loadFailedTestsYaml(const std::string &FailedTestsYamlPath) {
  std::map<std::string, std::vector<reconvergence::TestExpectationConfig>>
      ParsedTests;
  if (FailedTestsYamlPath.empty()) {
    return ParsedTests;
  }

  auto BufferOrError = MemoryBuffer::getFile(FailedTestsYamlPath);
  if (!BufferOrError) {
    errs() << "Error: Could not read failed-tests YAML file at '"
           << FailedTestsYamlPath << "'.\n";
    return ParsedTests;
  }

  yaml::Input Yin(BufferOrError.get()->getBuffer());
  FailedTestsYamlDocument Document;
  Yin >> Document;
  if (Yin.error()) {
    errs() << "Error: Failed to parse failed-tests YAML file at '"
           << FailedTestsYamlPath << "'.\n";
    return ParsedTests;
  }

  for (const auto &Entry : Document.Tests) {
    if (Entry.Name.empty()) {
      continue;
    }

    const std::string UpperExpectation = StringRef(Entry.Expectation).upper();
    const reconvergence::TestExpectation ParsedExpectation =
        UpperExpectation == "PASS" ? reconvergence::TestExpectation::PASS
                                   : reconvergence::TestExpectation::FAIL;

    ParsedTests[Entry.Name].push_back(reconvergence::TestExpectationConfig{
        ParsedExpectation, Entry.Condition, Entry.Comment});
  }

  return ParsedTests;
}

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

static cl::opt<std::string>
    FailedTestsYamlPath("failed_tests_yaml",
                        cl::desc("Path to failed-tests YAML metadata"),
                        cl::init(""));

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
  const auto TestExpectations = loadFailedTestsYaml(FailedTestsYamlPath);
  reconvergence::ReconvergenceTestGenerator TestGenerator(OutputDir,
                                                          TestExpectations);

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
