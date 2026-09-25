# Multisampled Render Targets (MSAA)

Raster tests can request multisample anti-aliasing (MSAA) with the optional
`Bindings.SampleCount` field. It defaults to `1`.

```yaml
Buffers:
  - Name: Output
    Format: Float32
    Channels: 4
    FillSize: 1048576
    OutputProps:
      Width: 256
      Height: 256
      Depth: 1

Bindings:
  RenderTarget: Output
  SampleCount: 4
```

Values other than `1` are valid only for traditional and mesh raster
pipelines. The runtime uses the same count for the color target, depth-stencil
target, render pass, and pipeline state. Binding an attachment with a different
count is an error.

## Supported Counts

A count of `0` is always invalid. Further restrictions are backend-specific:

* **DirectX** queries multisample quality levels and format support. The
  requested count must be supported for every color and depth-stencil format
  used by the pipeline. Color formats must also support resolve operations.
* **Vulkan** accepts the values represented by `VkSampleCountFlagBits`: `1`,
  `2`, `4`, `8`, `16`, `32`, and `64`. The requested value must also be present
  in both the format's image properties and the applicable framebuffer sample
  count limits.
* **Metal** queries `supportsTextureSampleCount` for the requested count.

Support depends on the device and format; a value accepted by the YAML parser
may still be rejected by the backend.

## Resolve and Readback

A multisampled image stores several samples per texel and cannot be copied
straight into the test's linear readback buffer. For `SampleCount > 1`, the
runtime creates a matching single-sample texture and resolves the rendered
color target into it before readback:

* DirectX uses `ResolveSubresource`.
* Vulkan uses `vkCmdResolveImage`.
* Metal resolves through `MTLStoreActionStoreAndMultisampleResolve` when the
  render pass ends.

The output buffer therefore retains the dimensions and format declared in
YAML; `SampleCount` does not multiply its size.

## Current Scope

`Bindings.SampleCount` creates multisampled raster attachments. Multisampled
textures have one mip level as required by DirectX, Vulkan, and Metal. The suite
also limits render targets to one array slice because layered rendering is not
implemented.

`SampleCount` does not create shader-visible `Texture2DMS` or `RWTexture2DMS`
resources; multisampled textures with `Sampled` or `Storage` usage are not
implemented yet. Sparse residency is also not implemented for multisampled
attachments.
