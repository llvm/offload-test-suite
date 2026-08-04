## Frequent Builder

The **frequent builder** is the CI application of the standalone build
distribution described in [offload-distribution.md](offload-distribution.md).
LLVM and DXC are built by two independent schedules:

| Schedule                  | Builds     | Cadence      | Reusable workflow          |
|---------------------------|------------|--------------|----------------------------|
| `frequent-build-llvm.yaml`| LLVM/Clang | every 2 hours| `build-llvm-callable.yaml` |
| `frequent-build-dxc.yaml` | DXC        | daily        | `build-dxc-callable.yaml`  |

Test cells consume both through `test-callable.yaml`.

The two are separate for **failure isolation**: with a single builder, a
broken LLVM `main` also withholds the DXC artifact, and vice versa. Split,
each component ages independently. The cadences follow how fast each
project moves. Over a recent week `llvm/llvm-project` `main` took about
86 commits a day while `Microsoft/DirectXShaderCompiler` `main` took about
0.1, so rebuilding DXC every two hours would nearly always reproduce the
same binaries.

PRs never build LLVM or DXC. The schedules publish the distribution as
artifacts; PR test cells resolve the newest successful run of each
schedule on `main` and download from it. Decoupling the build from the PR
means a broken LLVM or DXC `main` no longer blocks PR signal (cells keep
using the last good distribution), and no PR ever waits on a compile.

