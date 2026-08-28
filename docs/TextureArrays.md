# Texture Arrays

The test suite supports the 2D texture array types, so tests can verify
array-aware indexing, sampling and readback:

| YAML `Kind`        | HLSL type            | Descriptor |
| ------------------ | -------------------- | ---------- |
| `Texture2DArray`   | `Texture2DArray`     | SRV        |
| `RWTexture2DArray` | `RWTexture2DArray`   | UAV        |

1D and cube texture arrays are not supported yet.

## Defining Array Slices

The number of slices (layers) is given by the `ArraySlices` field in
`OutputProps`. It defaults to 1 and may only be greater than 1 for the array
kinds above.

```yaml
Buffers:
  - Name: Tex
    Format: Float32
    Channels: 4
    OutputProps:
      Width: 2
      Height: 2
      Depth: 1
      ArraySlices: 3
```

`ArraySlices` is unrelated to `ArraySize`, which describes an array *of
descriptors* (see `SRVToUAV.array.test`). A resource may use
both: `ArraySize` selects how many textures are bound, `ArraySlices` how many
layers each of those textures has.

## Data Layout

Texture array data is laid out **slice-major**: the complete mip chain of slice
0, then the complete mip chain of slice 1, and so on. This matches D3D12's
subresource indexing, where subresource `N` is `Mip + Slice * MipLevels`, and
is the order both the DirectX and Vulkan backends use for upload and readback.

For a 2x2 texture with 2 mip levels and 2 array slices the `Data` array is:

1. **Slice 0, Mip 0 (2x2 = 4 texels)**
2. **Slice 0, Mip 1 (1x1 = 1 texel)**
3. **Slice 1, Mip 0 (2x2 = 4 texels)**
4. **Slice 1, Mip 1 (1x1 = 1 texel)**

Mip dimensions are derived exactly as for non-array textures; see
[MipMappedTextures.md](MipMappedTextures.md).

```yaml
  - Name: Tex
    Format: Float32
    Channels: 4
    OutputProps: { Width: 2, Height: 2, Depth: 1, MipLevels: 2, ArraySlices: 2 }
    Data: [ # --- Slice 0, Mip 0 ---
            1.0, 0.0, 0.0, 1.0,  0.0, 1.0, 0.0, 1.0,
            0.0, 0.0, 1.0, 1.0,  1.0, 1.0, 1.0, 1.0,
            # --- Slice 0, Mip 1 ---
            1.0, 1.0, 0.0, 1.0,
            # --- Slice 1, Mip 0 ---
            0.0, 1.0, 1.0, 1.0,  1.0, 0.0, 1.0, 1.0,
            0.5, 0.0, 0.0, 1.0,  0.5, 0.5, 0.5, 1.0,
            # --- Slice 1, Mip 1 ---
            0.0, 0.0, 0.5, 1.0 ]
```

## Implementation Notes

Per-slice upload and readback go through a single shared description of the
texture layout (`Device::getTextureUploadLayout`, consumed by
`copyPackedToTextureLayout` / `copyTextureLayoutToPacked`). That layout reports
each subresource's byte offset, row pitch, tight row size and row count, so
back-ends with row alignment requirements — notably D3D12's 256-byte
`D3D12_TEXTURE_DATA_PITCH_ALIGNMENT` — work for arbitrary texture widths
without any per-test padding. `Array.UnalignedRowPitch.test` covers a
width whose natural row size is not 256-byte aligned.

Writes through an `RWTexture*Array` only reach mip 0, and a UAV spans array
slices (`FirstArraySlice`/`ArraySize`) but not mip levels, because the slice is
a shader coordinate and the mip is not. See
[MipMappedTextures.md](MipMappedTextures.md) for why the level is fixed by the
binding. `Array.SRVToUAV.test` relies on this: it seeds mip 1 with a
sentinel and checks that the round trip leaves it untouched.
