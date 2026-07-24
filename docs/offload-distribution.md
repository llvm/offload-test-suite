# Offload Test Suite Distribution

The offload test suite supports a *split build / test* mode where binaries
and test data are built on one machine, packaged as portable install
prefixes, and consumed on a different machine to run the lit tests. This
is the model used by the `SplitBuild=true` path of the
`build-and-test-callable.yaml` workflow.

This document describes the install layout, prerequisites, and how to
configure and run the suite from an installed prefix.

## Components

A complete deployment consists of two install prefixes:

1. **LLVM + OffloadTest prefix** — produced by the `install-distribution`,
   `install-offload-tools`, and `install-offload-test-suite` targets. Built
   with `clang/cmake/caches/HLSL.cmake` and `HLSL_ENABLE_OFFLOAD_DISTRIBUTION=On`.
   Contains:
   - `bin/` — clang, FileCheck, split-file, not, obj2yaml, api-query,
     offloader, and the other tools created by `add_offloadtest_tool`.
   - `include/`, `lib/clang/<ver>/include/` — clang resource headers
     (`hlsl-resource-headers` component).
   - `share/hlsl-test-suite/` — test sources, `lit.site.cfg.py.in` template,
     `configure-test-suite.py`, and golden images.

2. **DXC prefix** — a hand-curated `bin/` + `lib/` tree containing just
   the DXC binaries the test runner needs. Kept separate from the LLVM
   prefix to avoid header / binary conflicts between Clang's HLSL-related
   headers and DXC's. Contents:

   Windows:
   - `bin/dxc.exe`, `bin/dxv.exe` — DXC compiler and validator.
   - `bin/dxcompiler.dll` — DXC's compiler shared library (placed
     next to the executables so Windows' DLL search finds it via the
     app-directory rule, no `PATH` munging required).
   - `bin/dxil.dll` — DXC's signing / validation library.
   - `bin/dxc.pdb`, `bin/dxv.pdb`, `bin/dxcompiler.pdb`, `bin/dxil.pdb` —
     debug symbol files (PDBs) shipped alongside their corresponding
     binaries so crash dumps and live debugger sessions can resolve
     symbols. The Windows debugger looks for `.pdb` files next to the
     `.exe` / `.dll`, so no symbol-server configuration is required.
     PDBs are shipped only when the build config produces them (any
     config that enables `/Zi`, e.g. `Debug` or `RelWithDebInfo`); a
     pure `Release` build omits them and the distribution simply has
     no PDBs.
   - `lib/dxcompiler.lib`, `lib/dxil.lib` — Windows import libraries
     for downstream consumers that link against the DLLs.

   Linux / macOS:
   - `bin/dxc`, `bin/dxv`.
   - `lib/libdxcompiler.{so,dylib}`, `lib/libdxil.{so,dylib}` — the
     binaries have RUNPATH set to find them via `../lib`.

   We don't ship DXC's `cmake --install` output. A top-level `ninja
   install` walks every `cmake_install.cmake`, including LLVM tools
   (e.g. `llvm-as`) that aren't built by the default `ninja` target, so
   it fails. The per-component install targets (`install-dxc`,
   `install-dxcompiler`) work but only cover a subset of the files we
   need: `dxv` has no `install-dxv` custom target, `dxil` has no install
   rule at all (it's a prebuilt signing library bundled with DXC source),
   and the Windows import libraries (`.lib`) aren't installed either.
   Instead we copy everything we need straight out of the build
   directory's `bin/` and `lib/`.

## Building

From a configured llvm-project build tree that has OffloadTest enabled as
an external project:

```
cmake -G Ninja \
    -DHLSL_ENABLE_OFFLOAD_DISTRIBUTION=On \
    -DLLVM_EXTERNAL_PROJECTS=OffloadTest \
    -C llvm-project/clang/cmake/caches/HLSL.cmake \
    -DLLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR=<path-to-offload-test-suite> \
    -DCMAKE_INSTALL_PREFIX=<install-prefix> \
    <other LLVM/HLSL flags...> \
    llvm-project/llvm

ninja install-distribution install-offload-tools install-offload-test-suite

# See "DXC prefix" above for why we copy from the build folder instead
# of using `cmake --install` or per-component install targets.
```

Then assemble the DXC prefix by copying the relevant files out of
`<dxc-build>` into a clean `<dxc-dist>` tree, e.g. on Windows:

```
<dxc-build>/bin/dxc.exe         -> <dxc-dist>/bin/
<dxc-build>/bin/dxv.exe         -> <dxc-dist>/bin/
<dxc-build>/bin/dxcompiler.dll  -> <dxc-dist>/bin/
<dxc-build>/bin/dxil.dll        -> <dxc-dist>/bin/
<dxc-build>/bin/dxc.pdb         -> <dxc-dist>/bin/
<dxc-build>/bin/dxv.pdb         -> <dxc-dist>/bin/
<dxc-build>/bin/dxcompiler.pdb  -> <dxc-dist>/bin/
<dxc-build>/bin/dxil.pdb        -> <dxc-dist>/bin/
<dxc-build>/lib/dxcompiler.lib  -> <dxc-dist>/lib/
<dxc-build>/lib/dxil.lib        -> <dxc-dist>/lib/
```

The HLSL.cmake cache file enforces that `OffloadTest` is in
`LLVM_EXTERNAL_PROJECTS` when `HLSL_ENABLE_OFFLOAD_DISTRIBUTION` is on; the
configure step will fail fast otherwise. Note that
`-DHLSL_ENABLE_OFFLOAD_DISTRIBUTION=On` and `-DLLVM_EXTERNAL_PROJECTS=OffloadTest`
must appear on the `cmake` command line *before* the `-C HLSL.cmake` argument
so they're set in the cache before the cache script runs.

## Packaging

Tar each prefix and ship the tarballs to the test runner:

```
tar cf llvm-prefix.tar -C <install-prefix> .
tar cf dxc-prefix.tar  -C <dxc-dist>       .
```

## Test runner prerequisites

- Python 3.6+
- `pip install lit pyyaml`
- GPU drivers appropriate for the suite (D3D12 / Vulkan / Metal).
- The two extracted prefixes from the build runner.

No CMake, ninja, or compiler toolchain is required on the test runner.

## Configuring and running

Extract both prefixes, then run the configure script with `--dxc-path`
pointing at the DXC binary in its prefix:

```
rm -rf install dxc-dist
mkdir install dxc-dist
tar xf llvm-prefix.tar -C install
tar xf dxc-prefix.tar  -C dxc-dist

cd install/share/hlsl-test-suite
python configure-test-suite.py \
    --suite clang-d3d12 \
    --dxc-path ../../../dxc-dist/bin/dxc[.exe]
```

> **Always extract into freshly-cleaned prefixes.** `tar xf` overlays files
> onto whatever already exists and never removes orphans, so on a reused
> working directory (e.g. a persistent self-hosted runner) stale `.test`
> files from a previous build would survive and be run by lit. The `rm -rf`
> above guarantees lit sees only the tests in this tarball. The build runner
> likewise wipes its `install`/`dxc-dist` prefixes before installing.

This emits a fully-substituted `run/test/<suite>/lit.site.cfg.py`.

`dxc[.exe]` finds its runtime libraries automatically: on Windows the
DLLs sit next to the executable in `bin/`, and on Linux/macOS the
binaries have RUNPATH set to locate `../lib`. No `PATH` /
`LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` setup is required.

Then run lit:

```
lit -v run/test/clang-d3d12
```

`configure-test-suite.py --list-suites` prints the available suite names.

## Suites

| Name              | Backend     | Compiler |
|-------------------|-------------|----------|
| `d3d12`           | DirectX 12  | DXC      |
| `vk`              | Vulkan      | DXC      |
| `mtl`             | Metal       | DXC      |
| `warp-d3d12`      | WARP (D3D12)| DXC      |
| `clang-d3d12`     | DirectX 12  | Clang    |
| `clang-vk`        | Vulkan      | Clang    |
| `clang-mtl`       | Metal       | Clang    |
| `clang-warp-d3d12`| WARP (D3D12)| Clang    |

Clang suites do not require DXC; you can omit `--dxc-path` for those.

## CI usage

The reusable workflow `.github/workflows/build-and-test-callable.yaml`
implements this flow when invoked with `SplitBuild=true`. The build job
produces two artifacts (`build-<sku>-<target>` and `dxc-<sku>-<target>`)
and the test job consumes both.

## Standalone Build Distribution

An alternate approach for separating the build and test flow in the offload test
suite is using the "standalone" build mode. With this build flow, LLVM (and
optionally Clang) are built separately from the offload-test suite, and the
offload-test-suite is configured as the top-level CMake entry.

A sample configuration for this flow is provided in the
`cmake/caches/StandaloneDistribution.cmake` cache script. In this build
configuration, the LLVM build contributes Clang, the llvm testing tools, and the
subset of LLVM component libraries that the offload-test-suite's tools depend
on.

Using this flow LLVM and Clang are configured using a command like:

```
cmake -C <offload test>/cmake/caches/StandaloneDistribution.cmake \
      -DCMAKE_INSTALL_PREFIX=<path to install to>                 \
      <other cmake options> <path to llvm>
ninja install-distribution
```

Then configure and build the offload test suite with a command like so:

```
cmake -DCMAKE_PREFIX_PATH=<path to llvm install>/lib/cmake/llvm \
      -DLLVM_MAIN_SRC_DIR=<path to llvm-project>/llvm           \
      -DDXC_DIR=<path to folder containing dxc/dxv>             \
      -DOFFLOAD_TEST_TEST_CLANG=On                              \
      -DGOLDENIMAGE_DIR=<path to images>                        \
      <other cmake options> <path to ofload test suite>
ninja check-hlsl
```

In this configuration the offload-test-suite will build its tools against the
already built LLVM libraries which dramatically reduces build time. This
configuration does still require a checkout of the LLVM source tree to pull LIT
and the third-party unit testing libraries. If clone/checkout time or disk space
is a concern this could be a sparse checkout or future changes could allow this
to use LIT from pip and a stock googletest framework.
