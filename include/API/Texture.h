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

#include "API/Resources.h"

#include "llvm/ADT/BitmaskEnum.h"
#include "llvm/ADT/StringRef.h"

#include <cstdint>
#include <string>

namespace offloadtest {

// TODO: Add Unorm types (e.g. R8Unorm, RGBA8Unorm) which can be sampled as
// floats.
//
// TODO: Add SRGB types (e.g. RGBA8Srgb) once needed.
enum class TextureFormat {
  R16Sint,
  R16Uint,
  RG16Sint,
  RG16Uint,
  RGBA16Sint,
  RGBA16Uint,
  R32Sint,
  R32Uint,
  R32Float,
  RG32Sint,
  RG32Uint,
  RG32Float,
  RGB32Sint,
  RGB32Uint,
  RGB32Float,
  RGBA32Sint,
  RGBA32Uint,
  RGBA32Float,
  D32Float,
};

enum TextureUsage : uint32_t {
  Sampled = 1 << 0,
  Storage = 1 << 1,
  RenderTarget = 1 << 2,
  DepthStencil = 1 << 3,
  LLVM_MARK_AS_BITMASK_ENUM(/* LargestValue = */ DepthStencil)
};

inline llvm::StringRef getTextureFormatName(TextureFormat Format) {
  switch (Format) {
  case TextureFormat::R16Sint:
    return "R16Sint";
  case TextureFormat::R16Uint:
    return "R16Uint";
  case TextureFormat::RG16Sint:
    return "RG16Sint";
  case TextureFormat::RG16Uint:
    return "RG16Uint";
  case TextureFormat::RGBA16Sint:
    return "RGBA16Sint";
  case TextureFormat::RGBA16Uint:
    return "RGBA16Uint";
  case TextureFormat::R32Sint:
    return "R32Sint";
  case TextureFormat::R32Uint:
    return "R32Uint";
  case TextureFormat::R32Float:
    return "R32Float";
  case TextureFormat::RG32Sint:
    return "RG32Sint";
  case TextureFormat::RG32Uint:
    return "RG32Uint";
  case TextureFormat::RG32Float:
    return "RG32Float";
  case TextureFormat::RGB32Sint:
    return "RGB32Sint";
  case TextureFormat::RGB32Uint:
    return "RGB32Uint";
  case TextureFormat::RGB32Float:
    return "RGB32Float";
  case TextureFormat::RGBA32Sint:
    return "RGBA32Sint";
  case TextureFormat::RGBA32Uint:
    return "RGBA32Uint";
  case TextureFormat::RGBA32Float:
    return "RGBA32Float";
  case TextureFormat::D32Float:
    return "D32Float";
  }
  llvm_unreachable("All TextureFormat cases handled");
}

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

inline bool isDepthFormat(TextureFormat Format) {
  return Format == TextureFormat::D32Float;
}

/// DepthStencil cannot be combined with RenderTarget or Storage.
inline bool isValidTextureUsage(TextureUsage Usage) {
  if ((Usage & DepthStencil) != 0) {
    if ((Usage & RenderTarget) != 0)
      return false;
    if ((Usage & Storage) != 0)
      return false;
  }
  return true;
}

/// Depth formats require DepthStencil usage and cannot be used as RenderTarget
/// or Storage. Non-depth formats cannot be used with DepthStencil usage.
inline bool isValidTextureUsageAndFormat(TextureUsage Usage,
                                         TextureFormat Format) {
  if (!isValidTextureUsage(Usage))
    return false;
  if (isDepthFormat(Format)) {
    // Depth formats can only be used as DepthStencil and/or Sampled.
    if ((Usage & RenderTarget) != 0)
      return false;
    if ((Usage & Storage) != 0)
      return false;
  } else {
    // Non-depth formats cannot be used as DepthStencil.
    if ((Usage & DepthStencil) != 0)
      return false;
  }
  return true;
}

// TODO: Currently only 2D textures are supported. When expanding to 1D, 3D,
// cube, or array textures, add a TextureType enum and validation between usage
// and type (e.g. 3D textures cannot be used as DepthStencil).
struct TextureCreateDesc {
  MemoryLocation Location;
  TextureUsage Usage;
  TextureFormat Format;
  uint32_t Width;
  uint32_t Height;
  uint32_t MipLevels;
};

class Texture {
public:
  virtual ~Texture() = default;

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

protected:
  Texture() = default;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_TEXTURE_H
