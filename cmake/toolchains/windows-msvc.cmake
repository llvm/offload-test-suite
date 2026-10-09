# CMake toolchain for cross-compiling OffloadTest from a non-Windows host to
# the x86_64 Windows MSVC ABI. Targets clang as the compiler and lld-link
# as the linker; produces `offloader.exe` (and any other targets in the
# build) that can then be invoked under Wine + vkd3d-proton on Linux, on
# WSL via the WSL D3D12 path, or copied to a real Windows machine.
#
# The toolchain is **agnostic about how the Windows SDK / CRT are
# acquired**. Common paths:
#
#   - xwin                    https://github.com/Jake-Shadle/xwin
#   - msvc-wine               https://github.com/mstorsjo/msvc-wine
#   - EWDK (Enterprise WDK)   Microsoft's portable SDK + build tools ZIP
#   - Hand-copied from a Windows install of Visual Studio
#
# Any of these produce a tree containing UM / shared / UCRT / WinRT headers
# and matching libs. Point this toolchain at them via the four cache /
# environment variables below and the build is independent of which tool
# produced the tree.
#
# Required (set via -D… on the configure line, or as environment variables):
#
#   WINDOWS_SDK_INCLUDE_ROOT   Directory containing `um/`, `shared/`,
#                              `ucrt/`, and `winrt/` header subdirs.
#                              xwin layout:  ~/.xwin-cache/splat/sdk/include
#                              msvc-wine:    <root>/kits/<ver>/Include/<ver>
#                              EWDK:         <ewdk>/Program Files/Windows Kits/10/Include/<ver>
#   WINDOWS_SDK_LIB_ROOT       Directory containing `ucrt/x86_64/` and
#                              `um/x86_64/` lib subdirs.
#                              xwin layout:  ~/.xwin-cache/splat/sdk/lib
#                              msvc-wine:    <root>/kits/<ver>/Lib/<ver>
#                              EWDK:         <ewdk>/Program Files/Windows Kits/10/Lib/<ver>
#   WINDOWS_CRT_INCLUDE_DIR    Directory containing MSVC CRT headers
#                              (`stdint.h`, `stdio.h`, `xmmintrin.h` proxies,
#                              `cwchar`, …).
#                              xwin layout:  ~/.xwin-cache/splat/crt/include
#                              msvc-wine:    <root>/vc/Tools/MSVC/<ver>/include
#                              EWDK:         <ewdk>/Program Files/Microsoft Visual Studio/<ed>/<ver>/VC/Tools/MSVC/<ver>/include
#   WINDOWS_CRT_LIB_DIR        Directory containing MSVC CRT libs
#                              (`msvcrt.lib`, `vcruntime.lib`, …) for x86_64.
#                              xwin layout:  ~/.xwin-cache/splat/crt/lib/x86_64
#                              msvc-wine:    <root>/vc/Tools/MSVC/<ver>/lib/x64
#                              EWDK:         <ewdk>/…/MSVC/<ver>/lib/x64
#
# Optional (sensible defaults below):
#
#   WINDOWS_TARGET_ARCH        Target architecture triple stem; default
#                              `x86_64`. Set to `aarch64` for ARM64 Windows.
#   CMAKE_C_COMPILER /         Default: the `clang` / `clang++` on PATH.
#   CMAKE_CXX_COMPILER         Set to a specific compiler when the host
#                              has multiple clang versions.
#
# Example configure (xwin layout, adjust paths for your SDK source):
#
#   cmake -S llvm-project/llvm -B build-win -G Ninja \
#     -DCMAKE_TOOLCHAIN_FILE=$PWD/OffloadTest/cmake/toolchains/windows-msvc.cmake \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DLLVM_TABLEGEN=$PWD/build/bin/llvm-tblgen \
#     -DCLANG_TABLEGEN=$PWD/build/bin/clang-tblgen \
#     -DLLVM_ENABLE_PROJECTS=clang \
#     -DLLVM_EXTERNAL_PROJECTS=OffloadTest \
#     -DLLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR=$PWD/OffloadTest \
#     -DOFFLOADTEST_ENABLE_VULKAN=OFF \
#     -DOFFLOADTEST_ENABLE_METAL=OFF \
#     -DOFFLOADTEST_ENABLE_D3D12=ON \
#     -DD3D12_INCLUDE_DIRS=$HOME/.xwin-cache/splat/sdk/include/um \
#     -DD3D12_LIBRARIES='d3d12.lib;dxcore.lib;dxguid.lib;d3dcompiler.lib' \
#     -DWINDOWS_SDK_INCLUDE_ROOT=$HOME/.xwin-cache/splat/sdk/include \
#     -DWINDOWS_SDK_LIB_ROOT=$HOME/.xwin-cache/splat/sdk/lib \
#     -DWINDOWS_CRT_INCLUDE_DIR=$HOME/.xwin-cache/splat/crt/include \
#     -DWINDOWS_CRT_LIB_DIR=$HOME/.xwin-cache/splat/crt/lib/x86_64 \
#     -DPNG_INTEL_SSE=OFF
#
#   cmake --build build-win --target offloader
#
# See docs/cross-build.md for the rationale behind each flag and tips for
# acquiring the SDK from the various sources above.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# CMake re-loads the toolchain file inside `try_compile`, in a fresh scope
# that doesn't see the outer cache. Mark these variables on
# `CMAKE_TRY_COMPILE_PLATFORM_VARIABLES` so they propagate into the inner
# project, and read them from the environment as a fallback so callers
# can use either `-DWINDOWS_…=…` or `export WINDOWS_…=…`.
set(_windows_msvc_required_vars
  WINDOWS_SDK_INCLUDE_ROOT
  WINDOWS_SDK_LIB_ROOT
  WINDOWS_CRT_INCLUDE_DIR
  WINDOWS_CRT_LIB_DIR
)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  ${_windows_msvc_required_vars}
  WINDOWS_TARGET_ARCH
)

