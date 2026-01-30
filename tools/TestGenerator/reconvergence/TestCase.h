#ifndef EXPERIMENTAL_USERS_CLUCIE_TESTCASE_H_
#define EXPERIMENTAL_USERS_CLUCIE_TESTCASE_H_

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "VectorUtils.h"

namespace reconvergence {

class TestCase {
public:
  TestCase(uint32_t seed, uint32_t maxNestingLevel, uint32_t subgroupSize,
           uint32_t workgroupSizeX, uint32_t workgroupSizeY)
      : seed_(seed), max_nesting_level_(maxNestingLevel),
        subgroup_size_(subgroupSize), workgroup_size_x_(workgroupSizeX),
        workgroup_size_y_(workgroupSizeY) {}

  uint32_t getSeed() const { return seed_; }
  uint32_t getMaxNestingLevel() const { return max_nesting_level_; }
  uint32_t getSubgroupSize() const { return subgroup_size_; }
  uint32_t getWorkgroupSizeX() const { return workgroup_size_x_; }
  uint32_t getWorkgroupSizeY() const { return workgroup_size_y_; }

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
  uint32_t subgroup_size_;
  uint32_t workgroup_size_x_;
  uint32_t workgroup_size_y_;
};

} // namespace reconvergence

#endif // EXPERIMENTAL_USERS_CLUCIE_TESTCASE_H_
