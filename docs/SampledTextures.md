# Sampled Textures

The test suite supports sampled textures through `ResourceKind:
SampledTexture..`. This binds texture data and sampler state as one sampled
resource and is used to validate shader sampling behavior.

> Note: `SampledTexture...` resource kinds are currently supported only on the
> Vulkan backend.

For Vulkan, sampled texture resources map to combined image sampler descriptors
(`COMBINED_IMAGE_SAMPLER`).

## Defining a Sampled Texture Resource

A sampled texture resource is defined using:

1. A `Buffer` entry containing texture data.
2. A `Sampler` entry containing sampler state.
3. A descriptor `Resource` entry with `Kind: SampledTexture...`.

The `Buffer`, `Sampler`, and descriptor `Resource` entries must use the same
`Name` so they are resolved as one sampled texture binding.

**YAML Example (`SampledTexture2D`):**
```yaml
Buffers:
  - Name: SampledTex
    Format: Float32
    Channels: 4
    OutputProps:
      Width: 2
      Height: 2
      Depth: 1
      MipLevels: 2
    Data: [ 1.0, 0.0, 0.0, 1.0,  # Mip 0 texel 0 (Red)
            0.0, 1.0, 0.0, 1.0,  # Mip 0 texel 1 (Green)
            0.0, 0.0, 1.0, 1.0,  # Mip 0 texel 2 (Blue)
            1.0, 1.0, 1.0, 1.0,  # Mip 0 texel 3 (White)
            0.5, 0.5, 0.5, 1.0 ] # Mip 1 texel 0 (Grey)

Samplers:
  - Name: SampledTex
    Address: Clamp
    MinFilter: Linear
    MagFilter: Nearest

DescriptorSets:
  - Resources:
    - Name: SampledTex
      Kind: SampledTexture2D
      DirectXBinding: { Register: 0, Space: 0 }
      VulkanBinding: { Binding: 0 }
```

## Array and MS Types

Sampled texture resources are modeled as explicit kinds, including:

* `SampledTexture1D`
* `SampledTexture1DArray`
* `SampledTexture2D`
* `SampledTexture2DArray`
* `SampledTexture2DMS`
* `SampledTexture2DMSArray`
* `SampledTexture3D`
* `SampledTextureCUBE`
* `SampledTextureCUBEArray`

All sampled texture kinds follow the same binding model, where the Sampler and Textures are bound together using the same `Name`.

## Sampler Comparison

Comparison behavior is controlled by sampler state, not by resource kind:

* `Kind: Sampler` (default) requires `ComparisonOp: Never`.
* `Kind: SamplerComparison` requires `ComparisonOp` other than `Never`.

## Data and Mips

Sampled texture resources use the same texture storage rules as other texture
resources:

* `OutputProps` controls dimensions and mip count.
* `Data` stores all mip levels sequentially, tightly packed.
