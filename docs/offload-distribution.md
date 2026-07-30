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

## Frequent Builder

The **frequent builder** is the CI application of the standalone build
distribution described above, implemented by `frequent-build.yaml` (the
schedule) and the reusable workflows `build-callable.yaml` and
`test-callable.yaml`.

PRs never build LLVM or DXC. `frequent-build.yaml` runs on a schedule and
publishes the distribution as artifacts; PR test cells resolve the newest
successful run of that workflow on `main` and download from it. Decoupling
the build from the PR means a broken LLVM or DXC `main` no longer blocks
PR signal — cells keep using the last good distribution — and no PR ever
waits on a compile.

The trade-off is that PRs test against a toolchain up to one schedule
interval old, and a PR that changes the build configuration itself (for
example `StandaloneDistribution.cmake` or the builder's cmake args) is not
exercised by the test cells, because they consume a distribution built
from `main`'s configuration. `frequent-build.yaml` therefore also triggers
on pull requests that touch those files, so builder changes are still
validated by the PR that makes them.

### Rollout status

The builder is being brought up incrementally:

- **Today.** `frequent-build.yaml` runs the builder on a 4-hourly schedule
  and publishes artifacts, and **nothing consumes them**. Every test cell
  still builds for itself through `build-and-test-callable.yaml`, exactly
  as before. Running the builder unconsumed lets us watch build
  reliability, sccache hit rate, cache-key churn and artifact size without
  putting PR signal at risk.
- **Validating the consuming half.** The download and test-execution code
  is exercised by dispatching `validate-frequent-builder.yaml` manually. It
  resolves a frequent build and runs `test-callable.yaml` for one chosen
  x64 SKU and lit suite, covering run resolution, artifact download, the
  standalone rebuild and the lit run — the exact path a migrated pr-matrix
  cell takes. Compare its results against the same SKU/TestTarget in a
  normal `pr-matrix.yaml` run.
- **Next.** Once the builder is consistently green and
  `validate-frequent-builder.yaml` passes for a SKU, uncomment the
  `Resolve-Frequent-Build` job and that SKU's cells in the
  `Exec-Tests-Windows-Distributed` template in `pr-matrix.yaml`, deleting
  the corresponding legacy cells. A SKU is served by exactly one path at a
  time, never both, and migration proceeds one SKU at a time so a
  regression is always attributable and trivially revertable. The template
  enumerates cells explicitly as `{ SKU, Arch, TestTarget }` rather than as
  a cross-product, so "which build feeds which test" stays readable from
  `pr-matrix.yaml` alone.
- **Then the scheduled workflows.** `pr-matrix.yaml` is not the only caller
  of `SplitBuild=true`; the 19 scheduled per-SKU workflows
  (`windows-amd-*`, `windows-intel-*`, `windows-nvidia-*`) and
  `validate-split-build-test.yaml` use it too. Their build jobs target the
  generic `["self-hosted", "Windows", "X64"]` pool, which the frequent
  builder is necessarily a member of, so until they migrate they will keep
  landing foreign builds on it. The builder is only truly dedicated once
  no caller sets `SplitBuild=true`.
- **Phase 2.** `build-callable.yaml` gains arm64 cross-compile support and
  `windows-qc` migrates too.

### Motivation

`pr-matrix.yaml` fans out roughly twenty test cells per architecture
(SKU × backend × compiler). Under the `SplitBuild` model each of those
cells still kicked off a full LLVM/Clang/DXC compile, because the build ran
on the same SKU-labeled runner as the tests. The frequent builder removes
compilation from the PR path entirely: those N per-arch compiles become a
single scheduled build, amortised across every PR that runs before the
next one.

Crucially, the builder's artifact contains **no offload-test-suite code**.
Rebuilding the offload tools on the test runner is what keeps the
offload-suite SHA out of the cache key: a hundred consecutive
offload-test-suite PRs that don't bump the LLVM or DXC pins all hit the
same cached distribution. It also means each test cell always exercises the
exact offload-suite HEAD of the PR rather than a possibly-stale snapshot
taken by a cached builder run.

### Runner labels

| Label                   | Role |
|-------------------------|------|
| `hlsl-frequent-builder` | LLVM/Clang/DXC builds. No arch qualifier — in Phase 2 a single x64 box will produce both x64 and arm64 targets via cross-compile. |
| `hlsl-<sku>` (`windows-nvidia`, `windows-amd`, `windows-intel`, `windows-sw-rasterizer-x86_64`, `windows-qc`, `macos`) | Test execution against real hardware. |

Machines are strictly one or the other. Because GitHub Actions schedules a
job on the *intersection* of the requested labels, these disjoint label sets
mean a test job physically cannot land on the builder — it lacks the
`hlsl-<sku>` label the test job asks for — and vice versa. No yaml-level
guard is required.

### What the builder produces

`build-callable.yaml` configures llvm-project with
`cmake/caches/StandaloneDistribution.cmake` and runs
`ninja install-distribution`, then stages DXC out of its build directory
(see "DXC prefix" above). It uploads two tarballs per build config:

- `hlsl-dist-<os>-<arch>` — the LLVM/Clang install prefix.
- `hlsl-dxc-<os>-<arch>` — the DXC redistributable.

These names are deliberately **stable** — they carry no build-config hash,
so a PR can name the artifact it wants without knowing anything about how
the scheduled run was configured.

Stable names are safe because artifacts are scoped to a workflow run:
`(run id, name)` is the unique key, and `download-artifact@v4`'s `run-id`
supplies the first half. Successive scheduled builds all publish
`hlsl-dist-windows-x64` without colliding. The one invariant this relies
on is **at most one build job per `(OS, Arch)` per run** — two jobs in the
same run sharing an `(OS, Arch)` would collide, since `upload-artifact@v4`
rejects duplicate names within a run. Phase 2's arm64 job is distinguished
by `<arch>`.

### Choosing which build to consume

PR test cells do not hardcode a run. A resolver job queries the API for
the newest successful `frequent-build.yaml` run and passes its ID to every
cell as `BuildRunId`, so all cells in a PR test against one identical
toolchain rather than straddling a build boundary.

The resolver accepts only `schedule` and `workflow_dispatch` runs
originating from this repository. Filtering on `branch=main` alone is
**not** sufficient: a pull request from a fork whose branch is named
`main` also matches that filter, which would let a fork PR decide which
binaries every other PR is tested against. Both events the resolver does
accept can only be raised inside the base repository.

Cross-run *build* reuse is handled separately by `actions/cache@v4` under
the **fully qualified key**:

```
hlsl-dist-<os>-<arch>-<llvm-sha12>-<dxc-sha12>-<cfg12>
```

where `<cfg12>` is the first 12 hex characters of a SHA-256 over the build
type, the extra cmake args, and the contents of
`StandaloneDistribution.cmake`. On a cache hit the builder skips the
checkout-dependent build steps entirely and re-uploads the cached tarballs
as this run's artifacts. On a miss it does a full build (still
sccache-accelerated), populates the cache, and uploads.

