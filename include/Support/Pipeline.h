//===- Pipeline.h - GPU Pipeline Description --------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_SUPPORT_PIPELINE_H
#define OFFLOADTEST_SUPPORT_PIPELINE_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLTraits.h"
#include <memory>
#include <string>

namespace offloadtest {

enum class Stages { Compute };

enum class DataFormat {
  Hex8,
  Hex16,
  Hex32,
  Hex64,
  UInt16,
  UInt32,
  UInt64,
  Int16,
  Int32,
  Int64,
  Float32,
  Float64,
};

enum class ResourceKind {
  Buffer,
  StructuredBuffer,
  RWBuffer,
  RWStructuredBuffer,
  ConstantBuffer,
};

struct DirectXBinding {
  uint32_t Register;
  uint32_t Space;
};

struct OutputProperties {
  int Height;
  int Width;
  int Depth;
};

struct Buffer {
  std::string Name;
  DataFormat Format;
  int Channels;
  int Stride;
  std::unique_ptr<char[]> Data;
  size_t Size;
  OutputProperties OutputProps;

  uint32_t size() const { return Size; }

  uint32_t getSingleElementSize() const {
    switch (Format) {
    case DataFormat::Hex8:
      return 1;
    case DataFormat::Hex16:
    case DataFormat::UInt16:
    case DataFormat::Int16:
      return 2;
    case DataFormat::Hex32:
    case DataFormat::UInt32:
    case DataFormat::Int32:
    case DataFormat::Float32:
      return 4;
    case DataFormat::Hex64:
    case DataFormat::UInt64:
    case DataFormat::Int64:
    case DataFormat::Float64:
      return 8;
    }
    llvm_unreachable("All cases covered.");
  }

  uint32_t getElementSize() const {
    if (Stride > 0)
      return Stride;
    return getSingleElementSize() * Channels;
  }
};

struct Resource {
  ResourceKind Kind;
  std::string Name;
  DirectXBinding DXBinding;
  Buffer *BufferPtr = nullptr;

  bool isRaw() const {
    switch (Kind) {
    case ResourceKind::Buffer:
    case ResourceKind::RWBuffer:
      return false;
    case ResourceKind::StructuredBuffer:
    case ResourceKind::RWStructuredBuffer:
    case ResourceKind::ConstantBuffer:
      return true;
    }
    llvm_unreachable("All cases handled");
  }

  uint32_t getElementSize() const { return BufferPtr->getElementSize(); }

  uint32_t size() const { return BufferPtr->size(); }

  bool isReadWrite() const {
    switch (Kind) {
    case ResourceKind::Buffer:
    case ResourceKind::StructuredBuffer:
    case ResourceKind::ConstantBuffer:
      return false;
    case ResourceKind::RWBuffer:
    case ResourceKind::RWStructuredBuffer:
      return true;
    }
    llvm_unreachable("All cases handled");
  }
};

struct DescriptorSet {
  llvm::SmallVector<Resource> Resources;
};

struct Shader {
  Stages Stage;
  std::string Entry;
  std::unique_ptr<llvm::MemoryBuffer> Shader;
  int DispatchSize[3];
};

struct Pipeline {
  llvm::SmallVector<Shader> Shaders;

  llvm::SmallVector<Buffer> Buffers;
  llvm::SmallVector<DescriptorSet> Sets;

  uint32_t getDescriptorCount() const {
    uint32_t DescriptorCount = 0;
    for (auto &D : Sets)
      DescriptorCount += D.Resources.size();
    return DescriptorCount;
  }
};
} // namespace offloadtest

LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::DescriptorSet)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Resource)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Buffer)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Shader)

namespace llvm {
namespace yaml {

template <> struct MappingTraits<offloadtest::Pipeline> {
  static void mapping(IO &I, offloadtest::Pipeline &P);
};

template <> struct MappingTraits<offloadtest::DescriptorSet> {
  static void mapping(IO &I, offloadtest::DescriptorSet &D);
};

template <> struct MappingTraits<offloadtest::Buffer> {
  static void mapping(IO &I, offloadtest::Buffer &R);
};

template <> struct MappingTraits<offloadtest::Resource> {
  static void mapping(IO &I, offloadtest::Resource &R);
};

template <> struct MappingTraits<offloadtest::DirectXBinding> {
  static void mapping(IO &I, offloadtest::DirectXBinding &B);
};

template <> struct MappingTraits<offloadtest::OutputProperties> {
  static void mapping(IO &I, offloadtest::OutputProperties &P);
};

template <> struct MappingTraits<offloadtest::Shader> {
  static void mapping(IO &I, offloadtest::Shader &B);
};

template <> struct ScalarEnumerationTraits<offloadtest::DataFormat> {
  static void enumeration(IO &I, offloadtest::DataFormat &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::DataFormat::Val)
    ENUM_CASE(Hex8);
    ENUM_CASE(Hex16);
    ENUM_CASE(Hex32);
    ENUM_CASE(Hex64);
    ENUM_CASE(UInt16);
    ENUM_CASE(UInt32);
    ENUM_CASE(UInt64);
    ENUM_CASE(Int16);
    ENUM_CASE(Int32);
    ENUM_CASE(Int64);
    ENUM_CASE(Float32);
    ENUM_CASE(Float64);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<offloadtest::ResourceKind> {
  static void enumeration(IO &I, offloadtest::ResourceKind &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::ResourceKind::Val)
    ENUM_CASE(Buffer);
    ENUM_CASE(StructuredBuffer);
    ENUM_CASE(RWBuffer);
    ENUM_CASE(RWStructuredBuffer);
    ENUM_CASE(ConstantBuffer);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<offloadtest::Stages> {
  static void enumeration(IO &I, offloadtest::Stages &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::Stages::Val)
    ENUM_CASE(Compute);
#undef ENUM_CASE
  }
};
} // namespace yaml
} // namespace llvm

#endif // OFFLOADTEST_SUPPORT_PIPELINE_H
