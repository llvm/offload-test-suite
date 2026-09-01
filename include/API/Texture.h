//===- Texture.h - Offload API Texture ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_TEXTURE_H
#define OFFLOADTEST_API_TEXTURE_H

#include "API/API.h"
#include "API/Enums.h"
#include "API/Resources.h"

#include "llvm/ADT/BitmaskEnum.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace offloadtest {
LLVM_ENABLE_BITMASK_ENUMS_IN_NAMESPACE();

class Device;

enum TextureUsage : uint32_t {
  Sampled = 1 << 0,
  Storage = 1 << 1,
  RenderTarget = 1 << 2,
  DepthStencil = 1 << 3,
  LLVM_MARK_AS_BITMASK_ENUM(/* LargestValue = */ DepthStencil)
};

inline std::string getTextureUsageName(TextureUsage Usage) {
  std::string Result;
  if ((Usage & Sampled) != 0)
    Result += "Sampled|";
  if ((Usage & Storage) != 0)
    Result += "Storage|";
  if ((Usage & RenderTarget) != 0)
    Result += "RenderTarget|";
  if ((Usage & DepthStencil) != 0)
    Result += "DepthStencil|";
  if (!Result.empty())
    Result.pop_back(); // Remove trailing '|'
  return Result;
}

struct ClearColor {
  float R = 0.0f, G = 0.0f, B = 0.0f, A = 0.0f;
};

struct ClearDepthStencil {
  float Depth = 1.0f;
  uint8_t Stencil = 0;
};

using ClearValue = std::variant<ClearColor, ClearDepthStencil>;

// TODO: only 2D textures (and 2D texture arrays) are supported. 1D, 3D, and
// cube textures need their ResourceDimension cases filled in, plus validation
// between usage and shape (e.g. 3D textures cannot be used as DepthStencil).
struct TextureCreateDesc {
  MemoryLocation Location = MemoryLocation::GpuOnly;
  MemoryBacking Backing = MemoryBacking::Automatic;
  TextureUsage Usage = {};
  Format Fmt = Format::RGBA32Float;
  ResourceDimension Dim = ResourceDimension::Dim2D;
  uint32_t Width = 1;
  uint32_t Height = 1;
  uint32_t MipLevels = 1;
  uint32_t ArraySlices = 1;
  bool IsArray = false;
  // Clear value for render target or depth/stencil textures.
  // How and when this is applied depends on the backend:
  // - DX uses it as an optimized clear hint at resource creation time
  // - VK and MTL apply it at render pass begin
  std::optional<ClearValue> OptimizedClearValue;

  // The total number of subresources: one per (mip, slice) pair.
  uint32_t getSubresourceCount() const { return MipLevels * ArraySlices; }

  uint32_t getMipWidth(uint32_t Mip) const {
    assert(Mip < MipLevels && "Mip level index out of bounds.");
    return std::max(1u, Width >> Mip);
  }
  uint32_t getMipHeight(uint32_t Mip) const {
    assert(Mip < MipLevels && "Mip level index out of bounds.");
    return std::max(1u, Height >> Mip);
  }
};

