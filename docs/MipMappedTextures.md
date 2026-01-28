# Mipmapped Textures

The test suite supports textures with multiple mipmap levels. This allows verifying that the compiler correctly generates code for `CalculateLevelOfDetail`, `SampleGrad` (explicit gradients), and direct access via `mips[][]`.

## Defining Mip Levels

To define a texture with multiple mips, specify the `MipLevels` field in `OutputProps`.

**Example:**
```yaml
Buffers:
  - Name: Tex
    Format: Float32
    Channels: 4
    OutputProps:
      Width: 4
      Height: 4
      Depth: 1
      MipLevels: 3  # Defines Mip 0 (4x4), Mip 1 (2x2), and Mip 2 (1x1)
```

## Dimension Calculation

The dimensions of each mip level are calculated automatically based on the base `Width`, `Height`, and `Depth` provided in `OutputProps`. Following standard graphics API conventions (Vulkan, DirectX, Metal), each subsequent mip level is half the size of the previous level, rounded down, with a minimum of 1.

For Mip Level $N$:
*   $\text{Width}_N = \max(1, \lfloor \text{Width}_0 / 2^N \rfloor)$
*   $\text{Height}_N = \max(1, \lfloor \text{Height}_0 / 2^N \rfloor)$
*   $\text{Depth}_N = \max(1, \lfloor \text{Depth}_0 / 2^N \rfloor)$

## Data Layout

The `Data` array in the YAML must contain the texel data for **all mip levels sequentially**, packed tightly without padding.

For a 4x4 texture with 3 mip levels, the `Data` array structure is:

1.  **Mip 0 (4x4 = 16 texels):** Indices 0 - 15.
2.  **Mip 1 (2x2 = 4 texels):** Indices 16 - 19.
3.  **Mip 2 (1x1 = 1 texel):** Index 20.

**Total Size:** 21 texels.

### Example YAML

```yaml
  - Name: Tex
    Format: Float32
    Channels: 4 # RGBA
    OutputProps: { Width: 4, Height: 4, Depth: 1, MipLevels: 3 }
    Data: [
            # --- Mip 0 (4x4) ---
            1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1,  # Row 0 (Red)
            1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1,  # Row 1
            1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1,  # Row 2
            1,0,0,1, 1,0,0,1, 1,0,0,1, 1,0,0,1,  # Row 3

            # --- Mip 1 (2x2) ---
            0,1,0,1, 0,1,0,1,                    # Row 0 (Green)
            0,1,0,1, 0,1,0,1,                    # Row 1

            # --- Mip 2 (1x1) ---
            0,0,1,1                              # Single Pixel (Blue)
          ]
```

## Usage in Tests

This structure allows you to verify explicit mip selection in shaders:

```hlsl
// Should return Green (Mip 1)
float4 val = Tex.mips[1][int2(0,0)];

// Should return Blue (Mip 2)
float4 val2 = Tex.Load(int3(0,0,2));
```
