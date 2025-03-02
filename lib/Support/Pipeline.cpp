//===- Pipeline.cpp - Support Pipeline ------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Support/Pipeline.h"

using namespace offloadtest;

namespace llvm {
namespace yaml {
void MappingTraits<offloadtest::Pipeline>::mapping(IO &I,
                                                   offloadtest::Pipeline &P) {
  I.mapRequired("Shaders", P.Shaders);
  I.mapRequired("Buffers", P.Buffers);
  I.mapRequired("DescriptorSets", P.Sets);
  I.mapOptional("Bindings", P.Bindings);

  if (!I.outputting()) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        R.BufferPtr = P.findBuffer(R.Name);
        if (!R.BufferPtr)
          I.setError(Twine("Referenced buffer ") + R.Name + " not found!");
      }
    }
    if (!P.Bindings.VertexBuffer.empty()) {
      P.Bindings.VertexBufferPtr = P.findBuffer(P.Bindings.VertexBuffer);
      if (!P.Bindings.VertexBufferPtr)
        I.setError(Twine("Referenced vertex buffer ") +
                   P.Bindings.VertexBuffer + " not found!");
    }
  }
}

void MappingTraits<offloadtest::DescriptorSet>::mapping(
    IO &I, offloadtest::DescriptorSet &D) {
  I.mapRequired("Resources", D.Resources);
}

void MappingTraits<offloadtest::Buffer>::mapping(IO &I,
                                                 offloadtest::Buffer &B) {
  I.mapRequired("Name", B.Name);
  I.mapRequired("Format", B.Format);
  I.mapOptional("Channels", B.Channels, 1);
  I.mapOptional("Stride", B.Stride, 0);
  if (!I.outputting() && B.Stride != 0 && B.Channels != 1)
    I.setError("Cannot set a structure stride and more than one channel.");
  switch (B.Format) {
#define DATA_CASE(Enum, Type)                                                  \
  case DataFormat::Enum: {                                                     \
    if (I.outputting()) {                                                      \
      llvm::MutableArrayRef<Type> Arr(reinterpret_cast<Type *>(B.Data.get()),  \
                                      B.Size / sizeof(Type));                  \
      I.mapRequired("Data", Arr);                                              \
    } else {                                                                   \
      int64_t ZeroInitSize;                                                    \
      I.mapOptional("ZeroInitSize", ZeroInitSize, 0);                          \
      if (ZeroInitSize > 0) {                                                  \
        B.Size = ZeroInitSize;                                                 \
        B.Data.reset(new char[B.Size]);                                        \
        memset(B.Data.get(), 0, B.Size);                                       \
        break;                                                                 \
      }                                                                        \
      llvm::SmallVector<Type, 64> Arr;                                         \
      I.mapRequired("Data", Arr);                                              \
      B.Size = Arr.size() * sizeof(Type);                                      \
      B.Data.reset(new char[B.Size]);                                          \
      memcpy(B.Data.get(), Arr.data(), B.Size);                                \
    }                                                                          \
    break;                                                                     \
  }
    DATA_CASE(Hex8, llvm::yaml::Hex8)
    DATA_CASE(Hex16, llvm::yaml::Hex16)
    DATA_CASE(Hex32, llvm::yaml::Hex32)
    DATA_CASE(Hex64, llvm::yaml::Hex64)
    DATA_CASE(UInt16, uint16_t)
    DATA_CASE(UInt32, uint32_t)
    DATA_CASE(UInt64, uint64_t)
    DATA_CASE(Int16, int16_t)
    DATA_CASE(Int32, int32_t)
    DATA_CASE(Int64, int64_t)
    DATA_CASE(Float32, float)
    DATA_CASE(Float64, double)
  }

  I.mapOptional("OutputProps", B.OutputProps);
}

void MappingTraits<offloadtest::Resource>::mapping(IO &I,
                                                   offloadtest::Resource &R) {
  I.mapRequired("Name", R.Name);
  I.mapRequired("Kind", R.Kind);
  I.mapRequired("DirectXBinding", R.DXBinding);
}

void MappingTraits<offloadtest::DirectXBinding>::mapping(
    IO &I, offloadtest::DirectXBinding &B) {
  I.mapRequired("Register", B.Register);
  I.mapRequired("Space", B.Space);
}

void MappingTraits<offloadtest::IOBindings>::mapping(
    IO &I, offloadtest::IOBindings &B) {
  I.mapOptional("VertexBuffer", B.VertexBuffer);
}

void MappingTraits<offloadtest::OutputProperties>::mapping(
    IO &I, offloadtest::OutputProperties &P) {
  I.mapRequired("Height", P.Height);
  I.mapRequired("Width", P.Width);
  I.mapRequired("Depth", P.Depth);
}

void MappingTraits<offloadtest::Shader>::mapping(IO &I,
  offloadtest::Shader &S) {
    I.mapRequired("Stage", S.Stage);
    I.mapRequired("Entry", S.Entry);

    if (S.Stage == Stages::Compute) {
      // Stage-specific data, not sure if this should be optional
      // or moved into the Shaders structure.
      MutableArrayRef<int> MutableDispatchSize(S.DispatchSize);
      I.mapRequired("DispatchSize", MutableDispatchSize);
    }
}
} // namespace yaml
} // namespace llvm
