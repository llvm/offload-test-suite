# Cross-building OffloadTest on Linux for Windows / Wine

`cmake/toolchains/windows-msvc.cmake` is a CMake toolchain file that lets a non-Windows host (typically Linux) build `offloader.exe` and any other targets in the project against the x86_64 Windows MSVC ABI. The cross-built binary can then be run under Wine + vkd3d-proton on Linux for an end-to-end D3D12 / Vulkan smoke loop without dual-booting or a Windows VM.

This is currently a contributor-iteration aid — no in-tree CI runs this configuration today. Treat it as a "really experimental, YMMV" workflow until the wg-hlsl support-tier discussion lands a place for it in `CI.md`.

## What you need

- A recent **clang** with both Linux and Windows-MSVC targets. Distro clang ≥ 17 works; bleeding-edge from an LLVM build is also fine. `clang++` and `clang` must be on `PATH` (or pointed at via `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER`).
- **lld-link**, **llvm-rc**, **llvm-lib** on `PATH`. `lld-link` is the cross linker; the others handle Windows resource compilation and import-library generation. Distro `llvm` package usually ships all three.
- A **Windows SDK + CRT tree** (see acquisition options below).
- For runtime testing: **Wine** and **vkd3d-proton** (D3D12) or **wine-vulkan** (Vulkan). `binfmt_misc` registration for `.exe` (the `wine-binfmt` package on Arch, or the equivalent on your distro) lets the `.exe` run as if it were a native binary from any shell — no explicit `wine` prefix needed.

## Acquiring the Windows SDK + CRT

The toolchain is agnostic about *how* you get the SDK headers / libs / CRT. Four common paths, none preferred:

