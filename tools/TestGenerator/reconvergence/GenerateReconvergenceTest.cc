//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "GenerateReconvergenceTest.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "ComputeRandomProgram.h"
#include "RandomProgram.h"
#include "TestCase.h"
#include "VectorUtils.h"

namespace reconvergence {
void ReconvergenceTestGenerator::createRandomizedTests(
    uint32_t TotalMaxNestingLevel, uint32_t TotalSeedGroup,
    const std::map<uint32_t, uint32_t> &NestingLevelToTestsCount,
    uint32_t WaveSize, uint32_t ThreadgroupSizeX, uint32_t ThreadgroupSizeY) {
  uint32_t TotalTestsCount = 0;
  for (uint32_t MaxNestingLevel = 2; MaxNestingLevel <= TotalMaxNestingLevel;
       ++MaxNestingLevel) {
    const uint32_t TestsCount = NestingLevelToTestsCount.at(MaxNestingLevel);
    TotalTestsCount += TestsCount * TotalSeedGroup;
  }

  uint32_t TestId = 0;
  for (uint32_t MaxNestingLevel = 2; MaxNestingLevel <= TotalMaxNestingLevel;
       ++MaxNestingLevel) {
    uint32_t Seed = 0;
    for (uint32_t SeedGroup = 0; SeedGroup < TotalSeedGroup; ++SeedGroup) {
      const uint32_t TestsCount = NestingLevelToTestsCount.at(MaxNestingLevel);
      for (uint32_t _ = 0; _ < TestsCount; ++_) {
        const TestCase &Test =
            createSingleTest(Seed, MaxNestingLevel, WaveSize, ThreadgroupSizeX,
                             ThreadgroupSizeY);
        if (SaveShader) {
          saveShader(Test);
        }
        saveTestConfig(Test);

        TestId++;
        Seed++;
        std::cout << "\rShader with wave size " << WaveSize << " (" << TestId
                  << " / " << TotalTestsCount << ") generated successfully."
                  << std::flush;
      }
    }
  }
}

TestCase ReconvergenceTestGenerator::createSingleTest(
    uint32_t Seed, uint32_t MaxNestingLevel, uint32_t WaveSize,
    uint32_t ThreadgroupSizeX, uint32_t ThreadgroupSizeY) {
  TestCase TestCase(Seed, MaxNestingLevel, WaveSize, ThreadgroupSizeX,
                    ThreadgroupSizeY);
  ComputeRandomProgram Program(TestCase);

  Program.generateRandomProgram();

  const uint32_t InvocationStride = ThreadgroupSizeX * ThreadgroupSizeY;
  std::vector<std::vector<UVec4>> Ref(InvocationStride);
  Program.execute(WaveSize, InvocationStride, Ref);
  TestCase.setExpectedResult(Ref);

  std::stringstream Functions, Main;
  Program.printCodeHlsl(Functions, Main);

  std::stringstream Header, Layout, Globals;

  Header << "#define THREADS_X " << ThreadgroupSizeX << "\n";
  Header << "#define THREADS_Y " << ThreadgroupSizeY << "\n\n";

  Layout << "StructuredBuffer<uint> InputA : register(t0);\n";
  Layout << "RWStructuredBuffer<uint4> OutputB[" << InvocationStride
         << "] : register(u1);\n\n";

  Globals << "bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> "
             "(bit % 32)) & 1) != 0; }\n";

  Globals << "static int outLoc = 0;\n";

  std::stringstream &Shader = TestCase.getShader();
  Shader << Header.str();
  Shader << Layout.str();
  Shader << Globals.str();

  Shader << Functions.str() << "\n";

  std::stringstream Setup;

  Setup << "[numthreads(THREADS_X, THREADS_Y, 1)]\n";
  Setup << "void main(uint gIndex : SV_GroupIndex)\n" << "{\n";
  Shader << Setup.str() << Main.str() << "}\n";
  return TestCase;
}

void ReconvergenceTestGenerator::saveShader(const TestCase &Test) {
  const uint32_t WaveSize = Test.getWaveSize();
  const uint32_t ThreadgroupSizeX = Test.getThreadgroupSizeX();
  const uint32_t ThreadgroupSizeY = Test.getThreadgroupSizeY();
  const uint32_t MaxNestingLevel = Test.getMaxNestingLevel();
  const uint32_t Seed = Test.getSeed();
  const std::string Path = OutputDir + "/shaders/" + std::to_string(WaveSize);
  std::filesystem::create_directories(Path);
  const std::string Filename =
      Path + "/test_" + std::to_string(MaxNestingLevel) + "_" +
      std::to_string(WaveSize) + "_" + std::to_string(ThreadgroupSizeX) + "_" +
      std::to_string(ThreadgroupSizeY) + "_" + std::to_string(Seed) + ".hlsl";
  std::ofstream Ofs(Filename);
  Ofs << Test.getShaderString() << std::endl;
  Ofs.close();
}

void ReconvergenceTestGenerator::saveTestConfig(const TestCase &Test) {
  const uint32_t WaveSize = Test.getWaveSize();
  const uint32_t ThreadgroupSizeX = Test.getThreadgroupSizeX();
  const uint32_t ThreadgroupSizeY = Test.getThreadgroupSizeY();
  const uint32_t MaxNestingLevel = Test.getMaxNestingLevel();
  const uint32_t Seed = Test.getSeed();
  const std::string Path = OutputDir + "/tests/" + std::to_string(WaveSize);
  std::filesystem::create_directories(Path);
  const std::string TestName =
      "test_" + std::to_string(MaxNestingLevel) + "_" +
      std::to_string(WaveSize) + "_" + std::to_string(ThreadgroupSizeX) + "_" +
      std::to_string(ThreadgroupSizeY) + "_" + std::to_string(Seed) + ".test";
  const std::string Filename = Path + "/" + TestName;
  std::ofstream Ofs(Filename);

  Ofs << "#--- source.hlsl\n" << std::endl;
  // Add debugging info as comments at the top of the shader.
  Ofs << "// --- Wave size: " << WaveSize << std::endl;
  Ofs << "// --- Seed: " << Seed << std::endl;
  Ofs << "// --- NestingLevel: " << MaxNestingLevel << std::endl;

  Ofs << Test.getShaderString() << std::endl;
  Ofs << "//--- pipeline.yaml\n" << std::endl;
  Ofs << "---\n" << std::endl;

  // Configuration for the pipeline.
  std::stringstream Shaders;
  Shaders <<
      R""""(Shaders:
  - Stage: Compute
    Entry: main
    DispatchSize: [1, 1, 1]
)"""";

  std::stringstream Buffers;
  Buffers <<
      R""""(Buffers:
  - Name: InputA
    Format: UInt32
    Data: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90 ]
)"""";

  const uint32_t InvocationStride = ThreadgroupSizeX * ThreadgroupSizeY;
  const auto &ExpectedResult = Test.getExpectedResult();

  for (uint32_t InvocationIndex = 0; InvocationIndex < InvocationStride;
       ++InvocationIndex) {
    const size_t NumElements = ExpectedResult[InvocationIndex].size();
    const size_t AllocatedElements = NumElements == 0 ? 1 : NumElements;

    Buffers << "  - Name: OutputB_" << InvocationIndex << "\n";
    Buffers << "    Format: UInt32\n";
    Buffers << "    Stride: 16\n";
    Buffers << "    FillSize: " << AllocatedElements * 16 << "\n";

    Buffers << "  - Name: ExpectedOutputB_" << InvocationIndex << "\n";
    Buffers << "    Format: UInt32\n";
    Buffers << "    Stride: 16\n";
    if (NumElements == 0) {
      // Allocate one zero element so descriptor bindings are valid for all
      // UAVs.
      Buffers << "    Data: [ 0, 0, 0, 0 ]\n";
    } else {
      Buffers << "    Data: [ ";
      for (size_t ElementIndex = 0; ElementIndex < NumElements;
           ++ElementIndex) {
        const UVec4 &Vec = ExpectedResult[InvocationIndex][ElementIndex];
        Buffers << Vec.x() << ", " << Vec.y() << ", " << Vec.z() << ", "
                << Vec.w();
        if (ElementIndex < NumElements - 1) {
          Buffers << ", ";
        }
      }
      Buffers << " ]\n";
    }
  }

  std::stringstream Results;
  Results << "Results:\n";
  for (uint32_t InvocationIndex = 0; InvocationIndex < InvocationStride;
       ++InvocationIndex) {
    Results << "  - Result: Test_" << InvocationIndex << "\n";
    Results << "    Rule: BufferExact\n";
    Results << "    Actual: OutputB_" << InvocationIndex << "\n";
    Results << "    Expected: ExpectedOutputB_" << InvocationIndex << "\n";
  }

  std::stringstream DescriptorSets;
  DescriptorSets << R"""(DescriptorSets:
  - Resources:
    - Name: InputA
      Kind: StructuredBuffer
      DirectXBinding:
        Register: 0
        Space: 0
      VulkanBinding:
        Binding: 0
)""";

  for (uint32_t InvocationIndex = 0; InvocationIndex < InvocationStride;
       ++InvocationIndex) {
    DescriptorSets << "    - Name: OutputB_" << InvocationIndex << "\n";
    DescriptorSets << "      Kind: RWStructuredBuffer\n";
    DescriptorSets << "      DirectXBinding:\n";
    DescriptorSets << "        Register: " << (1 + InvocationIndex) << "\n";
    DescriptorSets << "        Space: 0\n";
    DescriptorSets << "      VulkanBinding:\n";
    DescriptorSets << "        Binding: " << (1 + InvocationIndex) << "\n";
  }

  Ofs << Shaders.str() << Buffers.str() << Results.str() << DescriptorSets.str()
      << std::endl;

  Ofs << "...\n" << "#--- end\n";

  std::stringstream Command;
  Command << "# UNSUPPORTED: Vulkan && !VK_KHR_shader_maximal_reconvergence"
          << std::endl;
  Command << "# UNSUPPORTED: !WaveSize_" << WaveSize << std::endl;
  Command << "\n\n# BUG: Some wave operations are not yet implemented."
          << "\n# XFAIL: Clang" << std::endl;

  // Add test expectation comments if this test is listed in the YAML.
  const auto TestExpectationsIt = TestExpectations.find(TestName);
  if (TestExpectationsIt != TestExpectations.end()) {
    for (const auto &ExpectationConfig : TestExpectationsIt->second) {
      Command << "\n\n";
      if (!ExpectationConfig.Comment.empty()) {
        Command << "# " << ExpectationConfig.Comment << std::endl;
      }
      if (ExpectationConfig.Expectation == TestExpectation::FAIL &&
          !ExpectationConfig.Condition.empty()) {
        Command << "# XFAIL: " << ExpectationConfig.Condition << std::endl;
      }
    }
  }

  Command << R"""(
# RUN: split-file %s %t
# RUN: %dxc_target -T cs_6_5 -fspv-enable-maximal-reconvergence -Fo %t.o %t/source.hlsl
# RUN: %offloader --no-diff-report %t/pipeline.yaml %t.o)""";
  Ofs << Command.str() << std::endl;
  Ofs.close();
}

} // namespace reconvergence
