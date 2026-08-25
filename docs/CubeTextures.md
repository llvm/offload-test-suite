# Cube Textures

The test suite supports the cube texture types:

| YAML `Kind`         | HLSL type           | Descriptor |
| ------------------- | ------------------- | ---------- |
| `TextureCube`       | `TextureCube`       | SRV        |
| `TextureCubeArray`  | `TextureCubeArray`  | SRV        |

## Data Layout

A cube is six array layers, one per face, always in this order:

| Layer | Face |
| ----- | ---- |
| 0     | `+X` |
| 1     | `-X` |
| 2     | `+Y` |
| 3     | `-Y` |
| 4     | `+Z` |
| 5     | `-Z` |

Because faces are layers, cube textures reuse the `ArraySlices` field and the
slice-major data layout described in [ArrayTextures.md](ArrayTextures.md)
unchanged. `ArraySlices` is a **layer** count, not a count of cubes:

* a `TextureCube` requires `ArraySlices: 6`
* a `TextureCubeArray` of *N* cubes requires `ArraySlices: 6 * N`

Both are validated. `ArraySlices` that is not a multiple of 6 is rejected, as is
a `TextureCube` with more than 6 layers — that is a cube array and must use the
array kind. See `Tools/Offloader/ArraySliceValidation.test`.

```yaml
Buffers:
  - Name: Tex
    Format: Float32
    Channels: 4
    OutputProps: { Width: 2, Height: 2, Depth: 1, ArraySlices: 6 }
    Data: [ # --- Face 0 (+X) ---
            ...
            # --- Face 1 (-X) ---
            ... ]

DescriptorSets:
  - Resources:
    - Name: Tex
      Kind: TextureCube
      DirectXBinding: { Register: 0, Space: 0 }
      VulkanBinding: { Binding: 0 }
```

With mip levels the ordering is the same as for array textures: the complete
mip chain of face 0, then the complete mip chain of face 1, and so on, matching
D3D12's `Mip + Slice * MipLevels` subresource indexing.

For a cube array the six faces of cube 0 come first (layers 0-5), then the six
faces of cube 1 (layers 6-11), and so on.

Note that `ArraySlices` and the `Elements` returned by a shader's
`GetDimensions` are in different units: the YAML field counts layers, while
`TextureCubeArray::GetDimensions` reports **cubes**, so a resource declared with
`ArraySlices: 12` reports 2.

## Backend Support

Cube textures are supported on DirectX and Vulkan.

* **Vulkan** creates the image with `VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT` and
  views it as `VK_IMAGE_VIEW_TYPE_CUBE` / `VK_IMAGE_VIEW_TYPE_CUBE_ARRAY`. A
  cube array view additionally requires the `imageCubeArray` device feature.
* **DirectX** creates an ordinary 2D resource whose `DepthOrArraySize` is the
  layer count, and gives it a `D3D12_SRV_DIMENSION_TEXTURECUBE` /
  `TEXTURECUBEARRAY` view. Note that `TextureCubeArray.NumCubes` is a count of
  *cubes*, so it is the layer count divided by six.
* **Metal** does not support cube textures; tests are marked `XFAIL: Metal`.