- **[xwin](https://github.com/Jake-Shadle/xwin)** — Rust tool that downloads the MSVC build tools via the official Visual Studio installer manifests and extracts a flat tree of headers and libs. Lightweight, single-command setup. Output is normally at `~/.xwin-cache/splat/`.
- **[msvc-wine](https://github.com/mstorsjo/msvc-wine)** — installs the actual Visual Studio Build Tools through Wine and scripts up wrapper compilers. Heavier than xwin but uses official Microsoft bits end-to-end. Widely used by FFmpeg / mpv / Mesa projects.
- **EWDK (Enterprise WDK)** — Microsoft's portable SDK + Build Tools ZIP. Mount it on Linux, set env vars, point CMake at the headers and libs. Licensed but free, no Wine involved.
- **Manually copied from a Windows install of Visual Studio.** `rsync C:\Program Files (x86)\Windows Kits\10\Include\10.0.x.0\{um,shared,ucrt}` and the matching `Lib\…` to a Linux directory. Works if you've got a Windows machine around once for setup.

Whichever you use, point the toolchain at the resulting tree with these four variables:

| Variable                   | What it contains                                                       |
|----------------------------|------------------------------------------------------------------------|
| `WINDOWS_SDK_INCLUDE_ROOT` | Parent of `um/`, `shared/`, `ucrt/`, `winrt/` SDK headers              |
| `WINDOWS_SDK_LIB_ROOT`    | Parent of `ucrt/x86_64/` and `um/x86_64/` SDK libs                     |
| `WINDOWS_CRT_INCLUDE_DIR` | MSVC CRT headers (`stdint.h`, `cwchar`, …)                             |
| `WINDOWS_CRT_LIB_DIR`    | MSVC CRT libs for the target arch (`msvcrt.lib`, `vcruntime.lib`, …)   |

The toolchain file's header comment maps each variable to the corresponding directory in xwin / msvc-wine / EWDK trees.

## Configure + build

Two-stage build: a native host build that produces tablegen helpers + the host LLVM, then a cross build that targets Windows MSVC and consumes the host tablegens.

### Stage 1 — host (Linux) build

A normal LLVM + OffloadTest build. Produces `llvm-tblgen`, `clang-tblgen`, and the host `offloader` / `clang` / `dxc` you'd normally use for the Linux Vulkan lit suite.

```bash
cmake -S llvm-project/llvm -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS='clang;clang-tools-extra' \
  -DLLVM_EXTERNAL_PROJECTS=OffloadTest \
  -DLLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR=$PWD/OffloadTest
cmake --build build --target offloader
```

### Stage 2 — Windows-MSVC cross build

Uses the toolchain file from this PR, points `LLVM_TABLEGEN` / `CLANG_TABLEGEN` at the stage-1 binaries, and disables Vulkan / Metal (Linux Vulkan headers would shadow the xwin SDK include path via `find_package(Vulkan)`; Metal is macOS-only). `D3D12_INCLUDE_DIRS` / `D3D12_LIBRARIES` are passed directly because `FindD3D12.cmake` keys off a Win10 SDK registry path that doesn't match any of the cross-SDK layouts.

```bash
cmake -S llvm-project/llvm -B build-win -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=$PWD/OffloadTest/cmake/toolchains/windows-msvc.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TABLEGEN=$PWD/build/bin/llvm-tblgen \
  -DCLANG_TABLEGEN=$PWD/build/bin/clang-tblgen \
  -DLLVM_ENABLE_PROJECTS=clang \
  -DLLVM_EXTERNAL_PROJECTS=OffloadTest \
  -DLLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR=$PWD/OffloadTest \
  -DLLVM_TARGETS_TO_BUILD='X86;SPIRV' \
  -DOFFLOADTEST_TEST_CLANG=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=TRUE \
  -DOFFLOADTEST_ENABLE_VULKAN=OFF \
  -DOFFLOADTEST_ENABLE_METAL=OFF \
  -DOFFLOADTEST_ENABLE_D3D12=ON \
  -DD3D12_INCLUDE_DIRS=$HOME/.xwin-cache/splat/sdk/include/um \
  -DD3D12_LIBRARIES='d3d12.lib;dxcore.lib;dxguid.lib;d3dcompiler.lib' \
  -DWINDOWS_SDK_INCLUDE_ROOT=$HOME/.xwin-cache/splat/sdk/include \
  -DWINDOWS_SDK_LIB_ROOT=$HOME/.xwin-cache/splat/sdk/lib \
  -DWINDOWS_CRT_INCLUDE_DIR=$HOME/.xwin-cache/splat/crt/include \
  -DWINDOWS_CRT_LIB_DIR=$HOME/.xwin-cache/splat/crt/lib/x86_64 \
  -DPNG_INTEL_SSE=OFF
cmake --build build-win --target offloader
```

Adjust the four `WINDOWS_…` paths if you're using msvc-wine / EWDK / a hand-copy instead of xwin.

`PNG_INTEL_SSE=OFF` is needed because libpng's hand-rolled SSE2 wrapper functions emit external `_mm_*` symbols that `lld-link` doesn't resolve under cross-MSVC (the intrinsic *headers* are reachable, but the inline-resolution path differs). Disabling its SIMD codegen leaves the C fallback, which is fine for a test runner.

## Running the cross-built binary

With `binfmt_misc` configured for `.exe`:

```bash
./build-win/bin/offloader.exe --api dx pipeline.yaml shader.o
```

Without `binfmt_misc`:

```bash
wine ./build-win/bin/offloader.exe --api dx pipeline.yaml shader.o
```

Both routes pick up the host's Wine prefix, so `WINEPREFIX` / `WINEDEBUG` / `VKD3D_*` / `DXVK_*` environment variables work the same as they would for any other Wine binary.

## Running the lit suite

The lit configuration the native build uses works from the cross-build directory unchanged — `add_offloadtest_lit_suite` resolves `%offloader` to `$<TARGET_FILE:offloader>`, which for the cross build is the cross-compiled `.exe`. With `binfmt_misc` in place lit invokes the binary like any other test program and Wine takes over transparently:

```bash
ninja -C build-win check-hlsl-d3d12
```

`WINEDEBUG=-all` and `VKD3D_CONFIG=…` flow through the environment exactly as for a regular Wine invocation.

If `binfmt_misc` for `.exe` isn't registered on the host, lit can't directly invoke the cross-built binary — there is no in-tree launcher hook today. Registering the binfmt entry once (most distros' `wine-binfmt` package does this for you) avoids needing one.

## Gotchas

- **Don't set `CMAKE_AR`.** The toolchain deliberately leaves it unset so CMake's MSVC-mode archive rule picks the right `<CMAKE_LINKER> /lib /OUT:foo.lib obj…` invocation. Setting `CMAKE_AR=llvm-lib` forces CMake to a GNU `ar qc …` style that `llvm-lib` rejects.
- **`find_package(Vulkan)` on the host finds Linux Vulkan.** The Linux SDK's `/usr/include` then shadows the xwin SDK headers when the discovered Vulkan include path makes it into any target's `-I`. Pass `-DCMAKE_DISABLE_FIND_PACKAGE_Vulkan=TRUE` to suppress it on the cross build (the offloader builds fine without Vulkan support on the cross side; Wine routes the cross-built `vulkan-1.dll` to the host's Vulkan loader at runtime regardless).
- **`-Xclang -nostdsysteminc`, not `-nostdinc`.** `-nostdinc` also strips clang's *builtin* include directory, which is where the SSE / AVX intrinsic wrappers live; libpng then fails to link with undefined `_mm_*` symbols. `-Xclang -nostdsysteminc` keeps the builtin path while suppressing the default Linux system search.
- **xwin / msvc-wine only ship the release-flavour CRT.** The default debug-CRT (`msvcrtd.lib`) is missing from those trees, so `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL` and `CMAKE_TRY_COMPILE_CONFIGURATION=Release` are set by the toolchain. If your SDK source ships both, you can override these in a follow-up `-D…` after the toolchain file loads.

