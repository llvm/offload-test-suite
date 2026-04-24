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

enum class BufferUsage {
  Storage,
  VertexBuffer,
};

struct BufferCreateDesc {
  MemoryLocation Location;
  BufferUsage Usage;
};

class Buffer {
  GPUAPI API;

public:
  virtual ~Buffer();
  virtual size_t getSizeInBytes() const = 0;

  // Maps the buffer's memory for host access. Only valid for CpuToGpu and
  // GpuToCpu buffers; returns an error for GpuOnly. Each successful map() must
  // be paired with a call to unmap().
  virtual llvm::Expected<void *> map() = 0;
  virtual llvm::Error unmap() = 0;

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  GPUAPI getAPI() const { return API; }

protected:
  explicit Buffer(GPUAPI API) : API(API) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_BUFFER_H
