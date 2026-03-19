cmake_minimum_required(VERSION 3.16)

project(maximal_reconvergence_test_generator)

set(CMAKE_CXX_STANDARD 17)
if(NOT LLVM_ENABLE_RTTI)
  if(MSVC)
    add_compile_options(/GR-)
  else()
    add_compile_options(-fno-rtti)
  endif()
endif()

add_executable(build_reconvergence_test main.cc GenerateReconvergenceTest.cc GenerateReconvergenceTest.h Ballot.h Ballots.h MaskUtils.h Op.h TestCase.h RandomProgram.h RandomProgramProbabilities.h RandomProgramProbabilities.cc RandomUtils.h RandomUtils.cc VectorUtils.h MathUtils.h)

target_link_libraries(build_reconvergence_test PRIVATE LLVMCore LLVMSupport)

add_custom_target(generate-reconvergence-tests ALL
  COMMAND $<TARGET_FILE:build_reconvergence_test> -output_dir "${OFFLOADTEST_SOURCE_DIR}/reconvergence-tests" -counts_per_level "1" -max_nesting_level "6" -total_seed_group "8" -failed_tests_yaml "${CMAKE_CURRENT_SOURCE_DIR}/FailedTests.yaml"
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS build_reconvergence_test
  COMMENT "Generating reconvergence tests"
)
