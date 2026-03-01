
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

  std::vector<UVec4> Ref;
  const uint32_t InvocationStride = ThreadgroupSizeX * ThreadgroupSizeY;
  uint32_t MaxLoc =
      Program.execute(/*countOnly=*/true, WaveSize, InvocationStride, Ref);
  MaxLoc++;
  MaxLoc *= InvocationStride;
  Ref.resize(MaxLoc, UVec4(0u, 0u, 0u, 0u));

  Program.execute(/*countOnly=*/false, WaveSize, InvocationStride, Ref);
  TestCase.setExpectedResult(Ref);

  std::stringstream Functions, Main;
  Program.printCodeHlsl(Functions, Main);

  std::stringstream Header, Layout, Globals;

  Header << "#define THREADS_X " << ThreadgroupSizeX << "\n";
  Header << "#define THREADS_Y " << ThreadgroupSizeY << "\n\n";

  Layout << "StructuredBuffer<uint> InputA : register(t0);\n";
  Layout << "RWStructuredBuffer<uint4> OutputB : register(u1);\n\n";

  Globals << "bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> "
             "(bit % 32)) & 1) != 0; }\n";

  Globals << "static int outLoc = 0;\n";
  Globals << "static int invocationStride = " << InvocationStride << ";\n\n";

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
  const std::string Path = "shaders/" + std::to_string(WaveSize);
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
  const std::string Path = "reconvergence-tests/" + std::to_string(WaveSize);
  std::filesystem::create_directories(Path);
  const std::string Filename =
      Path + "/test_" + std::to_string(MaxNestingLevel) + "_" +
      std::to_string(WaveSize) + "_" + std::to_string(ThreadgroupSizeX) + "_" +
      std::to_string(ThreadgroupSizeY) + "_" + std::to_string(Seed) + ".test";
  std::ofstream Ofs(Filename);

  Ofs << "#--- source.hlsl\n" << std::endl;
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
  - Name: OutputB
    Format: UInt32
    Stride: 16
    FillSize: )"""";
  const auto &ExpectedResult = Test.getExpectedResult();
  Buffers << ExpectedResult.size() * 16;
  Buffers << R""""(
  - Name: ExpectedOutputB
    Format: UInt32
    Stride: 16
    Data: )"""";
  Buffers << "[ ";
  for (size_t I = 0; I < ExpectedResult.size(); ++I) {
    const UVec4 &Vec = ExpectedResult[I];
    Buffers << Vec.x() << ", " << Vec.y() << ", " << Vec.z() << ", " << Vec.w();
    if (I < ExpectedResult.size() - 1) {
      Buffers << ",";
    }
    Buffers << " ";
  }
  Buffers << "]" << std::endl;

  std::stringstream Results;
  Results << R"""(Results:
  - Result: Test
    Rule: BufferExact
    Actual: OutputB
    Expected: ExpectedOutputB
)""";

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
    - Name: OutputB
      Kind: RWStructuredBuffer
      DirectXBinding:
        Register: 1
        Space: 0
      VulkanBinding:
        Binding: 1
)""";

  Ofs << Shaders.str() << Buffers.str() << Results.str() << DescriptorSets.str()
      << std::endl;

  Ofs << "...\n" << "#--- end\n";

  std::stringstream Command;
  Command << "# UNSUPPORTED: Vulkan && !VK_KHR_shader_maximal_reconvergence"
          << std::endl;
  Command << "# UNSUPPORTED: !WaveSize_" << WaveSize << std::endl;
  Command << R"""(
# Bug: Some wave operations are not yet implemented.
# XFAIL: Clang

# RUN: split-file %s %t
# RUN: %dxc_target -T cs_6_5 -fspv-enable-maximal-reconvergence -Fo %t.o %t/source.hlsl
# RUN: %offloader %t/pipeline.yaml %t.o)""";
  Ofs << Command.str() << std::endl;
  Ofs.close();
}

} // namespace reconvergence
