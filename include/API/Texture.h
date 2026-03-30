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
#include "Support/Pipeline.h"

#include "llvm/ADT/BitmaskEnum.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace offloadtest {

// TODO: Add Unorm types (e.g. R8Unorm, RGBA8Unorm) which can be sampled as
// floats.
// TODO: Add SRGB types (e.g. RGBA8Srgb) once needed.
//
// Note: No 3-channel formats due to lack of Metal support.
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

struct ClearColor {
  float R = 0.0f, G = 0.0f, B = 0.0f, A = 0.0f;
};

struct ClearDepthStencil {
  float Depth = 1.0f;
  uint8_t Stencil = 0;
};

using ClearValue = std::variant<ClearColor, ClearDepthStencil>;

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
  // Clear value for render target or depth/stencil textures.
  // How and when this is applied depends on the backend:
  // - DX uses it as an optimized clear hint at resource creation time
  // - VK and MTL apply it at render pass begin
  std::optional<ClearValue> OptimizedClearValue;
};

inline llvm::Error validateTextureCreateDesc(const TextureCreateDesc &Desc) {
  bool IsDepth = Desc.Format == TextureFormat::D32Float;
  bool IsRT = (Desc.Usage & TextureUsage::RenderTarget) != 0;
  bool IsDS = (Desc.Usage & TextureUsage::DepthStencil) != 0;

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

  // Depth formats require DepthStencil usage; non-depth formats forbid it.
  if (IsDepth && !IsDS)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Depth format '%s' requires DepthStencil usage.",
        getTextureFormatName(Desc.Format).data());
  if (!IsDepth && IsDS)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "DepthStencil usage requires a depth format, got '%s'.",
        getTextureFormatName(Desc.Format).data());

  // Render targets and depth/stencil textures only support a single mip level.
  if ((IsRT || IsDS) && Desc.MipLevels != 1)
    return llvm::createStringError(
        std::errc::not_supported,
        "Multiple mip levels are not supported for render target or "
        "depth/stencil textures.");

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

// Bridge for code that still describes textures as DataFormat + Channels (e.g.
// render targets bound via CPUBuffer). Once the pipeline is refactored to use
// TextureFormat directly, this function can be removed.
inline llvm::Expected<TextureFormat>
toTextureFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int16:
    switch (Channels) {
    case 1: return TextureFormat::R16Sint;
    case 2: return TextureFormat::RG16Sint;
    case 4: return TextureFormat::RGBA16Sint;
    }
    break;
  case DataFormat::UInt16:
    switch (Channels) {
    case 1: return TextureFormat::R16Uint;
    case 2: return TextureFormat::RG16Uint;
    case 4: return TextureFormat::RGBA16Uint;
    }
    break;
  case DataFormat::Int32:
    switch (Channels) {
    case 1: return TextureFormat::R32Sint;
    case 2: return TextureFormat::RG32Sint;
    case 4: return TextureFormat::RGBA32Sint;
    }
    break;
  case DataFormat::UInt32:
    switch (Channels) {
    case 1: return TextureFormat::R32Uint;
    case 2: return TextureFormat::RG32Uint;
    case 4: return TextureFormat::RGBA32Uint;
    }
    break;
  case DataFormat::Float32:
    switch (Channels) {
    case 1: return TextureFormat::R32Float;
    case 2: return TextureFormat::RG32Float;
    case 4: return TextureFormat::RGBA32Float;
    }
    break;
  case DataFormat::Depth32:
    if (Channels == 1)
      return TextureFormat::D32Float;
    break;
  // No TextureFormat mapping for these DataFormats.
  case DataFormat::Hex8:
  case DataFormat::Hex16:
  case DataFormat::Hex32:
  case DataFormat::Hex64:
  case DataFormat::UInt64:
  case DataFormat::Int64:
  case DataFormat::Float16:
  case DataFormat::Float64:
  case DataFormat::Bool:
    return llvm::createStringError(
        std::errc::invalid_argument,
        "DataFormat %d has no TextureFormat equivalent.",
        static_cast<int>(Format));
  }
  return llvm::createStringError(std::errc::invalid_argument,
                                 "No TextureFormat for DataFormat %d with %d "
                                 "channel(s).",
                                 static_cast<int>(Format), Channels);
}

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
