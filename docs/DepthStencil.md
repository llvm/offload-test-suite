# Depth and Stencil Tests

Traditional and mesh raster pipelines automatically create a depth/stencil
attachment matching the render target's dimensions and sample count. Depth
testing and writing are enabled, and depth is cleared to `1.0`. DirectX and
Metal use `Less`; Vulkan uses `LessEqual`.

Stencil testing is disabled by default. Enable it with an optional top-level
configuration:

```yaml
StencilState:
  Enabled: true
  ClearValue: 1
  ComparisonOp: Equal
```

`Enabled` defaults to `false`, `ClearValue` to `0`, and `ComparisonOp` to
`Equal`. Clear values must be in `0..255`. Comparison functions are `Never`,
`Less`, `Equal`, `LessEqual`, `Greater`, `NotEqual`, `GreaterEqual`, and
`Always`, using the same enum as comparison samplers. `StencilState` is valid
only on raster pipelines, even when disabled.

When enabled, both stencil faces use the selected comparison, read/write
masks of `0xff`, and `Keep` operations. The stencil attachment is cleared
to `ClearValue`, and the color attachment is cleared to transparent black
so rejected fragments have a defined background. Stencil writes and
depth/stencil readback are not exposed by this configuration.

The fixed-function stencil reference is `0`. A pixel shader's
`SV_StencilRef` output overrides it for stencil testing. DirectX tests
using that output should require the lit feature `ShaderStencilRef`,
which reflects `PSSpecifiedStencilRefSupported`. Vulkan tests should
require `VK_EXT_shader_stencil_export`; the backend enables this extension
when the device advertises it. Metal uses matching depth/stencil descriptors.

`test/Feature/Semantics/StencilRef.test` observes reference comparisons
through color output. `DepthOutputs.test` uses overlapping triangles to
observe `SV_Depth`, `SV_DepthLessEqual`, and `SV_DepthGreaterEqual` without
depth readback. Conservative-depth tests must keep their shader output
less than or equal to, or greater than or equal to, rasterized depth
respectively.
