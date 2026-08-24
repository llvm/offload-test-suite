# Cube Textures

The test suite supports the cube texture types:

| YAML `Kind`         | HLSL type           | Descriptor |
| ------------------- | ------------------- | ---------- |
| `TextureCube`       | `TextureCube`       | SRV        |
| `TextureCubeArray`  | `TextureCubeArray`  | SRV        |

## Faces Are Layers

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

## Access Is Sampling Only

Unlike 2D textures, cube textures have neither `Load` nor an `operator[]`:

```
error: no member named 'Load' in 'TextureCube<vector<float, 4> >'
error: type 'TextureCube<float4>' does not provide a subscript operator
```

A face and a texel within it are addressed by a **direction vector** rather than
by integer coordinates, so every cube test needs a sampler. `GetDimensions` is
available and reports the face dimensions.

The sample coordinate carries the cube index for array kinds:

* `TextureCube` takes a `float3` direction
* `TextureCubeArray` takes a `float4`: `xyz` is the direction, `w` selects the
  cube

```hlsl
Out[0] = Cube.SampleLevel(Samp, float3(1, 0, 0), 0);       // +X face
Out[1] = CubeArr.SampleLevel(Samp, float4(1, 0, 0, 1), 0); // +X face of cube 1
```

Sampling straight down an axis lands in the centre of the corresponding face.
`Sampler.filter.test` uses this to check the face ordering above, with each
face a uniform colour so that face selection is tested independently of any
in-face addressing convention. `Array.Sampler.filter.test` does the same for a
cube array, sampling two cubes to confirm that the `w` component selects a cube
and not a layer.

`Gather` and its per-channel forms (`GatherRed`/`Green`/`Blue`/`Alpha`) also
take a direction and return the four texels of the selected face in the usual
`[ (0,1), (1,1), (1,0), (0,0) ]` order. There is no offset overload, because an
integer texel offset has no meaning once it would cross a face edge:

```
error: no matching member function for call to 'Gather'
```

`Gather.test` covers all four channel forms on a face whose every channel
differs, plus a gather on a second face to show that the four texels never come
from a neighbouring one.

Cube coverage lives alongside the 2D and array coverage in the existing test
for each operation rather than in cube-specific files, and shares that test's
shader and pipeline rather than adding RUN lines. Those tests are marked
`UNSUPPORTED: Clang` until clang implements the cube resource types.

| Operation | `TextureCube` | `TextureCubeArray` |
| --------- | ------------- | ------------------ |
| `Sample` (and LOD clamp) | `Sample.test` | - |
| `SampleBias` (and min-LOD clamp) | `SampleBias.test` | - |
| `SampleLevel` | `Sampler.filter.test` | `Array.Sampler.filter.test` |
| `SampleGrad` | `SampleGrad.test` | - |
| `SampleCmp`, `SampleCmpLevelZero` | `SampleCmp.test` | - |
| `Gather`, `GatherRed`/`Green`/`Blue`/`Alpha` | `Gather.test` | - |
| `GatherCmp` | `GatherCmp.test` | - |
| `GetDimensions` | `GetDimensions.test` | `Array.GetDimensions.test` |
| `CalculateLevelOfDetail` | `CalculateLevelOfDetail.test` | - |

`TextureCube` has no `Load` and no subscript operator, so that is the whole
method surface. Two details are worth noting when reading those tests:

* `SampleGrad` on a cube takes **float3** gradients, not the float2 a
  `Texture2DArray` takes: the derivative of a direction is itself a direction,
  so there is no separate slice axis to omit.
* `GetDimensions` on a `TextureCubeArray` reports the number of **cubes**, not
  the number of layers -- 12 layers is reported as 2. This is the same
  units distinction as `D3D12_SRV_DIMENSION_TEXTURECUBEARRAY`'s `NumCubes`.
* A direction is projected onto the selected face before `CalculateLevelOfDetail`
  takes its derivative, so a cube LOD is not a simple function of the raw float3
  delta. That test asserts only the two endpoints that hold regardless: a
  negligible derivative gives LOD 0, and an enormous one saturates at
  `MipLevels - 1`.

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

Clang does not yet implement the `TextureCube` or `TextureCubeArray` resource
types, so the cube tests are marked `UNSUPPORTED: Clang` and currently exercise
DXC only.
