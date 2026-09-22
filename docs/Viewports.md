# Viewports

A viewport maps normalized device coordinates produced by the shader pipeline
to a rectangular region of the render target. A scissor rectangle then limits
which pixels in that region may be written.

Viewport and scissor state are independent. They must have matching array
counts, but their positions and dimensions may differ. For example, a smaller
scissor can crop a viewport without changing its coordinate transform.

## Default viewport

When `Viewports` and `Scissors` are omitted, a raster pipeline uses one viewport
and one scissor rectangle covering the full render target:

```yaml
Bindings:
  RenderTarget: Output
```

## Explicit viewports and scissors

Define viewport and scissor arrays under `Bindings`:

```yaml
Bindings:
  RenderTarget: Output
  Viewports:
    - X: 0
      Y: 0
      Width: 400
      Height: 600
    - X: 400
      Y: 0
      Width: 400
      Height: 600
  Scissors:
    - X: 0
      Y: 0
      Width: 400
      Height: 600
    - X: 400
      Y: 0
      Width: 400
      Height: 600
```

This divides an 800-by-600 render target into two side-by-side viewports.
Viewport 0 uses scissor 0, viewport 1 uses scissor 1, and so on.

`Viewports` and `Scissors` must be specified together and contain the same
number of entries. Up to 16 entries are supported.

Viewport fields:

| Field | Required | Default | Constraint |
|---|---|---|---|
| `X` | No | `0` | Finite |
| `Y` | No | `0` | Finite |
| `Width` | Yes | — | Finite and greater than zero |
| `Height` | Yes | — | Finite and greater than zero |
| `MinDepth` | No | `0` | Finite and between 0 and 1 |
| `MaxDepth` | No | `1` | Finite and between `MinDepth` and 1 |

Scissor fields:

| Field | Required | Default | Constraint |
|---|---|---|---|
| `X` | No | `0` | Non-negative |
| `Y` | No | `0` | Non-negative |
| `Width` | Yes | — | Greater than zero |
| `Height` | Yes | — | Greater than zero |

## Selecting a viewport from HLSL

A shader selects a viewport by writing `SV_ViewportArrayIndex`:

```hlsl
cbuffer Cameras {
  float4x4 ViewProjection[2];
};

struct VSOutput {
  float4 Position : SV_Position;
  uint ViewportIndex : SV_ViewportArrayIndex;
};

VSOutput main(float3 Position : POSITION, uint InstanceID : SV_InstanceID) {
  VSOutput Output;
  Output.Position = mul(ViewProjection[InstanceID], float4(Position, 1.0));
  Output.ViewportIndex = InstanceID;
  return Output;
}
```

The rasterizer applies the selected viewport transform and its corresponding
scissor rectangle to the primitive. A pixel shader can read the selected index
directly as an `SV_ViewportArrayIndex` input.

The index is primitive-level state, so all vertices of a primitive must write
the same value. A disagreement is invalid usage and does not produce a portable
viewport selection.

## Test requirements

Tests that specify more than one viewport should require the `multi-viewport`
lit feature. The feature gates the complete path: binding multiple viewport and
scissor slots and selecting among them from a shader.

```text
# REQUIRES: multi-viewport
```

Backend requirements differ:

- Direct3D 12 can bind multiple viewports without a capability check. The lit
  feature requires
  `VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation`
  so vertex, domain, and mesh shaders can select one without geometry-shader
  emulation.
- Vulkan requires `multiViewport` to bind more than one viewport and
  `shaderOutputViewportIndex` to select one from a vertex or tessellation
  evaluation shader.
- Metal requires Metal 2 or later. The test suite assumes this for all
  supported Metal targets and advertises the feature unconditionally.

See `test/Graphics/MultipleViewports.test` for a complete example.