foreach(var ${_windows_msvc_required_vars})
  if(NOT DEFINED ${var} AND DEFINED ENV{${var}})
    set(${var} "$ENV{${var}}")
  endif()
  if(NOT DEFINED ${var})
    message(FATAL_ERROR
      "${var} is required by the windows-msvc cross toolchain. "
      "See cmake/toolchains/windows-msvc.cmake for the expected layout "
      "and docs/cross-build.md for SDK-acquisition options.")
  endif()
endforeach()

if(NOT DEFINED WINDOWS_TARGET_ARCH)
  set(WINDOWS_TARGET_ARCH x86_64)
endif()

# Default to clang / clang++ on PATH. Callers can override before the
# project() call by setting CMAKE_C_COMPILER / CMAKE_CXX_COMPILER on the
# configure line.
if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER clang)
endif()
if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER clang++)
endif()

# Drive Windows tooling from system LLVM utilities. lld-link / llvm-rc /
# llvm-lib are required (host LLVM build doesn't always include lld).
# CMake's MSVC archive rule uses `<CMAKE_LINKER> /lib /OUT:foo.lib obj…`
# (when CMAKE_AR is unset) — setting CMAKE_AR forces a GNU `ar qc` style
# that llvm-lib rejects, so leave CMAKE_AR unset.
if(NOT CMAKE_LINKER)
  find_program(_lld_link NAMES lld-link REQUIRED)
  set(CMAKE_LINKER ${_lld_link})
endif()
if(NOT CMAKE_RC_COMPILER)
  find_program(_llvm_rc NAMES llvm-rc REQUIRED)
  set(CMAKE_RC_COMPILER ${_llvm_rc})
endif()

set(CMAKE_C_COMPILER_TARGET ${WINDOWS_TARGET_ARCH}-pc-windows-msvc)
set(CMAKE_CXX_COMPILER_TARGET ${WINDOWS_TARGET_ARCH}-pc-windows-msvc)

# Force lld-link via clang's `-fuse-ld=lld` so the host's default linker
# isn't picked up. CMake's LINKER_TYPE mechanism does the same on newer
# CMake versions; both are wired here for portability.
set(CMAKE_C_USING_LINKER_LLD "-fuse-ld=lld")
set(CMAKE_CXX_USING_LINKER_LLD "-fuse-ld=lld")
set(CMAKE_LINKER_TYPE LLD)

# `-Xclang -nostdsysteminc` suppresses clang's default Linux system header
# search (e.g. `/usr/include`) so xwin's CRT / Windows SDK headers aren't
# shadowed by glibc when host paths happen to leak into `-I`. Unlike
# `-nostdinc`, it keeps clang's *builtin* include path
# (`<resource-dir>/include`) reachable — that's where the SSE / AVX
# intrinsic wrappers (`xmmintrin.h`, `emmintrin.h`, …) live, and without
# them libpng's SIMD optimizations end up with undefined `_mm_*` symbols
# at link time.
set(_includes
  "-Xclang -nostdsysteminc"
  "-isystem ${WINDOWS_CRT_INCLUDE_DIR}"
  "-isystem ${WINDOWS_SDK_INCLUDE_ROOT}/ucrt"
  "-isystem ${WINDOWS_SDK_INCLUDE_ROOT}/um"
  "-isystem ${WINDOWS_SDK_INCLUDE_ROOT}/shared"
  "-isystem ${WINDOWS_SDK_INCLUDE_ROOT}/winrt"
)
string(JOIN " " _includes_str ${_includes})

set(_linker_libpaths
  "-Xlinker" "/libpath:${WINDOWS_CRT_LIB_DIR}"
  "-Xlinker" "/libpath:${WINDOWS_SDK_LIB_ROOT}/ucrt/${WINDOWS_TARGET_ARCH}"
  "-Xlinker" "/libpath:${WINDOWS_SDK_LIB_ROOT}/um/${WINDOWS_TARGET_ARCH}"
)
string(JOIN " " _linker_libpaths_str ${_linker_libpaths})

# Force the release-flavour CRT. Several SDK-acquisition tools (xwin
# included) only ship `msvcrt.lib`, not `msvcrtd.lib`, so the default
# debug-CRT pull breaks the configure-time try_compile.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

set(CMAKE_C_FLAGS_INIT "${_includes_str} -fms-compatibility -fms-extensions")
set(CMAKE_CXX_FLAGS_INIT "${_includes_str} -nostdinc++ -fms-compatibility -fms-extensions")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_linker_libpaths_str}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_linker_libpaths_str}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_linker_libpaths_str}")
set(CMAKE_STATIC_LINKER_FLAGS_INIT "")

# Cross-build: don't search the host filesystem for libraries or includes,
# only the toolchain-provided ones. Programs on the host are still
# reachable so tablegen / `find_program(DXC_EXECUTABLE …)` etc. can find
# their host-arch counterparts.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
