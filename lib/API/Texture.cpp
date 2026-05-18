#include "API/Texture.h"
#include "API/Device.h"

// Calculate the size in bytes of the texture data given a linear layout
// Useful for calculating the size for an upload or readback buffer.
size_t offloadtest::Texture::calculateLinearSizeInBytes(Device &Dev) const {
  const auto &Desc = getDesc();
  const uint32_t Stride = Dev.getTextureUploadRowStrideInBytes(Desc);
  return (Desc.Height - 1) * Stride +
         Desc.Width * getFormatSizeInBytes(Desc.Fmt);
}
