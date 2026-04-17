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

namespace offloadtest {

struct BufferCreateDesc {
  MemoryLocation Location;
};

class Buffer {
  GPUAPI Kind;

public:
  virtual ~Buffer();
  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  GPUAPI getKind() const { return Kind; }

protected:
  explicit Buffer(GPUAPI Kind) : Kind(Kind) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_BUFFER_H