inline llvm::Error validateTextureCreateDesc(const TextureCreateDesc &Desc) {
  if (!isTextureCompatible(Desc.Fmt))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Format '%s' is not compatible with texture creation.",
        getFormatName(Desc.Fmt).data());

  // Only 2D textures are implemented for now.
  if (Desc.Dim != ResourceDimension::Dim2D)
    return llvm::createStringError(std::errc::not_supported,
                                   "Only 2D textures are supported.");

  if (Desc.Width == 0 || Desc.Height == 0)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Texture dimensions must be non-zero, got %ux%u.", Desc.Width,
        Desc.Height);

  if (Desc.MipLevels == 0)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Texture must have at least one mip level.");

  // A full mip chain halves the largest extent until it reaches 1x1, so the
  // texture supports floor(log2(max(Width, Height))) + 1 levels.
  // TODO: Account for Depth when 3D textures are supported.
  uint32_t MaxMipLevels = 0;
  for (uint32_t Extent = std::max(Desc.Width, Desc.Height); Extent != 0;
       Extent >>= 1)
    ++MaxMipLevels;
  if (Desc.MipLevels > MaxMipLevels)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Texture dimensions %ux%u support at most %u mip levels, got %u.",
        Desc.Width, Desc.Height, MaxMipLevels, Desc.MipLevels);

  if (Desc.ArraySlices == 0)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "A texture requires at least one slice.");
  if (Desc.ArraySlices > 1 && !Desc.IsArray)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "A texture with %u slices must be created as a texture array.",
        Desc.ArraySlices);

  const bool IsDepth = isDepthFormat(Desc.Fmt);
  const bool IsRT = (Desc.Usage & TextureUsage::RenderTarget) != 0;
  const bool IsDS = (Desc.Usage & TextureUsage::DepthStencil) != 0;

  // DepthStencil + RenderTarget is not supported.
  if (IsDS && IsRT)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "DepthStencil and RenderTarget are mutually exclusive.");
  // DepthStencil + Storage is a valid but discouraged configuration (poor
  // performance on most hardware). Not supported for now.
  if (IsDS && (Desc.Usage & TextureUsage::Storage) != 0)
    return llvm::createStringError(
        std::errc::not_supported,
        "DepthStencil combined with Storage is not yet supported.");

  if (!IsDepth && IsDS)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "DepthStencil usage requires a depth format, got '%s'.",
        getFormatName(Desc.Fmt).data());

  // Render targets and depth/stencil textures only support a single mip level.
  if ((IsRT || IsDS) && Desc.MipLevels != 1)
    return llvm::createStringError(
        std::errc::not_supported,
        "Multiple mip levels are not supported for render target or "
        "depth/stencil textures.");

  // Layered rendering is not plumbed through the render pass API yet.
  if ((IsRT || IsDS) && Desc.ArraySlices != 1)
    return llvm::createStringError(
        std::errc::not_supported,
        "Array slices are not supported for render target or depth/stencil "
        "textures.");

  // A clear value requires RenderTarget or DepthStencil usage, and the
  // variant must match.
  if (Desc.OptimizedClearValue) {
    if (!IsRT && !IsDS)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "OptimizedClearValue requires RenderTarget or DepthStencil usage.");
    if (IsRT && !std::holds_alternative<ClearColor>(*Desc.OptimizedClearValue))
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RenderTarget usage requires a ClearColor clear value.");
    if (IsDS &&
        !std::holds_alternative<ClearDepthStencil>(*Desc.OptimizedClearValue))
      return llvm::createStringError(
          std::errc::invalid_argument,
          "DepthStencil usage requires a ClearDepthStencil clear value.");
  }

  return llvm::Error::success();
}

// The texel dimensions of a single sparse tile. A tile is a fixed byte size
// (~64 KiB) laid out as this WxHxD box of texels; Depth is 1 for 2D textures.
// Tile counts are computed per dimension as ceil(extent / tileExtent).
struct TileShape {
  uint32_t Width = 1;
  uint32_t Height = 1;
  uint32_t Depth = 1;
};

struct SubresourceFootprint {
  uint64_t Offset = 0; // Byte offset of this subresource in the buffer.
  uint32_t RowPitchInBytes = 0; // Destination row stride (may include padding).
  uint32_t RowSizeInBytes = 0;  // Tightly-packed bytes per row to copy.
  uint32_t NumRows = 0;         // Number of rows in this subresource.
};

struct TextureUploadLayout {
  // One entry per subresource, ordered slice-major (all mips of slice 0, then
  // all mips of slice 1, ...), matching D3D12's `Mip + Slice * MipLevels`
  // subresource indexing.
  llvm::SmallVector<SubresourceFootprint> Subresources;
  uint64_t TotalSizeInBytes = 0;

  // The size of the tightly-packed host-side representation of this texture:
  // every subresource's rows back to back with no row or subresource padding.
  uint64_t getPackedSizeInBytes() const {
    uint64_t Size = 0;
    for (const SubresourceFootprint &Sub : Subresources)
      Size += uint64_t(Sub.RowSizeInBytes) * Sub.NumRows;
    return Size;
  }
};

// Copy a texture's contents between a tightly-packed host buffer and a
// (possibly row/subresource padded) GPU-visible mapping described by `Layout`.
// These methods know how to bridge the two layouts, so backends with alignment
// requirements (e.g. D3D12's 256-byte D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) work
// for arbitrary widths.
void copyPackedToTextureLayout(void *Dst, const void *PackedSrc,
                               const TextureUploadLayout &Layout);
void copyTextureLayoutToPacked(void *PackedDst, const void *Src,
                               const TextureUploadLayout &Layout);

// Compute a tightly-packed upload layout (no row or subresource padding) for
// the given texture description. Suitable for backends whose buffer-to-texture
// copy consumes a tightly-packed staging buffer (e.g. Vulkan, Metal).
TextureUploadLayout
computeTightTextureUploadLayout(const TextureCreateDesc &Desc);

class Texture {
  GPUAPI API;

public:
  virtual ~Texture();
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  // Calculate the size in bytes of the texture data given a linear layout
  // Useful for calculating the size for an upload or readback buffer.
  size_t calculateLinearSizeInBytes(const Device &Dev) const;

  // The texel dimensions of a single sparse tile for this texture. The shape
  // varies by format, dimensionality, and sample count. Only meaningful for a
  // texture created with MemoryBacking::Sparse.
  virtual TileShape querySparseTileShape(const Device &Dev) const = 0;

  GPUAPI getAPI() const { return API; }
  virtual const TextureCreateDesc &getDesc() const = 0;

protected:
  explicit Texture(GPUAPI API) : API(API) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_TEXTURE_H
