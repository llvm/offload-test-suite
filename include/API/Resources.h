//===- Resources.h - Offload API shared resource types --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_RESOURCES_H
#define OFFLOADTEST_API_RESOURCES_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

#include <cstdint>

namespace offloadtest {

enum class MemoryLocation {
  GpuOnly,
  CpuToGpu,
  GpuToCpu,
};

// TODO: Add Unorm types (e.g. R8Unorm, RGBA8Unorm) which can be sampled as
// floats.
// TODO: Add SRGB types (e.g. RGBA8Srgb) once needed.
enum class Format {
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
  RGB32Float,
  RGBA32Sint,
  RGBA32Uint,
  RGBA32Float,
  D32Float,
  D32FloatS8Uint,
};

inline llvm::StringRef getFormatName(Format Format) {
  switch (Format) {
  case Format::R16Sint:
    return "R16Sint";
  case Format::R16Uint:
    return "R16Uint";
  case Format::RG16Sint:
    return "RG16Sint";
  case Format::RG16Uint:
    return "RG16Uint";
  case Format::RGBA16Sint:
    return "RGBA16Sint";
  case Format::RGBA16Uint:
    return "RGBA16Uint";
  case Format::R32Sint:
    return "R32Sint";
  case Format::R32Uint:
    return "R32Uint";
  case Format::R32Float:
    return "R32Float";
  case Format::RG32Sint:
    return "RG32Sint";
  case Format::RG32Uint:
    return "RG32Uint";
  case Format::RG32Float:
    return "RG32Float";
  case Format::RGB32Float:
    return "RGB32Float";
  case Format::RGBA32Sint:
    return "RGBA32Sint";
  case Format::RGBA32Uint:
    return "RGBA32Uint";
  case Format::RGBA32Float:
    return "RGBA32Float";
  case Format::D32Float:
    return "D32Float";
  case Format::D32FloatS8Uint:
    return "D32FloatS8Uint";
  }
  llvm_unreachable("All Format cases handled");
}

// Returns the size in bytes of a single texel/element for the given format.
inline uint32_t getFormatSize(Format Format) {
  switch (Format) {
  case Format::R16Sint:
  case Format::R16Uint:
    return 2;
  case Format::RG16Sint:
  case Format::RG16Uint:
  case Format::R32Sint:
  case Format::R32Uint:
  case Format::R32Float:
  case Format::D32Float:
    return 4;
  case Format::RGBA16Sint:
  case Format::RGBA16Uint:
  case Format::RG32Sint:
  case Format::RG32Uint:
  case Format::RG32Float:
  case Format::D32FloatS8Uint:
    return 8;
  case Format::RGB32Float:
    return 12;
  case Format::RGBA32Sint:
  case Format::RGBA32Uint:
  case Format::RGBA32Float:
    return 16;
  }
  llvm_unreachable("All Format cases handled");
}

inline bool isDepthFormat(Format Format) {
  switch (Format) {
  case Format::R16Sint:
  case Format::R16Uint:
  case Format::RG16Sint:
  case Format::RG16Uint:
  case Format::R32Sint:
  case Format::R32Uint:
  case Format::R32Float:
  case Format::RGBA16Sint:
  case Format::RGBA16Uint:
  case Format::RG32Sint:
  case Format::RG32Uint:
  case Format::RG32Float:
  case Format::RGB32Float:
  case Format::RGBA32Sint:
  case Format::RGBA32Uint:
  case Format::RGBA32Float:
    return false;
  case Format::D32Float:
  case Format::D32FloatS8Uint:
    return true;
  }
  llvm_unreachable("All Format cases handled");
}

// Returns true if the format can be used as a texture pixel format across all
// backends. Formats like RGB32Float are valid for vertex attributes but have no
// pixel format equivalent on some APIs (e.g. Metal).
inline bool isTextureCompatible(Format Format) {
  switch (Format) {
  case Format::RGB32Float:
    return false;
  case Format::R16Sint:
  case Format::R16Uint:
  case Format::RG16Sint:
  case Format::RG16Uint:
  case Format::RGBA16Sint:
  case Format::RGBA16Uint:
  case Format::R32Sint:
  case Format::R32Uint:
  case Format::R32Float:
  case Format::RG32Sint:
  case Format::RG32Uint:
  case Format::RG32Float:
  case Format::RGBA32Sint:
  case Format::RGBA32Uint:
  case Format::RGBA32Float:
  case Format::D32Float:
  case Format::D32FloatS8Uint:
    return true;
  }
  llvm_unreachable("All Format cases handled");
}

// Returns true if the format can be used as a vertex attribute.
inline bool isVertexCompatible(Format Format) {
  switch (Format) {
  case Format::R16Sint:
  case Format::R16Uint:
  case Format::RG16Sint:
  case Format::RG16Uint:
  case Format::RGBA16Sint:
  case Format::RGBA16Uint:
  case Format::R32Sint:
  case Format::R32Uint:
  case Format::R32Float:
  case Format::RG32Sint:
  case Format::RG32Uint:
  case Format::RG32Float:
  case Format::RGB32Float:
  case Format::RGBA32Sint:
  case Format::RGBA32Uint:
  case Format::RGBA32Float:
    return true;
  case Format::D32Float:
  case Format::D32FloatS8Uint:
    return false;
  }
  llvm_unreachable("All Format cases handled");
}

// Returns true if the format can be used as a BLAS position attribute for
// raytracing acceleration structure builds. Only a small subset of floating
// point formats are supported across DX12, Vulkan, and Metal.
inline bool isPositionCompatible(Format Format) {
  switch (Format) {
  case Format::RG32Float:
  case Format::RGB32Float:
  case Format::RGBA32Float:
    return true;
  case Format::R16Sint:
  case Format::R16Uint:
  case Format::RG16Sint:
  case Format::RG16Uint:
  case Format::RGBA16Sint:
  case Format::RGBA16Uint:
  case Format::R32Sint:
  case Format::R32Uint:
  case Format::R32Float:
  case Format::RG32Sint:
  case Format::RG32Uint:
  case Format::RGBA32Sint:
  case Format::RGBA32Uint:
  case Format::D32Float:
  case Format::D32FloatS8Uint:
    return false;
  }
  llvm_unreachable("All Format cases handled");
}

} // namespace offloadtest

#endif // OFFLOADTEST_API_RESOURCES_H
