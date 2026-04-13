//===- MTLResources.h - Metal Resource Helpers ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_MTLRESOURCES_H
#define OFFLOADTEST_API_MTLRESOURCES_H

#include "API/Device.h"

#include "Metal/Metal.hpp"

namespace offloadtest {

// Metal requires different storage modes for textures and buffers.
// Textures use Managed for GpuToCpu because Shared textures are not available
// on discrete GPUs and lack hardware compression/tiling optimizations.
inline MTL::StorageMode getMetalTextureStorageMode(MemoryLocation Location) {
  switch (Location) {
  case MemoryLocation::GpuOnly:
    return MTL::StorageModePrivate;
  case MemoryLocation::CpuToGpu:
  case MemoryLocation::GpuToCpu:
    return MTL::StorageModeManaged;
  }
  llvm_unreachable("All MemoryLocation cases handled");
}

// Metal requires different storage modes for textures and buffers.
// Buffers use Shared for GpuToCpu because they are linear memory with no
// layout concerns, and Shared avoids the explicit synchronizeResource step
// that Managed requires.
inline MTL::ResourceOptions
getMetalBufferResourceOptions(MemoryLocation Location) {
  switch (Location) {
  case MemoryLocation::GpuOnly:
    return MTL::ResourceStorageModePrivate;
  case MemoryLocation::CpuToGpu:
    return MTL::ResourceStorageModeManaged;
  case MemoryLocation::GpuToCpu:
    return MTL::ResourceStorageModeShared;
  }
  llvm_unreachable("All MemoryLocation cases handled");
}

inline MTL::PixelFormat getMetalPixelFormat(Format Format) {
  switch (Format) {
  case Format::R16Sint:
    return MTL::PixelFormatR16Sint;
  case Format::R16Uint:
    return MTL::PixelFormatR16Uint;
  case Format::RG16Sint:
    return MTL::PixelFormatRG16Sint;
  case Format::RG16Uint:
    return MTL::PixelFormatRG16Uint;
  case Format::RGBA16Sint:
    return MTL::PixelFormatRGBA16Sint;
  case Format::RGBA16Uint:
    return MTL::PixelFormatRGBA16Uint;
  case Format::R32Sint:
    return MTL::PixelFormatR32Sint;
  case Format::R32Uint:
    return MTL::PixelFormatR32Uint;
  case Format::R32Float:
    return MTL::PixelFormatR32Float;
  case Format::RG32Sint:
    return MTL::PixelFormatRG32Sint;
  case Format::RG32Uint:
    return MTL::PixelFormatRG32Uint;
  case Format::RG32Float:
    return MTL::PixelFormatRG32Float;
  // Metal has no 3-component pixel format.
  // RGB32Float is only valid as a vertex format.
  case Format::RGB32Float:
    llvm_unreachable("RGB32Float has no Metal pixel format equivalent");
  case Format::RGBA32Sint:
    return MTL::PixelFormatRGBA32Sint;
  case Format::RGBA32Uint:
    return MTL::PixelFormatRGBA32Uint;
  case Format::RGBA32Float:
    return MTL::PixelFormatRGBA32Float;
  case Format::D32Float:
    return MTL::PixelFormatDepth32Float;
  case Format::D32FloatS8Uint:
    return MTL::PixelFormatDepth32Float_Stencil8;
  }
  llvm_unreachable("All Format cases handled");
}

inline MTL::TextureUsage getMetalTextureUsage(TextureUsage Usage) {
  MTL::TextureUsage Flags = MTL::TextureUsageUnknown;
  if ((Usage & Sampled) != 0)
    Flags |= MTL::TextureUsageShaderRead;
  if ((Usage & Storage) != 0)
    Flags |= MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite;
  if ((Usage & RenderTarget) != 0)
    Flags |= MTL::TextureUsageRenderTarget;
  if ((Usage & DepthStencil) != 0)
    Flags |= MTL::TextureUsageRenderTarget;
  return Flags;
}

inline MTL::VertexFormat getMetalVertexFormat(Format Fmt) {
  switch (Fmt) {
  case Format::R16Sint:
    return MTL::VertexFormatShort;
  case Format::R16Uint:
    return MTL::VertexFormatUShort;
  case Format::RG16Sint:
    return MTL::VertexFormatShort2;
  case Format::RG16Uint:
    return MTL::VertexFormatUShort2;
  case Format::RGBA16Sint:
    return MTL::VertexFormatShort4;
  case Format::RGBA16Uint:
    return MTL::VertexFormatUShort4;
  case Format::R32Sint:
    return MTL::VertexFormatInt;
  case Format::R32Uint:
    return MTL::VertexFormatUInt;
  case Format::R32Float:
    return MTL::VertexFormatFloat;
  case Format::RG32Sint:
    return MTL::VertexFormatInt2;
  case Format::RG32Uint:
    return MTL::VertexFormatUInt2;
  case Format::RG32Float:
    return MTL::VertexFormatFloat2;
  case Format::RGB32Float:
    return MTL::VertexFormatFloat3;
  case Format::RGBA32Sint:
    return MTL::VertexFormatInt4;
  case Format::RGBA32Uint:
    return MTL::VertexFormatUInt4;
  case Format::RGBA32Float:
    return MTL::VertexFormatFloat4;
  // Depth formats cannot be used as vertex attributes.
  case Format::D32Float:
  case Format::D32FloatS8Uint:
    llvm_unreachable("Depth formats are not valid vertex formats");
  }
  llvm_unreachable("All Format cases handled");
}

} // namespace offloadtest

#endif // OFFLOADTEST_API_MTLRESOURCES_H
