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

#include "API/Resources.h"

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
public:
  virtual ~Buffer();
  virtual size_t getSizeInBytes() const = 0;

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

protected:
  Buffer() = default;
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_BUFFER_H
