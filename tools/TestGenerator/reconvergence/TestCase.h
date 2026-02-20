#ifndef TESTCASE_H_
#define TESTCASE_H_

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "VectorUtils.h"

namespace reconvergence {

class TestCase {
public:
  TestCase(uint32_t seed, uint32_t maxNestingLevel, uint32_t waveSize,
           uint32_t threadgroupSizeX, uint32_t threadgroupSizeY)
      : seed_(seed), max_nesting_level_(maxNestingLevel), wave_size_(waveSize),
        threadgroup_size_x_(threadgroupSizeX),
        threadgroup_size_y_(threadgroupSizeY) {}

  uint32_t getSeed() const { return seed_; }
  uint32_t getMaxNestingLevel() const { return max_nesting_level_; }
  uint32_t getWaveSize() const { return wave_size_; }
  uint32_t getThreadgroupSizeX() const { return threadgroup_size_x_; }
  uint32_t getThreadgroupSizeY() const { return threadgroup_size_y_; }

  std::stringstream &getShader() { return shader_; }
  std::string getShaderString() const { return shader_.str(); }

  void setExpectedResult(const std::vector<UVec4> &expected_result) {
    expected_result_ = std::move(expected_result);
  }
  const std::vector<UVec4> &getExpectedResult() const {
    return expected_result_;
  }

private:
  std::stringstream shader_;
  std::vector<UVec4> expected_result_;
  uint32_t seed_;
  uint32_t max_nesting_level_;
  uint32_t wave_size_;
  uint32_t threadgroup_size_x_;
  uint32_t threadgroup_size_y_;
};

} // namespace reconvergence

#endif // TESTCASE_H_
