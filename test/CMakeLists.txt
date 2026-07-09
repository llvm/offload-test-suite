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

option(OFFLOADTEST_ENABLE_DEBUG "Enable available runtime debug layers (defaults On)." ON)
option(OFFLOADTEST_ENABLE_VALIDATION "Enable available runtime validation layers (defaults Off)." OFF)

list(APPEND OFFLOADTEST_DEPS
  api-query
  offloader
  FileCheck
  split-file
  imgdiff
  OffloadTestUnit
  obj2yaml
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

llvm_canonicalize_cmake_booleans(OFFLOADTEST_TEST_CLANG
                                 OFFLOADTEST_ENABLE_DEBUG
                                 OFFLOADTEST_ENABLE_VALIDATION)

foreach(platform ${platforms_to_test})
  set(TEST_${platform} False)
endforeach()

# Because this path comes in from a user, we need to normalize it before
# expanding it into lit.
cmake_path(SET DXC_DIR NORMALIZE "${DXC_DIR}")

# The lit.site.cfg.py.in templates expect @OFFLOADTEST_TEST_ROOT@ — the
# directory containing test/lit.cfg.py. In an in-tree build that's the
# project source dir; in the standalone install layout it's the install
# share dir (set by configure-test-suite.py).
set(OFFLOADTEST_TEST_ROOT ${OFFLOADTEST_SOURCE_DIR})

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


# Adds a standalone check-hlsl-<suite> target that runs the Vulkan test suite
# pinned to a specific device, selected by name through the OFFLOADTEST_GPU_NAME
# environment variable that test/lit.cfg.py already honors -- there is no
# device-specific logic in the Python test config. add_lit_testsuite() offers no
# hook to set an environment variable for the spawned lit process, so this
# defines the target directly (mirroring add_lit_target) and exports
# OFFLOADTEST_GPU_NAME=<gpu_name> into lit's environment via `cmake -E env`.
# Passing "llvmpipe" targets Mesa's lavapipe software rasterizer; passing a
# vendor/description substring (e.g. "NVIDIA") is enough to add hardware suites
# such as clang-vk-nvidia in a future change. The target is intentionally kept
# out of the check-hlsl umbrella so it only runs when explicitly invoked.
function(add_offloadtest_gpu_suite suite gpu_name)
  # Suites named clang-* compile with clang instead of dxc. FORCE_CLANG is
  # consumed by configure_lit_site_cfg below, which substitutes @FORCE_CLANG@
  # into config.offloadtest_test_clang. Deriving it from the suite name here
  # keeps the toggle local to this function instead of requiring callers to
  # bracket each call with set(FORCE_CLANG ...).
  if (suite MATCHES "^clang-")
    set(FORCE_CLANG True)
  else()
    set(FORCE_CLANG False)
  endif()

  configure_lit_site_cfg(
    ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.site.cfg.py
    MAIN_CONFIG
    ${CMAKE_CURRENT_BINARY_DIR}/${suite}/lit.cfg.py
    PATHS
    "OFFLOADTEST_BINARY_DIR"
    "LLVM_TOOLS_DIR"
    )

  get_llvm_lit_path(lit_base_dir lit_file_name ALLOW_EXTERNAL)
  set(lit_args "${LLVM_LIT_ARGS}")
  separate_arguments(lit_args)

  add_custom_target(check-hlsl-${suite}
    COMMAND ${CMAKE_COMMAND} -E env OFFLOADTEST_GPU_NAME=${gpu_name}
            ${Python3_EXECUTABLE} ${lit_base_dir}/${lit_file_name}
            ${lit_args} ${CMAKE_CURRENT_BINARY_DIR}/${suite}
    COMMENT "Running the HLSL regression tests for ${suite} (device: ${gpu_name})"
    USES_TERMINAL
    )
  add_dependencies(check-hlsl-${suite} ${OFFLOADTEST_DEPS})
  set_target_properties(check-hlsl-${suite} PROPERTIES FOLDER "HLSL tests")
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
  set(FORCE_WARP False)
endif()

if (SUPPORTS_SPIRV AND OFFLOADTEST_ENABLE_VULKAN AND WIN32)
  set(TEST_vk True)
  # Lavapipe (Mesa's software Vulkan rasterizer) enumerates as "llvmpipe (LLVM
  # ...)"; select it purely via OFFLOADTEST_GPU_NAME, which
  # add_offloadtest_gpu_suite exports into lit's environment. No lavapipe-specific
  # logic lives in the Python test config.
  add_offloadtest_gpu_suite(vk-lavapipe llvmpipe)

  if (OFFLOADTEST_TEST_CLANG)
    add_offloadtest_gpu_suite(clang-vk-lavapipe llvmpipe)
  endif()

  set(TEST_vk False)
endif()

umbrella_lit_testsuite_end(check-hlsl)
set_target_properties(check-hlsl PROPERTIES FOLDER "HLSL tests")

# === Standalone test suite distribution ===
#
# See docs/offload-distribution.md for an overview. All test infrastructure
# is installed under a single `offload-test-suite` component. Build with
# `ninja install-offload-test-suite`.
set(OFFLOADTEST_INSTALL_TESTDIR "share/hlsl-test-suite" CACHE STRING
    "Install directory (relative to CMAKE_INSTALL_PREFIX) for the standalone offload test suite distribution.")

# Install the test source tree. Using two separate install(DIRECTORY) calls
# keeps EXCLUDE patterns out of FILES_MATCHING, where EXCLUDE inside
# FILES_MATCHING is interpreted as an include-override.
install(DIRECTORY ${OFFLOADTEST_SOURCE_DIR}/test/
  DESTINATION ${OFFLOADTEST_INSTALL_TESTDIR}/test
  COMPONENT offload-test-suite
  PATTERN "Output" EXCLUDE
  PATTERN "__pycache__" EXCLUDE
  PATTERN "CMakeLists.txt" EXCLUDE
  PATTERN "*.py.in" EXCLUDE)

# Install the canonical lit.site.cfg.py template so configure-test-suite.py
# can substitute @VAR@ tokens against the same template the in-tree build
# uses, avoiding any drift between the two code paths.
install(FILES ${OFFLOADTEST_SOURCE_DIR}/test/lit.site.cfg.py.in
  DESTINATION ${OFFLOADTEST_INSTALL_TESTDIR}
  COMPONENT offload-test-suite)

# Install the standalone configure script.
install(PROGRAMS ${OFFLOADTEST_SOURCE_DIR}/utils/configure-test-suite.py
  DESTINATION ${OFFLOADTEST_INSTALL_TESTDIR}
  COMPONENT offload-test-suite)

# Install requirements.txt so the standalone test runner can pip install
# its dependencies (lit, pyyaml) without needing a source checkout. This
# keeps a single source of truth for pinned versions.
install(FILES ${OFFLOADTEST_SOURCE_DIR}/test/requirements.txt
  DESTINATION ${OFFLOADTEST_INSTALL_TESTDIR}
  COMPONENT offload-test-suite)

# If golden images were available at configure time, ship them inside the
# distribution so the test runner doesn't need to re-checkout the repo.
if (GOLDENIMAGE_DIR AND EXISTS "${GOLDENIMAGE_DIR}")
  file(TO_CMAKE_PATH "${GOLDENIMAGE_DIR}" _goldenimage_dir)
  install(DIRECTORY "${_goldenimage_dir}/"
    DESTINATION ${OFFLOADTEST_INSTALL_TESTDIR}/golden-images
    COMPONENT offload-test-suite
    PATTERN ".git" EXCLUDE
    PATTERN ".gitattributes" EXCLUDE)
endif()

if (NOT LLVM_ENABLE_IDE)
  add_llvm_install_targets(install-offload-test-suite
    COMPONENT offload-test-suite)
endif()
