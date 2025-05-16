# OffloadTest
Experimental Runtime test suite for HLSL

## Current Status

| Testing Machine | DXC | Clang |
|-----------------|-----|-------|
| Windows DirectX12 Intel GPU | ![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-d3d12.yaml/badge.svg) | ![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-d3d12.yaml/badge.svg) |
| Windows DirectX12 Warp | ![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-warp-d3d12.yaml/badge.svg) | ![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-warp-d3d12.yaml/badge.svg) |
| Windows Vulkan Intel GPU | ![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-dxc-vk.yaml/badge.svg) | ![Clang](https://github.com/llvm/offload-test-suite/actions/workflows/windows-intel-clang-vk.yaml/badge.svg) |
| macOS Apple M1 | ![DXC](https://github.com/llvm/offload-test-suite/actions/workflows/macos-dxc-mtl.yaml/badge.svg) | ![Clang & DXC](https://github.com/llvm/offload-test-suite/actions/workflows/macos-clang-mtl.yaml/badge.svg) |


# Prerequisites

This project requires being able to locally build LLVM and leverages LLVM's build infrastructure. It also requires installing the `pyyaml` Python package. You can install `pyyaml` by running:

```shell
pip3 install pyyaml
```

On Windows, the [Graphics Tools](https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-environment-set-up#debug-layer) optional feature is additionally required to run the test suite.

# Adding to LLVM Build

Add the following to the CMake options:

```shell
-DLLVM_EXTERNAL_OFFLOADTEST_SOURCE_DIR=${workspaceRoot}\..\OffloadTest -DLLVM_EXTERNAL_PROJECTS="OffloadTest"
```

If you do not have a build of dxc on your path you'll need to specify the shader
compiler to use by passing:

```shell
-DDXC_DIR=<path to folder containing dxc & dxv>
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
  - Name: Out1 # Buffer where our output will go
    Format: Float32
    Stride: 4
    ZeroInitSize: 8
  - Name: Expected1 # Buffer which stores the expected result of our test
    Format: Float32
    Stride: 4
    Data: [ 0.0, 1.0 ]
  - Name: Out2 # Buffer where our output will go
    Format: Float16
    Stride: 2
    ZeroInitSize: 4 # ZeroInitSize needs to be 4 bytes minimum
  - Name: Expected2 # Buffer which stores the expected result of our test
    Format: Float16
    Stride: 2
    Data: [ 0x1, 0x2 ]
Results: # Using Result can verify test values without filecheck
  - Result: Test1
    Rule: BufferFuzzy # Rule which can be used to compare Float Buffers; They are compared within a ULP range
    ULPT: 1 # ULP to use
    DenormMode: Any # if DenormMode Field is not Specified, 'Any' is the default; FTZ and Preserve are the other options.
    Actual: Out1 # First buffer to compare
    Expected: Expected1 # Second buffer to compare against first
  - Result: Test2
    Rule: BufferExact # Compares Two Buffers for == equality between each value elementwise
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
