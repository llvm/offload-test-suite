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

inline MTL::StorageMode getMetalStorageMode(MemoryLocation Location) {
  switch (Location) {
  case MemoryLocation::GpuOnly:
    return MTL::StorageModePrivate;
  case MemoryLocation::CpuToGpu:
  case MemoryLocation::GpuToCpu:
    return MTL::StorageModeManaged;
  }
  llvm_unreachable("All MemoryLocation cases handled");
}

inline MTL::ResourceOptions getMetalResourceOptions(MemoryLocation Location) {
  switch (Location) {
  case MemoryLocation::GpuOnly:
    return MTL::ResourceStorageModePrivate;
  case MemoryLocation::CpuToGpu:
  case MemoryLocation::GpuToCpu:
    return MTL::ResourceStorageModeManaged;
  }
  llvm_unreachable("All MemoryLocation cases handled");
}

inline MTL::PixelFormat getMetalFormat(TextureFormat Format) {
  switch (Format) {
  case TextureFormat::R16Sint:
    return MTL::PixelFormatR16Sint;
  case TextureFormat::R16Uint:
    return MTL::PixelFormatR16Uint;
  case TextureFormat::RG16Sint:
    return MTL::PixelFormatRG16Sint;
  case TextureFormat::RG16Uint:
    return MTL::PixelFormatRG16Uint;
  case TextureFormat::RGBA16Sint:
    return MTL::PixelFormatRGBA16Sint;
  case TextureFormat::RGBA16Uint:
    return MTL::PixelFormatRGBA16Uint;
  case TextureFormat::R32Sint:
    return MTL::PixelFormatR32Sint;
  case TextureFormat::R32Uint:
    return MTL::PixelFormatR32Uint;
  case TextureFormat::R32Float:
    return MTL::PixelFormatR32Float;
  case TextureFormat::RG32Sint:
    return MTL::PixelFormatRG32Sint;
  case TextureFormat::RG32Uint:
    return MTL::PixelFormatRG32Uint;
  case TextureFormat::RG32Float:
    return MTL::PixelFormatRG32Float;
  case TextureFormat::RGBA32Sint:
    return MTL::PixelFormatRGBA32Sint;
  case TextureFormat::RGBA32Uint:
    return MTL::PixelFormatRGBA32Uint;
  case TextureFormat::RGBA32Float:
    return MTL::PixelFormatRGBA32Float;
  case TextureFormat::D32Float:
    return MTL::PixelFormatDepth32Float;
  }
  llvm_unreachable("All TextureFormat cases handled");
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

} // namespace offloadtest

#endif // OFFLOADTEST_API_MTLRESOURCES_H
