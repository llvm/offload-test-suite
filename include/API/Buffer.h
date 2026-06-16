//===- Buffer.h - Offload API Buffer --------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_BUFFER_H
#define OFFLOADTEST_API_BUFFER_H

#include "API/API.h"
#include "API/Resources.h"

#include "llvm/Support/Casting.h"
#include "llvm/Support/Error.h"

namespace offloadtest {

enum class BufferShaderAccessType {
  Raw,
  Typed,
  Structured,
};

union BufferShaderAccessTypeParams {
  Format Fmt;               // Typed Only
  uint32_t StructureStride; // Structured Only
};

enum class BufferUsage {
  // Generic storage buffer (UAV/SSBO). Also covers acceleration-structure
  // build inputs (vertex/index/instance buffers): backends widen this with
  // any native AS-input flags they need.
  Storage,
  ConstantBuffer,
  IndexBuffer,
  VertexBuffer,
  IndirectArgs,
};

struct BufferCreateDesc {
  MemoryLocation Location;
  MemoryBacking Backing;
  BufferUsage Usage;
  BufferShaderAccessType AccessType;
  BufferShaderAccessTypeParams AccessTypeParams;
  bool HasCounter;

  static BufferCreateDesc uploadBuffer() {
    return BufferCreateDesc{MemoryLocation::CpuToGpu,
                            MemoryBacking::Automatic,
                            BufferUsage::Storage,
                            BufferShaderAccessType::Raw,
                            {},
                            false};
  }

  static BufferCreateDesc readbackBuffer() {
    return BufferCreateDesc{MemoryLocation::GpuToCpu,
                            MemoryBacking::Automatic,
                            BufferUsage::Storage,
                            BufferShaderAccessType::Raw,
                            {},
                            false};
  }

  static BufferCreateDesc scratchBuffer() {
    return BufferCreateDesc{MemoryLocation::GpuOnly,
                            MemoryBacking::Automatic,
                            BufferUsage::Storage,
                            BufferShaderAccessType::Raw,
                            {},
                            false};
  }
};

class Buffer {
  GPUAPI API;

public:
  virtual ~Buffer();
  virtual size_t getSizeInBytes() const = 0;

  // The granularity, in bytes, of a single sparse tile for this buffer. Tile
  // counts are computed as ceil(sizeInBytes / granularity). Only meaningful for
  // buffers created with MemoryBacking::Sparse. The value is backend-defined
  // (e.g. 64 KiB on DX12/Vulkan, commonly 16 KiB on Metal/Apple Silicon).
  virtual size_t querySparseTileSizeInBytes() const = 0;

  // Maps the buffer's memory for host access. Only valid for CpuToGpu and
  // GpuToCpu buffers; returns an error for GpuOnly. Each successful map() must
  // be paired with a call to unmap() before the buffer is used on the GPU.
  virtual llvm::Expected<void *> map() = 0;
  virtual void unmap() = 0;

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  GPUAPI getAPI() const { return API; }
  virtual const BufferCreateDesc &getDesc() const = 0;

protected:
  explicit Buffer(GPUAPI API) : API(API) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_BUFFER_H
