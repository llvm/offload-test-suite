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

static bool isFloatingPointFormat(DataFormat Format) {
  return Format == DataFormat::Float16 || Format == DataFormat::Float32 ||
         Format == DataFormat::Float64;
}

namespace llvm {
namespace yaml {
void MappingTraits<offloadtest::Pipeline>::mapping(IO &I,
                                                   offloadtest::Pipeline &P) {
  I.mapRequired("Shaders", P.Shaders);
  // Runtime-specific settings.
  I.mapOptional("RuntimeSettings", P.Settings);

  I.mapRequired("Buffers", P.Buffers);
  I.mapOptional("Results", P.Results);
  I.mapRequired("DescriptorSets", P.Sets);

  if (!I.outputting()) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        R.BufferPtr = P.getBuffer(R.Name);
        if (!R.BufferPtr)
          I.setError(Twine("Referenced buffer ") + R.Name + " not found!");
      }
    }
    // Initialize result Buffers
    for (auto &R : P.Results) {
      R.ActualPtr = P.getBuffer(R.Actual);
      if (!R.ActualPtr)
        I.setError(Twine("Reference buffer ") + R.Actual + " not found!");
      R.ExpectedPtr = P.getBuffer(R.Expected);
      if (!R.ExpectedPtr)
        I.setError(Twine("Reference buffer ") + R.Expected + " not found!");
      if (R.ComparisonRule == Rule::BufferFloatULP ||
          R.ComparisonRule == Rule::BufferFloatEpsilon) {
        if (!isFloatingPointFormat(R.ActualPtr->Format) ||
            !isFloatingPointFormat(R.ExpectedPtr->Format))
          I.setError(Twine("BufferFloat only accepts Float buffers"));
        if (R.ActualPtr->Format != R.ExpectedPtr->Format)
          I.setError(Twine("Buffers must have the same type"));
      }
    }

    uint32_t DescriptorTableCount = 0;
    for (auto &R : P.Settings.DX.RootParams) {
      switch (R.Kind) {
      case dx::RootParamKind::DescriptorTable:
        ++DescriptorTableCount;
        break;
      case dx::RootParamKind::Constant: {
        auto &Constant = std::get<dx::RootConstant>(R.Data);
        Constant.BufferPtr = P.getBuffer(Constant.Name);
        if (!Constant.BufferPtr)
          I.setError(Twine("Referenced buffer in root constant ") +
                     Constant.Name + " not found!");
        break;
      }
      case dx::RootParamKind::RootDescriptor: {
        auto &Resource = std::get<dx::RootResource>(R.Data);
        Resource.BufferPtr = P.getBuffer(Resource.Name);
        if (!Resource.BufferPtr)
          I.setError(Twine("Referenced buffer in root descriptor ") +
                     Resource.Name + " not found!");
        break;
      }
      }
    }
    if (P.Settings.DX.RootParams.size() != 0 &&
        DescriptorTableCount != P.Sets.size())
      I.setError(Twine("Expected ") + std::to_string(P.Sets.size()) +
                 " DescriptorTable root parameters, found " +
                 std::to_string(DescriptorTableCount));
  }
}

void MappingTraits<offloadtest::DescriptorSet>::mapping(
    IO &I, offloadtest::DescriptorSet &D) {
  I.mapRequired("Resources", D.Resources);
}

// Data can contain one block of data for a singular resource
// or multiple blocks for a resource array.
// For a single resource the yaml will look like this:
//
//   Data: [0, 1, 2, 3]
//
// For an array of resources the yaml will have a list like this:
//
//   Data:
//     - [0, 1, 2, 3]
//     - [4, 5, 6, 7]
//     - [8, 9, 10, 11]
template <typename T> static void mapBufferData(IO &I, offloadtest::Buffer &B) {
  if (I.outputting()) {
    if (B.ArraySize == 1) {
      // single buffer output
      llvm::MutableArrayRef<T> Arr(reinterpret_cast<T *>(B.Data.back().get()),
                                   B.Size / sizeof(T));
      I.mapRequired("Data", Arr);
    } else {
      // array of buffers output
      llvm::SmallVector<llvm::MutableArrayRef<T>> Arrays;
      for (const auto &D : B.Data)
        Arrays.emplace_back(reinterpret_cast<T *>(D.get()), B.Size / sizeof(T));
      I.mapRequired("Data", Arrays);
    }
    return;
  }

  // zero-initialized buffer(s)
  int64_t ZeroInitSize;
  I.mapOptional("ZeroInitSize", ZeroInitSize, 0);
  if (ZeroInitSize > 0) {
    B.Size = ZeroInitSize;
    for (uint32_t i = 0; i < B.ArraySize; i++) {
      B.Data.push_back(std::make_unique<char[]>(B.Size));
      memset(B.Data.back().get(), 0, B.Size);
    }
    return;
  }

  // single buffer input
  if (B.ArraySize == 1) {
    llvm::SmallVector<T, 64> Arr;
    I.mapRequired("Data", Arr);
    B.Size = Arr.size() * sizeof(T);
    B.Data.push_back(std::make_unique<char[]>(B.Size));
    memcpy(B.Data.back().get(), Arr.data(), B.Size);
    return;
  }

  // array of buffers input
  llvm::SmallVector<llvm::SmallVector<T>> Arrays;
  I.mapRequired("Data", Arrays);
  B.Size = Arrays.back().size() * sizeof(T);

  uint32_t ActualSize = 0;
  for (auto Arr : Arrays) {
    if (Arr.size() * sizeof(T) != B.Size) {
      I.setError("All buffers must have the same size.");
      return;
    }
    B.Data.push_back(std::make_unique<char[]>(B.Size));
    memcpy(B.Data.back().get(), Arr.data(), B.Size);
    ActualSize++;
  }
  if (ActualSize != B.ArraySize)
    I.setError(Twine("Expected ") + std::to_string(B.ArraySize) +
               " buffers, found " + std::to_string(ActualSize));
}

