
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
    uint32_t totalMaxNestingLevel, uint32_t totalSeedGroup,
    const std::map<uint32_t, uint32_t> &nestingLevelToTestsCount,
    uint32_t subgroupSize, uint32_t workgroupSizeX, uint32_t workgroupSizeY) {
  uint32_t totalTestsCount = 0;
  for (uint32_t maxNestingLevel = 2; maxNestingLevel <= totalMaxNestingLevel;
       ++maxNestingLevel) {
    uint32_t testsCount = nestingLevelToTestsCount.at(maxNestingLevel);
    totalTestsCount += testsCount * totalSeedGroup;
  }

  uint32_t testId = 0;
  for (uint32_t maxNestingLevel = 2; maxNestingLevel <= totalMaxNestingLevel;
       ++maxNestingLevel) {
    uint32_t seed = 0;
    for (uint32_t seedGroup = 0; seedGroup < totalSeedGroup; ++seedGroup) {
      uint32_t testsCount = nestingLevelToTestsCount.at(maxNestingLevel);
      for (uint32_t _ = 0; _ < testsCount; ++_) {
        const TestCase &test =
            createSingleTest(seed, maxNestingLevel, subgroupSize,
                             workgroupSizeX, workgroupSizeY);
        if (save_shader_) {
          saveShader(test);
        }
        saveTestConfig(test);

        testId++;
        seed++;
        std::cout << "\rShader " << testId << " / " << totalTestsCount
                  << " generated successfully." << std::flush;
      }
    }
  }
}

TestCase ReconvergenceTestGenerator::createSingleTest(uint32_t seed,
                                                      uint32_t maxNestingLevel,
                                                      uint32_t subgroupSize,
                                                      uint32_t workgroupSizeX,
                                                      uint32_t workgroupSizeY) {
  TestCase testCase(seed, maxNestingLevel, subgroupSize, workgroupSizeX,
                    workgroupSizeY);
  ComputeRandomProgram program(testCase);

  program.generateRandomProgram();

  std::vector<UVec4> ref;
  const uint32_t invocationStride = workgroupSizeX * workgroupSizeY;
  uint32_t maxLoc =
      program.execute(/*countOnly=*/true, subgroupSize, invocationStride, ref);
  maxLoc++;
  maxLoc *= invocationStride;
  ref.resize(maxLoc, UVec4(0u, 0u, 0u, 0u));

  program.execute(/*countOnly=*/false, subgroupSize, invocationStride, ref);
  testCase.setExpectedResult(ref);

  std::stringstream functions, main;
  program.printCodeHlsl(functions, main);

  std::stringstream header, layout, globals;

  header << "#define THREADS_X " << workgroupSizeX << "\n";
  header << "#define THREADS_Y " << workgroupSizeY << "\n\n";

  layout << "StructuredBuffer<uint> InputA : register(t0);\n";
  layout << "RWStructuredBuffer<uint4> OutputB : register(u1);\n\n";

  globals << "bool testBit(uint4 mask, uint bit) { return ((mask[bit / 32] >> "
             "(bit % 32)) & 1) != 0; }\n";

  globals << "static int outLoc = 0;\n";
  globals << "static int invocationStride = " << invocationStride << ";\n\n";

  std::stringstream &shader = testCase.getShader();
  shader << header.str();
  shader << layout.str();
  shader << globals.str();

  shader << functions.str() << "\n";

  std::stringstream setup;

  setup << "[numthreads(THREADS_X, THREADS_Y, 1)]\n";
  setup << "void main(uint gIndex : SV_GroupIndex)\n" << "{\n";
  shader << setup.str() << main.str() << "}\n";
  return testCase;
}

