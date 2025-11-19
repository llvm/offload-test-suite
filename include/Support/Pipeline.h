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
#include <variant>

namespace offloadtest {

enum class Stages { Compute, Vertex, Pixel };

enum class Rule { BufferExact, BufferFloatULP, BufferFloatEpsilon };

enum class DenormMode { Any, FTZ, Preserve };

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
  Float16,
  Float32,
  Float64,
  Bool,
};

enum class ResourceKind {
  Buffer,
  StructuredBuffer,
  ByteAddressBuffer,
  Texture2D,
  RWBuffer,
  RWStructuredBuffer,
  RWByteAddressBuffer,
  RWTexture2D,
  ConstantBuffer,
};

struct DirectXBinding {
  uint32_t Register;
  uint32_t Space;
};

struct VulkanBinding {
  uint32_t Binding;
  std::optional<uint32_t> CounterBinding;
};

struct OutputProperties {
  int Height;
  int Width;
  int Depth;
};

static inline uint32_t getFormatSize(DataFormat Format) {
  switch (Format) {
  case DataFormat::Hex8:
    return 1;
  case DataFormat::Hex16:
  case DataFormat::UInt16:
  case DataFormat::Int16:
  case DataFormat::Float16:
    return 2;
  case DataFormat::Hex32:
  case DataFormat::UInt32:
  case DataFormat::Int32:
  case DataFormat::Float32:
  case DataFormat::Bool:
    return 4;
  case DataFormat::Hex64:
  case DataFormat::UInt64:
  case DataFormat::Int64:
  case DataFormat::Float64:
    return 8;
  }
  llvm_unreachable("All cases covered.");
}

struct Buffer {
  std::string Name;
  DataFormat Format;
  int Channels;
  int Stride;
  uint32_t ArraySize;
  // Data can contain one block of data for a singular resource
  // or multiple blocks for a resource array.
  llvm::SmallVector<std::unique_ptr<char[]>> Data;
  size_t Size;
  OutputProperties OutputProps;
  // Counters can contain one counter value for a singular resource
  // or multiple values for an array of resources with counters.
  llvm::SmallVector<uint32_t> Counters;

  uint32_t size() const { return Size; }

  uint32_t getSingleElementSize() const { return getFormatSize(Format); }

  uint32_t getElementSize() const {
    if (Stride > 0)
      return Stride;
    return getSingleElementSize() * Channels;
  }
};

struct Result {
  std::string Name;
  Rule ComparisonRule;
  std::string Actual;
  std::string Expected;
  Buffer *ActualPtr = nullptr;
  Buffer *ExpectedPtr = nullptr;
  DenormMode DM = DenormMode::Any;
  unsigned ULPT; // ULP Tolerance
  double Epsilon;
};

struct Resource {
  ResourceKind Kind;
  std::string Name;
  DirectXBinding DXBinding;
  std::optional<VulkanBinding> VKBinding;
  Buffer *BufferPtr = nullptr;
  bool HasCounter;

  bool isRaw() const {
    switch (Kind) {
    case ResourceKind::Buffer:
    case ResourceKind::RWBuffer:
    case ResourceKind::Texture2D:
    case ResourceKind::RWTexture2D:
      return false;
    case ResourceKind::StructuredBuffer:
    case ResourceKind::RWStructuredBuffer:
    case ResourceKind::ByteAddressBuffer:
    case ResourceKind::RWByteAddressBuffer:
    case ResourceKind::ConstantBuffer:
      return true;
    }
    llvm_unreachable("All cases handled");
  }

  bool isTexture() const {
    switch (Kind) {
    case ResourceKind::Buffer:
    case ResourceKind::RWBuffer:
    case ResourceKind::StructuredBuffer:
    case ResourceKind::RWStructuredBuffer:
    case ResourceKind::ByteAddressBuffer:
    case ResourceKind::RWByteAddressBuffer:
    case ResourceKind::ConstantBuffer:
      return false;
    case ResourceKind::Texture2D:
    case ResourceKind::RWTexture2D:
      return true;
    }
    llvm_unreachable("All cases handled");
  }

  bool isByteAddressBuffer() const {
    switch (Kind) {
    case ResourceKind::ByteAddressBuffer:
    case ResourceKind::RWByteAddressBuffer:
      return true;
    default:
      return false;
    }
  }

  bool isStructuredBuffer() const {
    switch (Kind) {
    case ResourceKind::StructuredBuffer:
    case ResourceKind::RWStructuredBuffer:
      return true;
    default:
      return false;
    }
  }

