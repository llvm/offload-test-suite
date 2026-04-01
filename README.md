# OffloadTest
Experimental Runtime test suite for HLSL

## Current Status

| Testing Machine | DXC | Clang |
|-----------------|-----|-------|
| **Tier 1 Targets** |
| Windows DirectX12 Intel GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-d3d12.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-d3d12.yaml) |
| Windows DirectX12 Warp (x64 LKG) | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-dxc-warp-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-dxc-warp-d3d12.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-clang-warp-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-clang-warp-d3d12.yaml) |
| Windows DirectX12 Warp (arm64 LKG) | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-dxc-warp-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-dxc-warp-d3d12.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-clang-warp-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-clang-warp-d3d12.yaml) |
| Windows Vulkan Intel GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-vk.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-vk.yaml) |
| **Tier 2 Targets** |
| macOS Apple M1 | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/macos-dxc-mtl.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/macos-dxc-mtl.yaml) | [![Clang & DXC](https://github.com/llvm/offload-test-suite/actions/workflows/macos-clang-mtl.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/macos-clang-mtl.yaml) |
| **Experimental Targets** |
| Windows DirectX12 AMD GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-dxc-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-dxc-d3d12.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-clang-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-clang-d3d12.yaml) |
| Windows DirectX12 NVIDIA GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-dxc-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-dxc-d3d12.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-clang-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-clang-d3d12.yaml) |
| Windows DirectX12 Qualcomm GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-dxc-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-dxc-d3d12.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-clang-d3d12.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-clang-d3d12.yaml) |
| Windows Vulkan AMD GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-dxc-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-dxc-vk.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-clang-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-amd-clang-vk.yaml) |
| Windows Vulkan NVIDIA GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-dxc-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-dxc-vk.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-clang-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-nvidia-clang-vk.yaml) |
| Windows Vulkan Qualcomm GPU | [![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-dxc-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-dxc-vk.yaml) | [![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-clang-vk.yaml/badge.svg)](https://github.com/llvm/offload-test-suite/actions/workflows/windows-qc-clang-vk.yaml) |

See the [Continuous Integration](docs/CI.md) documentation for the description of support tiers and builder hardware.

# Prerequisites

Requires the Vulkan 1.4 SDK.

This project requires being able to locally build LLVM and leverages LLVM's build infrastructure. It also requires installing the `pyyaml` Python package. You can install `pyyaml` by running:

```shell
pip3 install pyyaml
```

On Windows, the [Graphics Tools](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-environment-set-up#debug-layer) optional feature is additionally required to run the test suite.

# Building

The LLVM project provides a CMake cache file,
[`clang/cmake/caches/HLSL.cmake`](https://github.com/llvm/llvm-project/blob/main/clang/cmake/caches/HLSL.cmake),
that configures the required projects and targets for HLSL development. You can
use it with `-C` to set up a build that includes the offload test suite:

```shell
cmake -G Ninja -Bbuild \
  -C <path to llvm-project>/clang/cmake/caches/HLSL.cmake \
  -C <path to OffloadTest>/cmake/caches/OffloadTest.cmake \
  <path to llvm-project>/llvm
```

The `OffloadTest.cmake` cache file automatically sets
`LLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR` and `LLVM_EXTERNAL_PROJECTS` based on its
location in the source tree. If you already have an LLVM build configured, you
can add the offload test suite to it by passing the same `-C` flag or by adding
the following to your CMake options:

```shell
-DLLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR=<path to OffloadTest> -DLLVM_EXTERNAL_PROJECTS="OffloadTest"
```

If you do not have a build of dxc on your path you'll need to specify the shader
compiler to use by passing:

```shell
-DDXC_DIR=<path to folder containing dxc & dxv>
```

## Running Tests

```shell
cmake --build build --target check-hlsl
```

The `check-hlsl` target builds all required tools and runs the full test suite.
You can also run tests for a specific platform with `check-hlsl-<platform>`
(e.g. `check-hlsl-vk`, `check-hlsl-d3d12`). To only run `clang`-based tests
(without requiring DXC), use `check-hlsl-clang-<platform>` (e.g.
`check-hlsl-clang-mtl`). Subdirectories of the test suite are also available as
targets with `check-hlsl-<platform>-<path>` where the path is lowercased with
directory separators replaced by `-` (e.g. `check-hlsl-d3d12-feature-hlsllib`).

## Enabling clang-tidy

The offload test suite's code is clang-tidy clean for a limited ruleset.
If you have clang-tidy installed locally you can enable clang-tidy by adding `-DOFFLOADTEST_USE_CLANG_TIDY=On` to your CMake invocation.
You can also add `-DOFFLOADTEST_CLANG_TIDY_APPLY_FIX=On` to enable automatically applying the clang-tidy fix-its for any warnings that have automated fixes.

# Failing tests

Tests which are failing can be prevented from running using `XFAIL` and `UNSUPPORTED`. When `XFAIL`ing a test make sure to add a comment above
linking the appropriate issue and whether the failure is due to a bug or an unimplemented feature.

```
# Bug/Unimplemented <link to issue>
# XFAIL: Clang && Vulkan
```

# YAML Pipeline Format

This framework provides a YAML representation for describing GPU pipelines and buffers. The format is implemented by the `API/Pipeline.{h|cpp}` sources. The following is an example pipeline YAML description:

```yaml
---
Shaders:
  - Stage: Compute
    Entry: main
    DispatchSize: [1, 1, 1]
Buffers:
  - Name: Constants
    Format: Int32
    Data: [ 1, 2, 3, 4, 5, 6, 7, 8]
  - Name: In1
    Format: Float32
    Data: [ 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8]
  - Name: In2
    Format: Hex16
    Data: [ 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8]
  - Name: Tex
    Format: Float32
    Channels: 4
    OutputProps:
      Width: 2
      Height: 2
      Depth: 1
      MipLevels: 2
    Data: [ 1.0, 0.0, 0.0, 1.0,  # Mip 0 (2x2)
            0.0, 1.0, 0.0, 1.0,
            0.0, 0.0, 1.0, 1.0,
            1.0, 1.0, 1.0, 1.0,
            1.0, 1.0, 0.0, 1.0 ] # Mip 1 (1x1)
  - Name: Out1 # Buffer where our output will go
    Format: Float32
    Stride: 4
    FillSize: 8
    FillValue: 0.0 # The FillValue is optional and defaults to zero
  - Name: Expected1 # Buffer which stores the expected result of our test
    Format: Float32
    Stride: 4
    Data: [ 0.0, 1.0 ]
  - Name: Out2 # Buffer where our output will go
    Format: Float16
    Stride: 2
    FillSize: 4 # FillSize needs to be 4 bytes minimum
  - Name: Expected2 # Buffer which stores the expected result of our test
    Format: Float16
    Stride: 2
    Data: [ 0x1, 0x2 ]
Results: # Using Result can verify test values without filecheck
  - Result: Test1
    Rule: BufferFloatULP # Rule which can be used to compare Float Buffers; They are compared within a ULP range
    ULPT: 1 # ULP to use
    DenormMode: Any # if DenormMode Field is not Specified, 'Any' is the default; FTZ and Preserve are the other options.
    Actual: Out1 # First buffer to compare
    Expected: Expected1 # Second buffer to compare against first
  - Result: Test2
    Rule: BufferExact # Compares Two Buffers for == equality between each value elementwise
    Actual: Out1
    Expected: Expected1
  - Result: Test3
    Rule: BufferFloatEpsilon # Rule which can be used to compare Float Buffers; They are compared within an epsilon difference
    Epsilon: 0.0008
    Actual: Out1
    Expected: Expected1
DescriptorSets:
  - Resources:
    - Name: Constants
      Kind: ConstantBuffer
      DirectXBinding:
        Register: 0 # implies b0 due to Access being Constant
        Space: 0
      VulkanBinding:
        Binding: 0 # [[vk::binding(0, 0)]]
    - Name: In1
      Kind: Buffer
      DirectXBinding:
        Register: 0 # implies t0 due to Access being RO
        Space: 0
      VulkanBinding:
        Binding: 10
  - Resources:
    - Name: In2
      Kind: Buffer
      DirectXBinding:
        Register: 1 # implies t1 due to Access being RO
        Space: 0
      VulkanBinding:
        Binding: 0 # [[vk::binding(0, 1)]]
...
```