void ReconvergenceTestGenerator::saveShader(const TestCase &test) {
  uint32_t subgroupSize = test.getSubgroupSize();
  uint32_t workgroupSizeX = test.getWorkgroupSizeX();
  uint32_t workgroupSizeY = test.getWorkgroupSizeY();
  uint32_t maxNestingLevel = test.getMaxNestingLevel();
  uint32_t seed = test.getSeed();
  std::string path = "shaders/" + std::to_string(subgroupSize);
  std::filesystem::create_directories(path);
  std::string filename = path + "/test_" + std::to_string(maxNestingLevel) +
                         "_" + std::to_string(subgroupSize) + "_" +
                         std::to_string(workgroupSizeX) + "_" +
                         std::to_string(workgroupSizeY) + "_" +
                         std::to_string(seed) + ".hlsl";
  std::ofstream ofs(filename);
  ofs << test.getShaderString() << std::endl;
  ofs.close();
}

void ReconvergenceTestGenerator::saveTestConfig(const TestCase &test) {
  uint32_t subgroupSize = test.getSubgroupSize();
  uint32_t workgroupSizeX = test.getWorkgroupSizeX();
  uint32_t workgroupSizeY = test.getWorkgroupSizeY();
  uint32_t maxNestingLevel = test.getMaxNestingLevel();
  uint32_t seed = test.getSeed();
  std::string path = "tests/" + std::to_string(subgroupSize);
  std::filesystem::create_directories(path);
  std::string filename = path + "/test_" + std::to_string(maxNestingLevel) +
                         "_" + std::to_string(subgroupSize) + "_" +
                         std::to_string(workgroupSizeX) + "_" +
                         std::to_string(workgroupSizeY) + "_" +
                         std::to_string(seed) + ".test";
  std::ofstream ofs(filename);

  ofs << "#--- source.hlsl\n" << std::endl;
  ofs << test.getShaderString() << std::endl;
  ofs << "//--- pipeline.yaml\n" << std::endl;
  ofs << "---\n" << std::endl;

  // Configuration for the pipeline.
  std::stringstream shaders;
  shaders <<
      R""""(Shaders:
  - Stage: Compute
    Entry: main
    DispatchSize: [1, 1, 1]
)"""";

  std::stringstream buffers;
  buffers <<
      R""""(Buffers:
  - Name: InputA
    Format: UInt32
    Data: [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90 ]
  - Name: OutputB
    Format: UInt32
    Stride: 16
    FillSize: )"""";
  const auto &expectedResult = test.getExpectedResult();
  buffers << expectedResult.size() * 16;
  buffers << R""""(
  - Name: ExpectedOutputB
    Format: UInt32
    Stride: 16
    Data: )"""";
  buffers << "[ ";
  for (size_t i = 0; i < expectedResult.size(); ++i) {
    const UVec4 &vec = expectedResult[i];
    buffers << vec.x() << ", " << vec.y() << ", " << vec.z() << ", " << vec.w();
    if (i < expectedResult.size() - 1) {
      buffers << ",";
    }
    buffers << " ";
  }
  buffers << "]" << std::endl;

  std::stringstream results;
  results << R"""(Results:
  - Result: Test
    Rule: BufferExact
    Actual: OutputB
    Expected: ExpectedOutputB
)""";

  std::stringstream descriptorSets;
  descriptorSets << R"""(DescriptorSets:
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

  ofs << shaders.str() << buffers.str() << results.str() << descriptorSets.str()
      << std::endl;

  ofs << "...\n" << "#--- end\n";

  std::stringstream command;
  command << "# UNSUPPORTED: Vulkan && !VK_KHR_shader_maximal_reconvergence"
          << std::endl;
  command << "# UNSUPPORTED: !SubgroupSize" << subgroupSize << std::endl;
  command << R"""(
# Bug: Some wave operations are not yet implemented.
# XFAIL: Clang

# RUN: split-file %s %t
# RUN: %dxc_target -T cs_6_5 -fspv-enable-maximal-reconvergence -Fo %t.o %t/source.hlsl
# RUN: %offloader %t/pipeline.yaml %t.o)""";
  ofs << command.str() << std::endl;
  ofs.close();
}

} // namespace reconvergence
