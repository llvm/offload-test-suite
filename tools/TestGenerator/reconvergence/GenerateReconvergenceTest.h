#include <cstdint>
#include <map>

#include "TestCase.h"

namespace reconvergence {

class ReconvergenceTestGenerator {
public:
  ReconvergenceTestGenerator() = default;
  void createRandomizedTests(
      uint32_t TotalMaxNestingLevel, uint32_t TotalSeedGroup,
      const std::map<uint32_t, uint32_t> &NestingLevelToTestsCount,
      uint32_t WaveSize, uint32_t ThreadgroupSizeX, uint32_t ThreadgroupSizeY);

  bool SaveShader = false;

private:
  TestCase createSingleTest(uint32_t Seed, uint32_t MaxNestingLevel,
                            uint32_t WaveSize, uint32_t ThreadgroupSizeX,
                            uint32_t ThreadgroupSizeY);

  void saveShader(const TestCase &Test);
  void saveTestConfig(const TestCase &Test);
};
} // namespace reconvergence
