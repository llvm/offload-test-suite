#include "API/Texture.h"
#include "API/Device.h"

#include <algorithm>

// Calculate the size in bytes of the texture data given a linear layout
// Useful for calculating the size for an upload or readback buffer.
size_t
offloadtest::Texture::calculateLinearSizeInBytes(const Device &Dev) const {
  const auto &Desc = getDesc();
  const uint32_t Stride = Dev.getTextureUploadRowStrideInBytes(Desc);
  return (Desc.Height - 1) * Stride +
         Desc.Width * getFormatSizeInBytes(Desc.Fmt);
}

offloadtest::TextureUploadLayout
offloadtest::computeTightTextureUploadLayout(const TextureCreateDesc &Desc) {
  const uint32_t ElementSize = getFormatSizeInBytes(Desc.Fmt);
  TextureUploadLayout Layout;
  Layout.Subresources.reserve(Desc.MipLevels);
  uint64_t Offset = 0;
  for (uint32_t I = 0; I < Desc.MipLevels; ++I) {
    const uint32_t MipWidth = std::max(1u, Desc.Width >> I);
    const uint32_t MipHeight = std::max(1u, Desc.Height >> I);
    SubresourceFootprint Sub;
    Sub.Offset = Offset;
    Sub.RowSizeInBytes = MipWidth * ElementSize;
    Sub.RowPitchInBytes = Sub.RowSizeInBytes;
    Sub.NumRows = MipHeight;
    Layout.Subresources.push_back(Sub);
    Offset += uint64_t(Sub.RowSizeInBytes) * Sub.NumRows;
  }
  Layout.TotalSizeInBytes = Offset;
  return Layout;
}
