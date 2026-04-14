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
#include "llvm/Support/Error.h"

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace offloadtest {

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

// TODO: Currently only 2D textures are supported. When expanding to 1D, 3D,
// cube, or array textures, add a TextureType enum and validation between usage
// and type (e.g. 3D textures cannot be used as DepthStencil).
struct TextureCreateDesc {
  MemoryLocation Location;
  TextureUsage Usage;
  Format Format;
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
  if (!isTextureCompatible(Desc.Format))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Format '%s' is not compatible with texture creation.",
        getFormatName(Desc.Format).data());

  const bool IsDepth = isDepthFormat(Desc.Format);
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

  // Depth formats require DepthStencil usage; non-depth formats forbid it.
  if (IsDepth && !IsDS)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "Depth format '%s' requires DepthStencil usage.",
        getFormatName(Desc.Format).data());
  if (!IsDepth && IsDS)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "DepthStencil usage requires a depth format, got '%s'.",
        getFormatName(Desc.Format).data());

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

class Texture {
public:
  virtual ~Texture();

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

protected:
  Texture() = default;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_TEXTURE_H
