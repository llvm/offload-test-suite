# Using WARP on Windows

Windows has a software rasterizer implementation called the Windows Advanced
Rasterization Platform (WARP), which is a conforming implementation of DirectX
and Vulkan 1.2.

The offload-test-suite supports running against WARP on Windows by default and
will generate test targets for `check-hlsl-warp-d3d12` and
`check-hlsl-clang-warp-d3d12`.

There are some useful CMake options to tweak the configuration to better utilize
WARP:

* **OFFLOADTEST_WARP_ONLY** - Skips generating d3d test configurations for
  non-WARP configurations. This is useful if you're running Windows in a VM and
  do not have a physical GPU.

* **WARP_VERSION** - Defaults to `System` which uses the system version of WARP.
  This option may also be set to an explicit WARP version, and the configuration
  step will pull WARP from NuGet. See the [NuGet package
  listing](https://www.nuget.org/packages/Microsoft.Direct3D.WARP) to identify
  valid versions.