The trade-off is that PRs test against a toolchain up to one schedule
interval old, and a PR that changes the build configuration itself (for
example `StandaloneDistribution.cmake` or a builder's cmake args) is not
exercised by the test cells, because they consume a distribution built
from `main`'s configuration. Validate such a change by dispatching the
relevant schedule against the branch before merging.

Neither schedule has a `pull_request` trigger. Both run on a self-hosted
machine, and a fork PR must never execute code there: the builders produce
the binaries every test cell trusts, and they keep a persistent workspace
and sccache directory. Neither `schedule` nor `workflow_dispatch` can be
raised from a fork. `workflow_dispatch` accepts a branch, so maintainers
can still build a branch on demand.

### Motivation

`pr-matrix.yaml` fans out roughly twenty test cells per architecture
(SKU × backend × compiler). Under the `SplitBuild` model each of those
cells still kicked off a full LLVM/Clang/DXC compile, because the build ran
on the same SKU-labeled runner as the tests. The frequent builder removes
compilation from the PR path entirely: those N per-arch compiles become
two scheduled builds, shared by every PR that runs before the next ones.

Crucially, the builders' artifacts contain **no offload-test-suite code**.
Rebuilding the offload tools on the test runner means each test cell always
exercises the exact offload-suite HEAD of the PR rather than a
possibly-stale snapshot taken by the builder.

### Runner labels

| Label                   | Role |
|-------------------------|------|
| `hlsl-frequent-builder` | LLVM/Clang/DXC builds. No arch qualifier: in Phase 2 a single x64 box will produce both x64 and arm64 targets via cross-compile. |
| `hlsl-<sku>` (`windows-nvidia`, `windows-amd`, `windows-intel`, `windows-sw-rasterizer-x86_64`, `windows-qc`, `macos`) | Test execution against real hardware. |

Machines are strictly one or the other. Because GitHub Actions schedules a
job on the *intersection* of the requested labels, these disjoint label sets
mean a test job physically cannot land on the builder (it lacks the
`hlsl-<sku>` label the test job asks for), and vice versa. No yaml-level
guard is required.

### What the builder produces

`build-llvm-callable.yaml` configures llvm-project with
`cmake/caches/StandaloneDistribution.cmake` and runs
`ninja install-distribution`. `build-dxc-callable.yaml` builds DXC and
stages it out of its build directory (see "DXC prefix" in
[offload-distribution.md](offload-distribution.md)). Each
uploads one tarball per build config:

- `hlsl-dist-<os>-<arch>`: the LLVM/Clang install prefix.
- `hlsl-dxc-<os>-<arch>`: the DXC redistributable.

These names are deliberately **stable**: they carry no build-config hash,
so a PR can name the artifact it wants without knowing anything about how
the scheduled run was configured.

Stable names are safe because artifacts are scoped to a workflow run:
`(run id, name)` is the unique key, and `download-artifact@v4`'s `run-id`
supplies the first half. Successive scheduled builds all publish
`hlsl-dist-windows-x64` without colliding. The one invariant this relies
on is **at most one build job per `(OS, Arch)` per run**. Two jobs in the
same run sharing an `(OS, Arch)` would collide, since `upload-artifact@v4`
rejects duplicate names within a run. Phase 2's arm64 job is distinguished
by `<arch>`.

### Choosing which build to consume

PR test cells do not hardcode a run. A resolver job queries the API for
the newest successful run of each schedule and passes the two IDs to every
cell as `LlvmRunId` and `DxcRunId`, so all cells in a PR test against one
identical toolchain rather than some cells using one build and some
another.

The resolver accepts only `schedule` and `workflow_dispatch` runs
originating from this repository, and neither event can be raised from a
fork, so a fork PR cannot influence which binaries other PRs test against.

Incremental build speed comes from `sccache` on the builder, whose cache
directory persists across jobs. Measured across twelve runs of the
equivalent combined `SplitBuild` build on the same hardware, a build takes
roughly 19 minutes at the median and 34 at the worst, which is what the
LLVM job's 90-minute timeout and the 2-hourly cadence are sized against.
DXC is the smaller half and gets a 60-minute timeout.

### What the test runner does

`test-callable.yaml` downloads the LLVM prefix from a specified
`frequent-build-llvm.yaml` run and the DXC prefix from a specified
`frequent-build-dxc.yaml` run, using `download-artifact@v4`'s `run-id` +
`github-token` (which is why the job needs `actions: read`), extracts them,
checks out the offload-test-suite at the PR head plus an llvm-project
source tree (needed to supply LIT and the third-party unit testing
libraries, as described in
[offload-distribution.md](offload-distribution.md)), configures the
offload-test-suite as a standalone top-level CMake project against
`install/lib/cmake/llvm`, and builds `hlsl-test-depends`. It then runs
`ninja check-hlsl-unit` followed by
`ninja check-hlsl-<suite>`.

`clang-tidy` is part of the distribution, so the test runner enables
`OFFLOADTEST_USE_CLANG_TIDY` and points `CLANG_TIDY` at the copy in the
install prefix. This preserves the lint coverage that used to come from the
in-tree build.

Unlike the `SplitBuild` flow, this path does **not** use
`configure-test-suite.py` or the installed `share/hlsl-test-suite` tree.
Lit configuration is generated by the standalone CMake build. Test runners
therefore do need CMake, Ninja and a host toolchain, which they already had
under the previous model.

### Failure semantics

A failed scheduled build does **not** affect PRs. Test cells resolve the
newest *successful* run of each schedule, so a broken LLVM or DXC `main`
leaves PRs testing against the last good distribution instead of turning
the whole matrix red. Because the two are separate, a broken LLVM
`main` does not hold back a fresh DXC. The cost is that the failure is
silent from a PR's point of view: the distribution simply ages until
someone notices the scheduled workflow is red. Treat both schedules as
monitored workflows in their own right, not as PR signal.

Two further failure modes surface in a PR:

- **No successful run exists** (nothing has been built yet, or every run
  since the workflow landed has failed). The resolver fails with an
  actionable message naming the schedule; dispatch it once to seed it.
- **Artifacts have expired.** Retention is sized to outlive a long run of
  failures: LLVM artifacts are kept 3 days at a 2-hourly cadence (~36
  successive builds would have to fail) and DXC artifacts 14 days at a
  daily cadence (~14 builds). If that happens the resolver succeeds and
  the download step is what fails.

A stuck build cannot block the schedule. The LLVM build job sets
`timeout-minutes: 90`, well above the ~34 minute worst case, and the DXC
job 60, so a stuck build is killed rather than holding the machine. Each
job's concurrency group is keyed on ref, OS and arch with
`cancel-in-progress: false`: only one build per target runs at a time,
GitHub keeps at most one more waiting, and a build that is merely queued
is never discarded. Cancelling queued builds would be wrong here, because
the builder shares a pool with legacy `SplitBuild` jobs and a wait can
legitimately exceed the schedule interval.
