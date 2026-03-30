//===- DXResources.h - DirectX Resource Helpers ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DXRESOURCES_H
#define OFFLOADTEST_API_DXRESOURCES_H

#include "API/Device.h"

#include <d3d12.h>
#include <dxgiformat.h>

namespace offloadtest {

inline D3D12_HEAP_TYPE getDXHeapType(MemoryLocation Location) {
  switch (Location) {
  case MemoryLocation::GpuOnly:
    return D3D12_HEAP_TYPE_DEFAULT;
  case MemoryLocation::CpuToGpu:
    return D3D12_HEAP_TYPE_UPLOAD;
  case MemoryLocation::GpuToCpu:
    return D3D12_HEAP_TYPE_READBACK;
  }
  llvm_unreachable("All MemoryLocation cases handled");
}

inline DXGI_FORMAT getDXGIFormat(TextureFormat Format) {
  switch (Format) {
  case TextureFormat::R16Sint:
    return DXGI_FORMAT_R16_SINT;
  case TextureFormat::R16Uint:
    return DXGI_FORMAT_R16_UINT;
  case TextureFormat::RG16Sint:
    return DXGI_FORMAT_R16G16_SINT;
  case TextureFormat::RG16Uint:
    return DXGI_FORMAT_R16G16_UINT;
  case TextureFormat::RGBA16Sint:
    return DXGI_FORMAT_R16G16B16A16_SINT;
  case TextureFormat::RGBA16Uint:
    return DXGI_FORMAT_R16G16B16A16_UINT;
  case TextureFormat::R32Sint:
    return DXGI_FORMAT_R32_SINT;
  case TextureFormat::R32Uint:
    return DXGI_FORMAT_R32_UINT;
  case TextureFormat::R32Float:
    return DXGI_FORMAT_R32_FLOAT;
  case TextureFormat::RG32Sint:
    return DXGI_FORMAT_R32G32_SINT;
  case TextureFormat::RG32Uint:
    return DXGI_FORMAT_R32G32_UINT;
  case TextureFormat::RG32Float:
    return DXGI_FORMAT_R32G32_FLOAT;
  case TextureFormat::RGBA32Sint:
    return DXGI_FORMAT_R32G32B32A32_SINT;
  case TextureFormat::RGBA32Uint:
    return DXGI_FORMAT_R32G32B32A32_UINT;
  case TextureFormat::RGBA32Float:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case TextureFormat::D32Float:
    return DXGI_FORMAT_D32_FLOAT;
  case TextureFormat::D32FloatS8Uint:
    return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
  }
  llvm_unreachable("All TextureFormat cases handled");
}

inline D3D12_RESOURCE_FLAGS getDXResourceFlags(TextureUsage Usage) {
  D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
  if ((Usage & TextureUsage::Storage) != 0)
    Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  if ((Usage & TextureUsage::RenderTarget) != 0)
    Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if ((Usage & TextureUsage::DepthStencil) != 0)
    Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  return Flags;
}

} // namespace offloadtest

#endif // OFFLOADTEST_API_DXRESOURCES_H
