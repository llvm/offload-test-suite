#include "API/Texture.h"
#include "API/Device.h"

#include <cassert>
#include <cstring>

// Calculate the size in bytes of the texture data given a linear layout
// Useful for calculating the size for an upload or readback buffer.
size_t
offloadtest::Texture::calculateLinearSizeInBytes(const Device &Dev) const {
  // The backend's own layout already accounts for every (mip, slice)
  // subresource as well as any row and subresource alignment it requires.
  return Dev.getTextureUploadLayout(getDesc()).TotalSizeInBytes;
}

offloadtest::TextureUploadLayout
offloadtest::computeTightTextureUploadLayout(const TextureCreateDesc &Desc) {
  const uint32_t ElementSize = getFormatSizeInBytes(Desc.Fmt);
  TextureUploadLayout Layout;
  Layout.Subresources.reserve(Desc.getSubresourceCount());
  uint64_t Offset = 0;
  for (uint32_t Mip = 0; Mip < Desc.MipLevels; ++Mip) {
    SubresourceFootprint Sub;
    Sub.Offset = Offset;
    Sub.RowSizeInBytes = Desc.getMipWidth(Mip) * ElementSize;
    Sub.RowPitchInBytes = Sub.RowSizeInBytes;
    Sub.NumRows = Desc.getMipHeight(Mip);
    Layout.Subresources.push_back(Sub);
    Offset += uint64_t(Sub.RowSizeInBytes) * Sub.NumRows;
  }
  Layout.TotalSizeInBytes = Offset;
  return Layout;
}

void offloadtest::copyPackedToTextureLayout(void *Dst, const void *PackedSrc,
                                            const TextureUploadLayout &Layout) {
  auto *const DstBase = static_cast<uint8_t *>(Dst);
  const auto *SrcPtr = static_cast<const uint8_t *>(PackedSrc);
  for (const SubresourceFootprint &Sub : Layout.Subresources) {
    assert(Sub.RowSizeInBytes <= Sub.RowPitchInBytes &&
           "The packed side has no padding, so a row can never be larger "
           "than the padded side's row pitch.");
    uint8_t *DstPtr = DstBase + Sub.Offset;
    for (uint32_t Row = 0; Row < Sub.NumRows; ++Row) {
      memcpy(DstPtr, SrcPtr, Sub.RowSizeInBytes);
      DstPtr += Sub.RowPitchInBytes;
      SrcPtr += Sub.RowSizeInBytes;
    }
  }
}

void offloadtest::copyTextureLayoutToPacked(void *PackedDst, const void *Src,
                                            const TextureUploadLayout &Layout) {
  auto *DstPtr = static_cast<uint8_t *>(PackedDst);
  const auto *const SrcBase = static_cast<const uint8_t *>(Src);
  for (const SubresourceFootprint &Sub : Layout.Subresources) {
    assert(Sub.RowSizeInBytes <= Sub.RowPitchInBytes &&
           "The packed side has no padding, so a row can never be larger "
           "than the padded side's row pitch.");
    const uint8_t *SrcPtr = SrcBase + Sub.Offset;
    for (uint32_t Row = 0; Row < Sub.NumRows; ++Row) {
      memcpy(DstPtr, SrcPtr, Sub.RowSizeInBytes);
      DstPtr += Sub.RowSizeInBytes;
      SrcPtr += Sub.RowPitchInBytes;
    }
  }
}
