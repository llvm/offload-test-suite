# This project is only buildable as a subproject of LLVM.

project(OffloadTest)

# Add path for custom modules
list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
  )

set(OFFLOADTEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(OFFLOADTEST_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(OFFLOADTEST_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(DXC_DIR "" CACHE STRING "Path to a DXC build or install binary directory")
find_program(DXC_EXECUTABLE dxc HINTS ${DXC_DIR})
find_program(DXV_EXECUTABLE dxv HINTS ${DXC_DIR})

if ("clang" IN_LIST LLVM_ENABLE_PROJECTS)
  set(default_OFFLOADTEST_TEST_CLANG On)
else()
  set(default_OFFLOADTEST_TEST_CLANG Off)
endif()
option(OFFLOADTEST_TEST_CLANG "Enable testing in-tree clang as the compiler" ${default_OFFLOADTEST_TEST_CLANG})

option(OFFLOADTEST_WARP_ONLY "Only generate Warp configurations (useful for testing in VMs, Windows-only)." OFF)
if (OFFLOADTEST_WARP_ONLY AND NOT WIN32)
  message(FATAL_ERROR "OFFLOADTEST_WARP_ONLY is only suppoted on Windows hosts!")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_CURRENT_BINARY_DIR}/include)

macro(add_offloadtest_tool name)
  add_llvm_executable(${name} ${ARGN})
  set_target_properties(${name} PROPERTIES FOLDER "HLSL Test/Tools")
endmacro()

macro(add_offloadtest_library name)
  add_llvm_library(OffloadTest${name} BUILDTREE_ONLY ${ARGN})
  set_target_properties(OffloadTest${name} PROPERTIES FOLDER "Offload Test/Libraries")
endmacro()

find_package(Vulkan)
message(STATUS "Vulkan Include Dirs: ${Vulkan_INCLUDE_DIRS}")
if (Vulkan_INCLUDE_DIRS)
  set(OFFLOADTEST_ENABLE_VULKAN On)
endif ()

find_package(D3D12)
message(STATUS "D3D12 Include Dirs: ${D3D12_INCLUDE_DIRS}")
if (D3D12_INCLUDE_DIRS)
  set(OFFLOADTEST_ENABLE_D3D12 On)
endif ()

find_package(D3D12_WSL)
if (D3D12_WSL_FOUND)
  set(OFFLOADTEST_ENABLE_D3D12 On)
endif ()

if (APPLE)
  set(METAL_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third-party/metal-cpp
                         ${CMAKE_CURRENT_SOURCE_DIR}/third-party/metal_irconverter_runtime)
  set(OFFLOADTEST_ENABLE_METAL On)
endif ()

if (NOT OFFLOADTEST_ENABLE_VULKAN AND NOT OFFLOADTEST_ENABLE_D3D12 AND NOT APPLE)
  message(FATAL_ERROR "No supported runtime API")
endif ()

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/Config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/Config.h)

set(DIRECTX_HEADERS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/third-party/DirectX-Headers)

if (WIN32)
  message(STATUS "Including vendored zlib")
  add_subdirectory(third-party/zlib)
  # These are some extra fun hacks becauze ZLIB's CMake is fragile
  if (NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS_BUILDTREE_ONLY zlibstatic)
  endif ()
  # libpng does some crazy stuff with zlib's include directories.
  set(ZLIB_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib
                        ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib CACHE STRING "" FORCE)
  set(ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third-party/zlib
                        ${CMAKE_CURRENT_BINARY_DIR}/third-party/zlib CACHE STRING "" FORCE)
  include_directories(${ZLIB_INCLIDE_DIR})
  # Skip exports for libpng because they don't work with building my own zlib.
  set(SKIP_INSTALL_EXPORT On)
endif ()
add_subdirectory(third-party/libpng)
set_property(GLOBAL APPEND PROPERTY LLVM_EXPORTS_BUILDTREE_ONLY png_static)

include(Warp)

# This must be after the third-party targets are generated so that we only apply
# it to code in the offload-test-suite.
option(OFFLOADTEST_USE_CLANG_TIDY "Use clang-tidy on offload test codebase" Off)
option(OFFLOADTEST_CLANG_TIDY_APPLY_FIX "Automatically apply clang-tidy fixes" Off)

if (OFFLOADTEST_USE_CLANG_TIDY)
  find_program(CLANG_TIDY clang-tidy)
  if (NOT CLANG_TIDY)
    message(
      FATAL_ERROR
        "Clang-tidy not found in PATH, please specify CLANG_TIDY to CMake.")
  endif ()
  if (APPLE)
    if (CMAKE_OSX_SYSROOT)
      set(CLANG_TIDY_ARGS --extra-arg=-isysroot${CMAKE_OSX_SYSROOT})
    elseif(NOT CMAKE_CROSSCOMPILING)
      # xcrun -sdk macosx --show-sdk-path
      execute_process(COMMAND xcrun -sdk macosx --show-sdk-path
        OUTPUT_VARIABLE SYSROOT
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(CLANG_TIDY_ARGS --extra-arg=-isysroot${SYSROOT})
    endif()
  endif()
  if (OFFLOADTEST_CLANG_TIDY_APPLY_FIX)
    set(CLANG_TIDY_ARGS ${CLANG_TIDY_ARGS} --fix)
  endif ()
  set(CMAKE_C_CLANG_TIDY ${CLANG_TIDY} ${CLANG_TIDY_ARGS})
  set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY} ${CLANG_TIDY_ARGS})
endif ()

add_subdirectory(lib)
add_subdirectory(tools)

add_subdirectory(unittests)
add_subdirectory(test)
