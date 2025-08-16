function(try_compile_hlsl output)
  if (DEFINED ${output})
    return()
  endif()
  # NOTE: `SOURCE` needs to be a multi-argument because source code
  # often contains semicolons which happens to be CMake's list separator
  # which confuses `cmake_parse_arguments()`.
  cmake_parse_arguments(ARG "" "" "SOURCE;FLAGS" ${ARGN})
  if (ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unexpected arguments \"${ARG_UNPARSED_ARGUMENTS}\"")
  endif()
  if(NOT ARG_SOURCE)
    set(ARG_SOURCE "export int foo(int x, int y) { return x + y; }\n")
  endif()
  set(SIMPLE_HLSL ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/src.hlsl)
  file(WRITE ${SIMPLE_HLSL} "${ARG_SOURCE}\n")
  string(REGEX MATCHALL "<[A-Za-z0-9_]*>" substitutions
         ${CMAKE_C_COMPILE_OBJECT})

  set(TRY_COMPILE_FLAGS "${ARG_FLAGS}")
  string(REPLACE ";" " " extra_flags "${TRY_COMPILE_FLAGS}")

  set(test_compile_command ${DXC_EXECUTABLE} ${extra_flags} ${SIMPLE_HLSL})
  string(REPLACE "\"" "" test_compile_command "${test_compile_command}")
  string(REPLACE " " ";" test_compile_command "${test_compile_command}")

  execute_process(
    COMMAND ${test_compile_command}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE TEST_OUTPUT
    ERROR_VARIABLE TEST_ERROR
  )
  CHECK_COMPILER_FLAG_COMMON_PATTERNS(_CheckCCompilerFlag_COMMON_PATTERNS)
  set(ERRORS_FOUND OFF)
  foreach(var ${_CheckCCompilerFlag_COMMON_PATTERNS})
    if("${var}" STREQUAL "FAIL_REGEX")
      continue()
    endif()
    if("${TEST_ERROR}" MATCHES "${var}" OR "${TEST_OUTPUT}" MATCHES "${var}")
      set(ERRORS_FOUND ON)
    endif()
  endforeach()

  if(result EQUAL 0 AND NOT ERRORS_FOUND)
    set(result True)
  else()
    file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Testing compiler for supporting " ${ARGN} ":\n"
        "Command: ${test_compile_command}\n"
        "${TEST_OUTPUT}\n${TEST_ERROR}\n${result}\n")
    set(result False)
  endif()
  set(${output} ${result} CACHE BOOL "HLSL compiler supports SPIR-V")
  message(STATUS "${output} - ${${output}}")
endfunction()

list(APPEND OFFLOADTEST_DEPS
  api-query
  offloader
  FileCheck
  split-file
  imgdiff
  OffloadTestUnit
  not)

if (OFFLOADTEST_TEST_CLANG)
  list(APPEND OFFLOADTEST_DEPS clang)
endif()

add_custom_target(hlsl-test-depends DEPENDS ${OFFLOADTEST_DEPS})

try_compile_hlsl(SUPPORTS_SPIRV SOURCE
                "RWBuffer<int> In : register(u0);
                RWBuffer<int> Out : register(u1);

                [numthreads(8,1,1)]
                void main(uint3 TID : SV_GroupThreadID) {
                  Out[TID.x] = In[TID.x];
                }"
                FLAGS -T cs_6_0 -spirv
)

# Initialize all platforms to false.
set(TEST_d3d12 False)
set(TEST_vk False)
set(TEST_mtl False)
set(FORCE_CLANG False)
set(FORCE_WARP False)

if (OFFLOADTEST_ENABLE_D3D12)
  list(APPEND platforms_to_test d3d12)
endif()

if (SUPPORTS_SPIRV AND OFFLOADTEST_ENABLE_VULKAN)
  list(APPEND platforms_to_test vk)
endif()

if (OFFLOADTEST_ENABLE_METAL)
  list(APPEND platforms_to_test mtl)
endif()

llvm_canonicalize_cmake_booleans(OFFLOADTEST_TEST_CLANG)

foreach(platform ${platforms_to_test})
  set(TEST_${platform} False)
endforeach()

# Because this path comes in from a user, we need to normalize it before
# expanding it into lit.
cmake_path(SET DXC_DIR NORMALIZE "${DXC_DIR}")

function(add_offloadtest_lit_suite suite)
  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.site.cfg.py
    MAIN_CONFIG
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.cfg.py
    PATHS
    "OFFLOADTEST_BINARY_DIR"
    "LLVM_TOOLS_DIR"
    )

  add_lit_testsuite(check-hlsl-${suite}
    "Running the HLSL regression tests for ${suite}"
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}
    DEPENDS ${OFFLOADTEST_DEPS}
    )
  set_target_properties(check-hlsl-${suite} PROPERTIES FOLDER "HLSL tests")

  add_lit_testsuites(hlsl-${suite} ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${OFFLOADTEST_DEPS}
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${suite}/
    FOLDER "HLSL tests/Suites/${suite}"
  )
endfunction()

umbrella_lit_testsuite_begin(check-hlsl)

# Add unit test suite
configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg.py
    MAIN_CONFIG
    ${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.cfg.py
    PATHS
    "OFFLOADTEST_BINARY_DIR"
    "LLVM_TOOLS_DIR"
    )
add_lit_testsuite(check-hlsl-unit
    "Running the offload test unit test suite"
    ${CMAKE_CURRENT_BINARY_DIR}/Unit
    DEPENDS ${OFFLOADTEST_DEPS}
    )
set_target_properties(check-hlsl-unit PROPERTIES FOLDER "HLSL tests")

if (NOT OFFLOADTEST_WARP_ONLY)
  foreach(platform ${platforms_to_test})
    set(TEST_${platform} True)
    add_offloadtest_lit_suite(${platform})

    if (OFFLOADTEST_TEST_CLANG)
      set(FORCE_CLANG True)
      add_offloadtest_lit_suite(clang-${platform})
      set(FORCE_CLANG False)
    endif()

    set(TEST_${platform} False)
  endforeach()
endif()

if (OFFLOADTEST_ENABLE_D3D12 AND WIN32)
  set(TEST_d3d12 True)
  set(FORCE_WARP True)
  get_property(WARP_ARCHITECTURE GLOBAL PROPERTY WARP_ARCHITECTURE)
  add_offloadtest_lit_suite(warp-d3d12)

  if (OFFLOADTEST_TEST_CLANG)
    set(FORCE_CLANG True)
    add_offloadtest_lit_suite(clang-warp-d3d12)
    set(FORCE_CLANG False)
  endif()

  set(TEST_d3d12 False)
endif()

umbrella_lit_testsuite_end(check-hlsl)
set_target_properties(check-hlsl PROPERTIES FOLDER "HLSL tests")