// Counter(s) can contain one counter value for a singular resource
// or multiple values for array of resources with counters.
// For a single resource the yaml will look like this:
//
//   Counter: 5
//
// For an array of resources the yaml will have look like this:
//
//   Counters: [5, 4, 3]
//
static void mapCounters(IO &I, offloadtest::Buffer &B) {
  // counters are printed only if they exist and only on output
  if (!I.outputting() || B.Counters.empty())
    return;
 
  if (B.ArraySize == 1) {
    assert(B.Counters.size() == 1 &&
            "expected a single counter for a single buffer");
    I.mapRequired("Counter", B.Counters.back());
  } else {
    assert(B.Counters.size() == B.ArraySize &&
          "number of counters should match the number of buffers");
    I.mapRequired("Counters", B.Counters);
  }
}

void MappingTraits<offloadtest::Buffer>::mapping(IO &I,
                                                 offloadtest::Buffer &B) {
  I.mapRequired("Name", B.Name);
  I.mapRequired("Format", B.Format);
  I.mapOptional("Channels", B.Channels, 1);
  I.mapOptional("Stride", B.Stride, 0);
  I.mapOptional("ArraySize", B.ArraySize, 1);
  mapCounters(I, B);

  if (!I.outputting() && B.Stride != 0 && B.Channels != 1)
    I.setError("Cannot set a structure stride and more than one channel.");
  switch (B.Format) {
#define DATA_CASE(Enum, Type)                                                  \
  case DataFormat::Enum:                                                       \
    mapBufferData<Type>(I, B);                                                 \
    break;
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
    DATA_CASE(Float16, llvm::yaml::Hex16)
    DATA_CASE(Float32, float)
    DATA_CASE(Float64, double)
    DATA_CASE(Bool, uint32_t) // Because sizeof(bool) is 1 but HLSL represents a bool using 4 bytes.
  }

  I.mapOptional("OutputProps", B.OutputProps);
}

void MappingTraits<offloadtest::Resource>::mapping(IO &I,
                                                   offloadtest::Resource &R) {
  I.mapRequired("Name", R.Name);
  I.mapRequired("Kind", R.Kind);
  I.mapOptional("HasCounter", R.HasCounter, 0);
  I.mapRequired("DirectXBinding", R.DXBinding);
  I.mapOptional("VulkanBinding", R.VKBinding);
}

void MappingTraits<offloadtest::DirectXBinding>::mapping(
    IO &I, offloadtest::DirectXBinding &B) {
  I.mapRequired("Register", B.Register);
  I.mapRequired("Space", B.Space);
}

void MappingTraits<offloadtest::VulkanBinding>::mapping(
    IO &I, offloadtest::VulkanBinding &B) {
  I.mapRequired("Binding", B.Binding);
}

void MappingTraits<offloadtest::OutputProperties>::mapping(
    IO &I, offloadtest::OutputProperties &P) {
  I.mapRequired("Height", P.Height);
  I.mapRequired("Width", P.Width);
  I.mapRequired("Depth", P.Depth);
}

void MappingTraits<offloadtest::dx::RootResource>::mapping(
    IO &I, offloadtest::dx::RootResource &R) {
  I.mapRequired("Name", R.Name);
  I.mapRequired("Kind", R.Kind);
  R.DXBinding = {0, 0};
  R.VKBinding = std::nullopt;
  if (!I.outputting() && !R.isRaw())
    I.setError("Root descriptors must be raw resource types.");
}

void MappingTraits<offloadtest::dx::RootParameter>::mapping(
    IO &I, offloadtest::dx::RootParameter &P) {
  I.mapRequired("Kind", P.Kind);
  switch (P.Kind) {
  case dx::RootParamKind::Constant:
    if (!I.outputting())
      P.Data = dx::RootConstant();
    I.mapRequired("Name", std::get<dx::RootConstant>(P.Data).Name);
    break;
  case dx::RootParamKind::RootDescriptor:
    if (!I.outputting())
      P.Data = dx::RootResource();
    I.mapRequired("Resource", std::get<dx::RootResource>(P.Data));
    break;
  case dx::RootParamKind::DescriptorTable:
    break;
  }
}

void MappingTraits<offloadtest::dx::Settings>::mapping(
    IO &I, offloadtest::dx::Settings &S) {
  I.mapOptional("RootParameters", S.RootParams);
}

void MappingTraits<offloadtest::RuntimeSettings>::mapping(
    IO &I, offloadtest::RuntimeSettings &S) {
  I.mapOptional("DirectX", S.DX);
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
void MappingTraits<offloadtest::Result>::mapping(IO &I,
                                                 offloadtest::Result &R) {
  I.mapRequired("Result", R.Name);
  I.mapRequired("Rule", R.ComparisonRule);
  I.mapRequired("Actual", R.Actual);
  I.mapRequired("Expected", R.Expected);

  switch (R.ComparisonRule) {
  case Rule::BufferFloatULP: {
    I.mapRequired("ULPT", R.ULPT);
    I.mapOptional("DenormMode", R.DM);
    break;
  }
  case Rule::BufferFloatEpsilon: {
    I.mapRequired("Epsilon", R.Epsilon);
    I.mapOptional("DenormMode", R.DM);
    break;
  }
  default:
    break;
  }
}
} // namespace yaml
} // namespace llvm