  uint32_t getElementSize() const {
    // ByteAddressBuffer uses DXGI_FORMAT_R32_TYPELESS which has 4-byte elements
    return isByteAddressBuffer() ? 4 : BufferPtr->getElementSize();
  }

  uint32_t size() const { return BufferPtr->size(); }

  bool isReadWrite() const {
    switch (Kind) {
    case ResourceKind::Buffer:
    case ResourceKind::StructuredBuffer:
    case ResourceKind::ByteAddressBuffer:
    case ResourceKind::Texture2D:
    case ResourceKind::ConstantBuffer:
      return false;
    case ResourceKind::RWBuffer:
    case ResourceKind::RWStructuredBuffer:
    case ResourceKind::RWByteAddressBuffer:
    case ResourceKind::RWTexture2D:
      return true;
    }
    llvm_unreachable("All cases handled");
  }

  bool isReadOnly() const { return !isReadWrite(); }
};

struct DescriptorSet {
  llvm::SmallVector<Resource> Resources;
};
namespace dx {
enum class RootParamKind {
  Constant,
  DescriptorTable,
  RootDescriptor,
};

struct RootResource : public Resource {};

struct RootConstant {
  Buffer *BufferPtr;
  std::string Name;
};

struct RootParameter {
  RootParamKind Kind;

  std::variant<RootConstant, RootResource> Data;
};
struct Settings {
  llvm::SmallVector<RootParameter> RootParams;
};
} // namespace dx

struct RuntimeSettings {
  dx::Settings DX;
};

struct VertexAttribute {
  DataFormat Format;
  int Channels;
  int Offset;
  std::string Name;

  uint32_t size() const { return getFormatSize(Format) * Channels; }
};

struct IOBindings {
  std::string VertexBuffer;
  Buffer *VertexBufferPtr;
  llvm::SmallVector<VertexAttribute> VertexAttributes;

  std::string RenderTarget;
  Buffer *RTargetBufferPtr;

  uint32_t getVertexStride() const {
    uint32_t Stride = 0;
    for (auto VA : VertexAttributes)
      Stride += VA.size();
    return Stride;
  }

  uint32_t getVertexCount() const {
    return VertexBufferPtr->size() / getVertexStride();
  }
};

struct SpecializationConstant {
  uint32_t ConstantID;
  DataFormat Type;
  std::string Value;
};

struct Shader {
  Stages Stage;
  std::string Entry;
  std::unique_ptr<llvm::MemoryBuffer> Shader;
  int DispatchSize[3];
  llvm::SmallVector<SpecializationConstant> SpecializationConstants;
};

struct Pipeline {
  llvm::SmallVector<Shader> Shaders;
  RuntimeSettings Settings;

  IOBindings Bindings;
  llvm::SmallVector<Buffer> Buffers;
  llvm::SmallVector<Result> Results;
  llvm::SmallVector<DescriptorSet> Sets;

  uint32_t getDescriptorCount() const {
    uint32_t DescriptorCount = 0;
    for (auto &D : Sets)
      DescriptorCount += D.Resources.size();
    return DescriptorCount;
  }

  uint32_t getDescriptorCountWithFlattenedArrays() const {
    uint32_t DescriptorCount = 0;
    for (auto &D : Sets)
      for (auto &R : D.Resources)
        DescriptorCount += R.BufferPtr->ArraySize;
    return DescriptorCount;
  }

  Buffer *getBuffer(llvm::StringRef Name) {
    for (auto &B : Buffers)
      if (Name == B.Name)
        return &B;
    return nullptr;
  }

  bool isGraphics() const { return !isCompute(); }

  bool isCompute() const {
    return Shaders.size() == 1 && Shaders[0].Stage == Stages::Compute;
  }
};
} // namespace offloadtest

LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::DescriptorSet)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Resource)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Buffer)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Shader)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::dx::RootParameter)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::Result)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::VertexAttribute)
LLVM_YAML_IS_SEQUENCE_VECTOR(offloadtest::SpecializationConstant)

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

template <> struct MappingTraits<offloadtest::Result> {
  static void mapping(IO &I, offloadtest::Result &R);
};

template <> struct MappingTraits<offloadtest::Resource> {
  static void mapping(IO &I, offloadtest::Resource &R);
};

template <> struct MappingTraits<offloadtest::DirectXBinding> {
  static void mapping(IO &I, offloadtest::DirectXBinding &B);
};

