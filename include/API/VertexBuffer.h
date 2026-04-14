//===- VertexBuffer.h - Offload API Vertex Buffer -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_VERTEXBUFFER_H
#define OFFLOADTEST_API_VERTEXBUFFER_H

#include "API/Buffer.h"
#include "API/Resources.h"

#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>

namespace offloadtest {

struct VertexStream {
  std::string Name; // Semantic name (e.g. POSITION, COLOR).
  Format Fmt;
};

struct VertexBufferDesc {
  llvm::SmallVector<VertexStream> Streams;

  uint32_t getStride() const {
    uint32_t Stride = 0;
    for (const auto &S : Streams)
      Stride += getFormatSizeInBytes(S.Fmt);
    return Stride;
  }

  // Returns the byte offset of the stream at the given index.
  uint32_t getOffset(uint32_t Index) const {
    assert(Index < Streams.size() && "Stream index out of bounds");
    uint32_t Offset = 0;
    for (uint32_t I = 0; I < Index; ++I)
      Offset += getFormatSizeInBytes(Streams[I].Fmt);
    return Offset;
  }
};

struct VertexBuffer {
  VertexBufferDesc Desc;
  std::shared_ptr<Buffer> Data;

  uint32_t getVertexCount() const {
    uint32_t Stride = Desc.getStride();
    if (Stride == 0)
      return 0;
    return static_cast<uint32_t>(Data->getSizeInBytes()) / Stride;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_VERTEXBUFFER_H
