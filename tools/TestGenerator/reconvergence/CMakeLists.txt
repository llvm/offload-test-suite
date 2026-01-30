cmake_minimum_required(VERSION 3.16)

project(maximal_reconvergence_test_generator)

set(CMAKE_CXX_STANDARD 17)

add_executable(generate_reconvergence_test main.cc GenerateReconvergenceTest.cc GenerateReconvergenceTest.h Ballot.h Ballots.h MaskUtils.h Op.h TestCase.h RandomProgram.h RandomUtils.h RandomUtils.cc VectorUtils.h MathUtils.h)

add_custom_target(run ALL
  COMMAND generate_reconvergence_test
  DEPENDS generate_reconvergence_test)