template <> struct MappingTraits<offloadtest::VulkanBinding> {
  static void mapping(IO &I, offloadtest::VulkanBinding &B);
};

template <> struct MappingTraits<offloadtest::IOBindings> {
  static void mapping(IO &I, offloadtest::IOBindings &B);
};

template <> struct MappingTraits<offloadtest::VertexAttribute> {
  static void mapping(IO &I, offloadtest::VertexAttribute &A);
};

template <> struct MappingTraits<offloadtest::OutputProperties> {
  static void mapping(IO &I, offloadtest::OutputProperties &P);
};

template <> struct MappingTraits<offloadtest::Shader> {
  static void mapping(IO &I, offloadtest::Shader &B);
};

template <> struct MappingTraits<offloadtest::dx::RootResource> {
  static void mapping(IO &I, offloadtest::dx::RootResource &R);
};

template <> struct MappingTraits<offloadtest::dx::RootParameter> {
  static void mapping(IO &I, offloadtest::dx::RootParameter &S);
};

template <> struct MappingTraits<offloadtest::dx::Settings> {
  static void mapping(IO &I, offloadtest::dx::Settings &S);
};

template <> struct MappingTraits<offloadtest::RuntimeSettings> {
  static void mapping(IO &I, offloadtest::RuntimeSettings &S);
};

template <> struct MappingTraits<offloadtest::SpecializationConstant> {
  static void mapping(IO &I, offloadtest::SpecializationConstant &C);
};

template <> struct ScalarEnumerationTraits<offloadtest::Rule> {
  static void enumeration(IO &I, offloadtest::Rule &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::Rule::Val)
    ENUM_CASE(BufferExact);
    ENUM_CASE(BufferFloatULP);
    ENUM_CASE(BufferFloatEpsilon);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<offloadtest::DenormMode> {
  static void enumeration(IO &I, offloadtest::DenormMode &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::DenormMode::Val)
    ENUM_CASE(Any);
    ENUM_CASE(FTZ);
    ENUM_CASE(Preserve);
#undef ENUM_CASE
  }
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
    ENUM_CASE(Float16);
    ENUM_CASE(Float32);
    ENUM_CASE(Float64);
    ENUM_CASE(Bool);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<offloadtest::ResourceKind> {
  static void enumeration(IO &I, offloadtest::ResourceKind &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::ResourceKind::Val)
    ENUM_CASE(Buffer);
    ENUM_CASE(StructuredBuffer);
    ENUM_CASE(ByteAddressBuffer);
    ENUM_CASE(Texture2D);
    ENUM_CASE(RWBuffer);
    ENUM_CASE(RWStructuredBuffer);
    ENUM_CASE(RWByteAddressBuffer);
    ENUM_CASE(RWTexture2D);
    ENUM_CASE(ConstantBuffer);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<offloadtest::Stages> {
  static void enumeration(IO &I, offloadtest::Stages &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::Stages::Val)
    ENUM_CASE(Compute);
    ENUM_CASE(Vertex);
    ENUM_CASE(Pixel);
#undef ENUM_CASE
  }
};

template <> struct ScalarEnumerationTraits<offloadtest::dx::RootParamKind> {
  static void enumeration(IO &I, offloadtest::dx::RootParamKind &V) {
#define ENUM_CASE(Val) I.enumCase(V, #Val, offloadtest::dx::RootParamKind::Val)
    ENUM_CASE(Constant);
    ENUM_CASE(DescriptorTable);
    ENUM_CASE(RootDescriptor);
#undef ENUM_CASE
  }
};

template <typename T> struct SequenceTraits<SmallVector<SmallVector<T>>> {
  static size_t size(IO &io, SmallVector<SmallVector<T>> &seq) {
    return seq.size();
  }

  static SmallVector<T> &element(IO &io, SmallVector<SmallVector<T>> &seq,
                                 size_t index) {
    if (index >= seq.size())
      seq.resize(index + 1);
    return seq[index];
  }
};

template <typename T> struct SequenceTraits<SmallVector<MutableArrayRef<T>>> {
  static size_t size(IO &io, SmallVector<MutableArrayRef<T>> &seq) {
    return seq.size();
  }

  static MutableArrayRef<T> &
  element(IO &io, SmallVector<MutableArrayRef<T>> &seq, size_t index) {
    assert(index < seq.size());
    return seq[index];
  }
};

} // namespace yaml
} // namespace llvm

#endif // OFFLOADTEST_SUPPORT_PIPELINE_H
