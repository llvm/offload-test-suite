#include <cstdint>
#include <map>

#include "TestCase.h"

namespace reconvergence {

class ReconvergenceTestGenerator {
public:
  ReconvergenceTestGenerator() = default;
  void createRandomizedTests(
      uint32_t totalMaxNestingLevel, uint32_t totalSeedGroup,
      const std::map<uint32_t, uint32_t> &nestingLevelToTestsCount,
      uint32_t subgroupSize, uint32_t workgroupSizeX, uint32_t workgroupSizeY);

  bool save_shader_ = true;

private:
  TestCase createSingleTest(uint32_t seed, uint32_t maxNestingLevel,
                            uint32_t subgroupSize, uint32_t workgroupSizeX,
                            uint32_t workgroupSizeY);

  void saveShader(const TestCase &test);
  void saveTestConfig(const TestCase &test);
};
} // namespace reconvergence