### What the test runner does

`test-callable.yaml` downloads both prefixes from a specified
`frequent-build.yaml` run using `download-artifact@v4`'s `run-id` +
`github-token` (which is why the job needs `actions: read`), extracts them,
checks out the offload-test-suite at the PR head plus an llvm-project
source tree (needed for the reasons given above), configures the
offload-test-suite as a standalone top-level CMake project against
`install/lib/cmake/llvm`, and builds `hlsl-test-depends`. That build is
well under a minute. It then runs `ninja check-hlsl-unit` followed by
`ninja check-hlsl-<suite>`.

`clang-tidy` is part of the distribution, so the test runner enables
`OFFLOADTEST_USE_CLANG_TIDY` and points `CLANG_TIDY` at the copy in the
install prefix. This preserves the lint coverage that used to come from the
in-tree build.

Unlike the `SplitBuild` flow, this path does **not** use
`configure-test-suite.py` or the installed `share/hlsl-test-suite` tree —
lit configuration is generated by the standalone CMake build. Test runners
therefore do need CMake, Ninja and a host toolchain, which they already had
under the previous model.

### Failure semantics

A failed scheduled build does **not** affect PRs. Test cells resolve the
newest *successful* `frequent-build.yaml` run, so a broken LLVM or DXC
`main` leaves PRs testing against the last good distribution instead of
turning the whole matrix red. The cost is that the failure is silent from
a PR's point of view: the distribution simply ages until someone notices
the scheduled workflow is red. Treat `frequent-build.yaml` as a monitored
workflow in its own right, not as PR signal.

Two failure modes do surface in a PR, both from the resolver job:

- **No successful run exists** (nothing has been built yet, or every run
  since the workflow landed has failed). The resolver fails with an
  actionable message; dispatch `frequent-build.yaml` once to seed it.
- **Artifacts have expired.** Artifacts are retained for 7 days, so at a
  4-hourly cadence roughly 40 successive builds would have to fail before
  the newest successful run's artifacts vanish. If that happens the
  resolver succeeds and the download step is what fails.

### Scope

Phase 1 covers Windows x64 only. `build-callable.yaml` hard-fails on any
other `OS`/`Arch` combination.

`windows-qc` is the only arm64 SKU, so `SplitBuild` never bought it
anything — its build always ran on itself. It stays on
`build-and-test-callable.yaml` until Phase 2.

macOS is deliberately out of scope: builds are fast enough on the mac
runner that the extra plumbing isn't worth it, and there is no
arm64/x64 cross-compile question. `Exec-Tests-MacOS` continues to invoke
`build-and-test-callable.yaml` directly.

### Provisioning a frequent-builder machine

1. The runner has the `hlsl-frequent-builder` label and **no** `hlsl-<sku>`
   label.
2. The runner service runs as a user with write access to the workspace and
   the sccache directory, and may run `VsDevCmd.bat`.
3. Visual Studio 2022 with the C++ x64 build tools.
4. `sccache` on `PATH`, with a cache directory that persists across jobs.
5. Python 3.11+ on `PATH`.
6. CMake 3.31+ and Ninja on `PATH`.
7. Git on `PATH` with credentials to clone llvm-project, DXC and
   offload-test-suite.
