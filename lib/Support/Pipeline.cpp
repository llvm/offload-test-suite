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

template <typename Type> static void setData(IO &I, offloadtest::Buffer &B) {
  if (I.outputting()) {
    llvm::MutableArrayRef<Type> Arr(reinterpret_cast<Type *>(B.Data.get()),
                                    B.Size / sizeof(Type));
    I.mapRequired("Data", Arr);
  } else {
    int64_t ZeroInitSize;
    I.mapOptional("ZeroInitSize", ZeroInitSize, 0);
    if (ZeroInitSize > 0) {
      B.Size = ZeroInitSize;
      B.Data.reset(new char[B.Size]);
      memset(B.Data.get(), 0, B.Size);
      I.mapOptional("OutputProps", B.OutputProps);
      return;
    }
    llvm::SmallVector<Type, 64> Arr;
    I.mapRequired("Data", Arr);
    B.Size = Arr.size() * sizeof(Type);
    B.Data.reset(new char[B.Size]);
    memcpy(B.Data.get(), Arr.data(), B.Size);
  }

  I.mapOptional("OutputProps", B.OutputProps);
}

void MappingTraits<offloadtest::Buffer>::mapping(IO &I,
                                                 offloadtest::Buffer &B) {
  I.mapRequired("Name", B.Name);
  I.mapRequired("Format", B.Format);
  I.mapOptional("Channels", B.Channels, 1);
  I.mapOptional("Stride", B.Stride, 0);
  I.mapOptional("Counter", B.Counter, 0);
  if (!I.outputting() && B.Stride != 0 && B.Channels != 1)
    I.setError("Cannot set a structure stride and more than one channel.");
  using DF = offloadtest::DataFormat;
  switch (B.Format) {
  case DF::Hex8:
    return setData<llvm::yaml::Hex8>(I, B);
  case DF::Hex16:
    return setData<llvm::yaml::Hex16>(I, B);
  case DF::Hex32:
    return setData<llvm::yaml::Hex32>(I, B);
  case DF::Hex64:
    return setData<llvm::yaml::Hex64>(I, B);
  case DF::UInt16:
    return setData<uint16_t>(I, B);
  case DF::UInt32:
    return setData<uint32_t>(I, B);
  case DF::UInt64:
    return setData<uint64_t>(I, B);
  case DF::Int16:
    return setData<int16_t>(I, B);
  case DF::Int32:
    return setData<int32_t>(I, B);
  case DF::Int64:
    return setData<int64_t>(I, B);
  case DF::Float16:
    return setData<llvm::yaml::Hex16>(I, B); // assuming no native float16
  case DF::Float32:
    return setData<float>(I, B);
  case DF::Float64:
    return setData<double>(I, B);
  case DF::Bool:
    return setData<uint32_t>(I, B); // Because sizeof(bool) is 1 but HLSL
                                    // represents a bool using 4 bytes.
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
