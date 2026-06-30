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
#include "llvm/ADT/DenseMap.h"

using namespace offloadtest;

static bool isFloatingPointFormat(DataFormat Format) {
  return Format == DataFormat::Float16 || Format == DataFormat::Float32 ||
         Format == DataFormat::Float64;
}

void PushConstantBlock::getContent(
    llvm::SmallVectorImpl<uint8_t> &Output) const {
  Output.resize(size());
  for (const PushConstantValue &V : Values)
    memcpy(Output.data() + V.OffsetInBytes, V.Data.data(), V.Data.size());
}

void CPUBuffer::copyFromTexture(const void *Src, size_t SrcRowPitch) {
  const uint32_t Height = OutputProps.Height;
  const uint32_t RowBytes = getImageRowBytes();
  assert(SrcRowPitch >= RowBytes && "Source row pitch is smaller than image");
  uint8_t *Dst = reinterpret_cast<uint8_t *>(Data[0].get());
  if (SrcRowPitch == RowBytes) {
    memcpy(Dst, Src, static_cast<size_t>(Height) * RowBytes);
    return;
  }
  const uint8_t *S = reinterpret_cast<const uint8_t *>(Src);
  for (uint32_t Y = 0; Y < Height; ++Y)
    memcpy(Dst + static_cast<size_t>(Y) * RowBytes,
           S + static_cast<size_t>(Y) * SrcRowPitch, RowBytes);
}

uint32_t PushConstantBlock::size() const {
  uint32_t Size = 0;
  for (const PushConstantValue &V : Values)
    Size =
        std::max(Size, V.OffsetInBytes + static_cast<uint32_t>(V.Data.size()));
  return Size;
}

namespace llvm {
namespace yaml {

void MappingTraits<offloadtest::Pipeline>::mapping(IO &I,
                                                   offloadtest::Pipeline &P) {
  I.mapRequired("Shaders", P.Shaders);

  // Runtime-specific settings.
  I.mapOptional("RuntimeSettings", P.Settings);

  I.mapRequired("Buffers", P.Buffers);
  I.mapOptional("Samplers", P.Samplers);
  I.mapOptional("Results", P.Results);
  I.mapRequired("DescriptorSets", P.Sets);
  I.mapOptional("Bindings", P.Bindings);
  I.mapOptional("PushConstants", P.PushConstants);
  I.mapOptional("AccelerationStructures", P.AccelStructs);
  I.mapOptional("RayTracingPipelineConfig", P.RTConfig);
  I.mapOptional("HitGroups", P.HitGroups);
  I.mapOptional("ShaderBindingTable", P.SBT);

  // Runs here (not right after Shaders) because the tessellation topology
  // check reads Bindings.Topology and Bindings.PatchControlPoints, and the
  // RT-pipeline check reads HitGroups / RTConfig / SBT. Must still run
  // before validateDispatchParameters, which reads P.Kind.
  if (auto Err = P.validatePipelineKind())
    I.setError(llvm::toString(std::move(Err)));

  I.mapOptional("DispatchParameters", P.DispatchParameters);
  if (auto Err = P.validateDispatchParameters())
    I.setError(llvm::toString(std::move(Err)));

  if (!I.outputting()) {
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        if (R.isAccelerationStructure()) {
          R.TLASPtr = P.getTLAS(R.Name);
          if (!R.TLASPtr)
            I.setError(Twine("Referenced TLAS ") + R.Name + " not found!");
        } else if (R.isSampledTexture()) {
          R.SamplerPtr = P.getSampler(R.Name);
          if (!R.SamplerPtr)
            I.setError(Twine("Referenced sampler ") + R.Name + " not found!");
          R.BufferPtr = P.getBuffer(R.Name);
          if (!R.BufferPtr)
            I.setError(Twine("Referenced buffer ") + R.Name + " not found!");
        } else if (R.isSampler()) {
          R.SamplerPtr = P.getSampler(R.Name);
          if (!R.SamplerPtr)
            I.setError(Twine("Referenced sampler ") + R.Name + " not found!");
        } else {
          R.BufferPtr = P.getBuffer(R.Name);
          if (!R.BufferPtr)
            I.setError(Twine("Referenced buffer ") + R.Name + " not found!");
        }
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

    if (!P.Bindings.VertexBuffer.empty()) {
      P.Bindings.VertexBufferPtr = P.getBuffer(P.Bindings.VertexBuffer);
      if (!P.Bindings.VertexBufferPtr)
        I.setError(Twine("Referenced vertex buffer ") +
                   P.Bindings.VertexBuffer + " not found!");
    }

    if (!P.Bindings.RenderTarget.empty()) {
      P.Bindings.RTargetBufferPtr = P.getBuffer(P.Bindings.RenderTarget);
      if (!P.Bindings.RTargetBufferPtr)
        I.setError(Twine("Referenced render target buffer ") +
                   P.Bindings.RenderTarget + " not found!");
    }

    // Resolve buffer name references in acceleration structure descriptions.
    for (auto &B : P.AccelStructs.BLAS) {
      for (auto &T : B.Triangles) {
        T.VertexBufferPtr = P.getBuffer(T.VertexBuffer);
        if (!T.VertexBufferPtr)
          I.setError(Twine("BLAS '") + B.Name +
                     "': referenced vertex buffer '" + T.VertexBuffer +
                     "' not found!");
        if (!T.IndexBuffer.empty()) {
          T.IndexBufferPtr = P.getBuffer(T.IndexBuffer);
          if (!T.IndexBufferPtr)
            I.setError(Twine("BLAS '") + B.Name +
                       "': referenced index buffer '" + T.IndexBuffer +
                       "' not found!");
        }
      }
      for (auto &A : B.AABBs) {
        A.AABBBufferPtr = P.getBuffer(A.AABBBuffer);
        if (!A.AABBBufferPtr)
          I.setError(Twine("BLAS '") + B.Name + "': referenced AABB buffer '" +
                     A.AABBBuffer + "' not found!");
      }
    }

    // Resolve BLAS name references in TLAS instance descriptions.
    for (auto &T : P.AccelStructs.TLAS) {
      for (auto &Inst : T.Instances) {
        for (int Idx = 0, E = P.AccelStructs.BLAS.size(); Idx < E; ++Idx) {
          if (P.AccelStructs.BLAS[Idx].Name == Inst.BLAS) {
            Inst.BLASIdx = Idx;
            break;
          }
        }
        if (Inst.BLASIdx < 0)
          I.setError(Twine("TLAS '") + T.Name + "': referenced BLAS '" +
                     Inst.BLAS + "' not found!");
      }
    }
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
template <typename T> static void setData(IO &I, offloadtest::CPUBuffer &B) {
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

  // Buffers can be initialized to be filled with a fixed value.
  int64_t FillSize;

  // Explicitly reject ZeroInitSize to avoid a confusing error while
  // transitioning to FillSize. We can remove this once in flight PRs have had
  // time to go in.
  I.mapOptional("ZeroInitSize", FillSize, 0);
  if (FillSize > 0) {
    I.setError("invalid key 'ZeroInitSize' - did you mean 'FillSize'?");
    return;
  }

  T FillValue;
  I.mapOptional("FillSize", FillSize, 0);
  I.mapOptional("FillValue", FillValue, T{});
  if (FillSize > 0) {
    B.Size = FillSize;
    llvm::SmallVector<T> FillData(FillSize);
    std::fill(FillData.begin(), FillData.end(), FillValue);

    for (uint32_t I = 0; I < B.ArraySize; I++) {
      B.Data.push_back(std::make_unique<char[]>(B.Size));
      memcpy(B.Data.back().get(), FillData.data(), B.Size);
    }
    return;
  }
  if (FillValue) {
    I.setError("'FillValue' specified without 'FillSize'");
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
static void setCounters(IO &I, offloadtest::CPUBuffer &B) {
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

void MappingTraits<offloadtest::Sampler>::mapping(IO &I,
                                                  offloadtest::Sampler &S) {
  I.mapRequired("Name", S.Name);
  I.mapOptional("Kind", S.Kind);
  I.mapOptional("MinFilter", S.MinFilter);
  I.mapOptional("MagFilter", S.MagFilter);
  I.mapOptional("Address", S.Address);
  I.mapOptional("MinLOD", S.MinLOD);
  I.mapOptional("MaxLOD", S.MaxLOD);
  I.mapOptional("MipLODBias", S.MipLODBias);
  I.mapOptional("ComparisonOp", S.ComparisonOp);
  if (!I.outputting()) {
    if (S.Kind == SamplerKind::Sampler &&
        S.ComparisonOp != CompareFunction::Never) {
      I.setError("Sampler kind 'Sampler' requires ComparisonOp: Never");
      return;
    }
    if (S.Kind == SamplerKind::SamplerComparison &&
        S.ComparisonOp == CompareFunction::Never) {
      I.setError(
          "Sampler kind 'SamplerComparison' requires ComparisonOp other than "
          "Never");
      return;
    }
  }
}

void MappingTraits<offloadtest::CPUBuffer>::mapping(IO &I,
                                                    offloadtest::CPUBuffer &B) {
  I.mapRequired("Name", B.Name);
  I.mapRequired("Format", B.Format);
  I.mapOptional("Channels", B.Channels, 1);
  I.mapOptional("Stride", B.Stride, 0);
  I.mapOptional("ArraySize", B.ArraySize, 1);
  setCounters(I, B);

  if (!I.outputting() && B.Stride != 0 && B.Channels != 1)
    I.setError("Cannot set a structure stride and more than one channel.");
  using DF = offloadtest::DataFormat;
  switch (B.Format) {
  case DF::Hex8:
    setData<llvm::yaml::Hex8>(I, B);
    break;
  case DF::Hex16:
    setData<llvm::yaml::Hex16>(I, B);
    break;
  case DF::Hex32:
    setData<llvm::yaml::Hex32>(I, B);
    break;
  case DF::Hex64:
    setData<llvm::yaml::Hex64>(I, B);
    break;
  case DF::UInt16:
    setData<uint16_t>(I, B);
    break;
  case DF::UInt32:
    setData<uint32_t>(I, B);
    break;
  case DF::UInt64:
    setData<uint64_t>(I, B);
    break;
  case DF::Int16:
    setData<int16_t>(I, B);
    break;
  case DF::Int32:
    setData<int32_t>(I, B);
    break;
  case DF::Int64:
    setData<int64_t>(I, B);
    break;
  case DF::Float16:
    setData<llvm::yaml::Hex16>(I, B); // assuming no native float16
    break;
  case DF::Float32:
    setData<float>(I, B);
    break;
  case DF::Depth32:
    setData<float>(I, B);
    break;
  case DF::Float64:
    setData<double>(I, B);
    break;
  case DF::Bool:
    setData<uint32_t>(I, B); // Because sizeof(bool) is 1 but HLSL
                             // represents a bool using 4 bytes.
    break;
  }

  I.mapOptional("OutputProps", B.OutputProps);

  if (!I.outputting() && B.OutputProps.Width > 0) {
    uint32_t ExpectedSize = 0;
    uint32_t W = B.OutputProps.Width;
    uint32_t H = B.OutputProps.Height;
    uint32_t D = B.OutputProps.Depth;
    const uint32_t ElementSize = B.getElementSize();
    for (int I = 0; I < B.OutputProps.MipLevels; ++I) {
      ExpectedSize += W * H * D * ElementSize;
      W = std::max(1u, W / 2);
      H = std::max(1u, H / 2);
      D = std::max(1u, D / 2);
    }

    if (B.Size != ExpectedSize)
      I.setError(Twine("Buffer '") + B.Name + "' size (" + Twine(B.Size) +
                 ") does not match OutputProps dimensions (" +
                 Twine(ExpectedSize) + ")");
  }
}

void MappingTraits<offloadtest::Resource>::mapping(IO &I,
                                                   offloadtest::Resource &R) {
  I.mapRequired("Name", R.Name);
  I.mapRequired("Kind", R.Kind);
  I.mapOptional("HasCounter", R.HasCounter, 0);
  I.mapOptional("TilesMapped", R.TilesMapped);
  I.mapOptional("IsReserved", R.IsReserved);
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
  I.mapOptional("CounterBinding", B.CounterBinding);
}

void MappingTraits<offloadtest::VertexAttribute>::mapping(
    IO &I, offloadtest::VertexAttribute &A) {
  I.mapRequired("Format", A.Format);
  I.mapRequired("Channels", A.Channels);
  I.mapRequired("Offset", A.Offset);
  I.mapRequired("Name", A.Name);
}

void MappingTraits<offloadtest::IOBindings>::mapping(
    IO &I, offloadtest::IOBindings &B) {
  I.mapOptional("VertexBuffer", B.VertexBuffer);
  I.mapOptional("VertexAttributes", B.VertexAttributes);
  I.mapOptional("RenderTarget", B.RenderTarget);
  I.mapOptional("Topology", B.Topology,
                offloadtest::PrimitiveTopology::TriangleList);
  I.mapOptional("PatchControlPoints", B.PatchControlPoints);
}

void MappingTraits<offloadtest::PushConstantBlock>::mapping(
    IO &I, offloadtest::PushConstantBlock &B) {
  I.mapRequired("Stage", B.Stage);
  I.mapRequired("Values", B.Values);
}

template <typename T>
static void setData(IO &I, offloadtest::PushConstantValue &B) {
  llvm::SmallVector<T, 4> Bytes;
  I.mapRequired("Data", Bytes);
  const size_t Size = Bytes.size() * sizeof(T);
  B.Data.resize(Size);
  memcpy(B.Data.data(), Bytes.data(), Size);
}

void MappingTraits<offloadtest::PushConstantValue>::mapping(
    IO &I, offloadtest::PushConstantValue &B) {
  I.mapRequired("Format", B.Format);
  I.mapRequired("Offset", B.OffsetInBytes);

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
  case DF::Depth32:
    return setData<float>(I, B);
  case DF::Float64:
    return setData<double>(I, B);
  case DF::Bool:
    return setData<uint32_t>(I, B); // Because sizeof(bool) is 1 but HLSL
                                    // represents a bool using 4 bytes.
  }
}

void MappingTraits<offloadtest::DispatchParametersSet>::mapping(
    IO &I, offloadtest::DispatchParametersSet &Set) {
  I.mapOptional("DispatchGroupCount", Set.DispatchGroupCount);
  I.mapOptional("VertexCount", Set.VertexCount);
}

void MappingTraits<offloadtest::OutputProperties>::mapping(
    IO &I, offloadtest::OutputProperties &P) {
  I.mapRequired("Height", P.Height);
  I.mapRequired("Width", P.Width);
  I.mapRequired("Depth", P.Depth);
  I.mapOptional("MipLevels", P.MipLevels, 1);
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
  I.mapOptional("SpecializationConstants", S.SpecializationConstants);
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

void MappingTraits<offloadtest::SpecializationConstant>::mapping(
    IO &I, offloadtest::SpecializationConstant &C) {
  I.mapRequired("ConstantID", C.ConstantID);
  I.mapRequired("Type", C.Type);
  I.mapRequired("Value", C.Value);
}

void MappingTraits<offloadtest::TriangleGeometry>::mapping(
    IO &I, offloadtest::TriangleGeometry &G) {
  I.mapRequired("VertexBuffer", G.VertexBuffer);
  I.mapOptional("VertexFormat", G.VertexFormat, Format::RGB32Float);
  I.mapOptional("VertexStride", G.VertexStride, 12u);
  I.mapRequired("VertexCount", G.VertexCount);
  I.mapOptional("IndexBuffer", G.IndexBuffer, std::string());
  I.mapOptional("IndexFormat", G.IdxFormat, IndexFormat::Uint32);
  I.mapOptional("IndexCount", G.IndexCount, 0u);
  I.mapOptional("Opaque", G.Opaque, true);
}

void MappingTraits<offloadtest::AABBGeometry>::mapping(
    IO &I, offloadtest::AABBGeometry &G) {
  I.mapRequired("AABBBuffer", G.AABBBuffer);
  I.mapRequired("AABBCount", G.AABBCount);
  I.mapOptional("AABBStride", G.AABBStride, 24u);
  I.mapOptional("Opaque", G.Opaque, true);
}

void MappingTraits<offloadtest::BLASDesc>::mapping(IO &I,
                                                   offloadtest::BLASDesc &D) {
  I.mapRequired("Name", D.Name);
  I.mapOptional("Triangles", D.Triangles);
  I.mapOptional("AABBs", D.AABBs);
}

void MappingTraits<offloadtest::InstanceDesc>::mapping(
    IO &I, offloadtest::InstanceDesc &D) {
  I.mapRequired("BLAS", D.BLAS);
  llvm::SmallVector<float> Transform(std::begin(D.Transform),
                                     std::end(D.Transform));
  I.mapOptional("Transform", Transform);
  if (Transform.size() != std::size(D.Transform)) {
    I.setError(llvm::Twine("InstanceDesc.Transform must have exactly ") +
               llvm::Twine(std::size(D.Transform)) +
               " floats (3x4 row-major), got " + llvm::Twine(Transform.size()));
    return;
  }
  std::copy(Transform.begin(), Transform.end(), std::begin(D.Transform));
  I.mapOptional("InstanceID", D.InstanceID, 0u);
  uint32_t Mask = D.InstanceMask;
  I.mapOptional("InstanceMask", Mask, 255u);
  D.InstanceMask = static_cast<uint8_t>(Mask);
  I.mapOptional("InstanceContributionToHitGroupIndex",
                D.InstanceContributionToHitGroupIndex, 0u);
  I.mapOptional("InstanceFlags", D.Flags, offloadtest::InstanceFlagNone);
}

void MappingTraits<offloadtest::TLASDesc>::mapping(IO &I,
                                                   offloadtest::TLASDesc &D) {
  I.mapRequired("Name", D.Name);
  I.mapRequired("Instances", D.Instances);
}

void MappingTraits<offloadtest::AccelerationStructureDescs>::mapping(
    IO &I, offloadtest::AccelerationStructureDescs &D) {
  I.mapOptional("BLAS", D.BLAS);
  I.mapOptional("TLAS", D.TLAS);
}

void MappingTraits<offloadtest::HitGroup>::mapping(IO &I,
                                                   offloadtest::HitGroup &G) {
  I.mapRequired("Name", G.Name);
  I.mapOptional("Type", G.Type, offloadtest::HitGroupType::Triangles);
  I.mapRequired("ClosestHit", G.ClosestHit);
  I.mapOptional("AnyHit", G.AnyHit);
  I.mapOptional("Intersection", G.Intersection);
}

void MappingTraits<offloadtest::RayTracingPipelineConfig>::mapping(
    IO &I, offloadtest::RayTracingPipelineConfig &C) {
  I.mapOptional("MaxTraceRecursionDepth", C.MaxTraceRecursionDepth, 1u);
  I.mapOptional("MaxPayloadSizeInBytes", C.MaxPayloadSizeInBytes, 0u);
  I.mapOptional("MaxAttributeSizeInBytes", C.MaxAttributeSizeInBytes, 8u);
  I.mapOptional("PipelineFlags", C.PipelineFlags);
}

void MappingTraits<offloadtest::SBTEntry>::mapping(IO &I,
                                                   offloadtest::SBTEntry &E) {
  I.mapRequired("ShaderName", E.ShaderName);
  llvm::SmallVector<llvm::yaml::Hex8> Bytes;
  if (I.outputting())
    for (const uint8_t B : E.LocalRootData)
      Bytes.push_back(static_cast<llvm::yaml::Hex8>(B));
  I.mapOptional("LocalRootData", Bytes);
  if (!I.outputting()) {
    E.LocalRootData.clear();
    E.LocalRootData.reserve(Bytes.size());
    for (auto B : Bytes)
      E.LocalRootData.push_back(static_cast<uint8_t>(B));
  }
}

void MappingTraits<offloadtest::ShaderBindingTableDesc>::mapping(
    IO &I, offloadtest::ShaderBindingTableDesc &S) {
  I.mapRequired("RayGen", S.RayGen);
  I.mapOptional("Miss", S.Miss);
  I.mapOptional("HitGroup", S.HitGroup);
  I.mapOptional("Callable", S.Callable);
}

} // namespace yaml
} // namespace llvm

llvm::Error offloadtest::Pipeline::validatePipelineKind() {
  bool HasShaderType[NumStages] = {};
  bool HasAnyRayTracingStage = false;
  for (const auto &Shader : Shaders) {
    const auto Idx = llvm::to_underlying(Shader.Stage);
    // RayTracing pipelines may host multiple shaders of the same stage (e.g.
    // several miss shaders, multiple hit groups). Every other pipeline kind
    // forbids duplicates.
    if (HasShaderType[Idx] && !isRayTracingStage(Shader.Stage))
      return llvm::createStringError(
          "Pipeline has multiple shaders of the same type.");
    HasShaderType[Idx] = true;
    if (isRayTracingStage(Shader.Stage))
      HasAnyRayTracingStage = true;
  }

  const bool HasComputeStage =
      HasShaderType[llvm::to_underlying(Stages::Compute)];
  const bool HasVertexStage =
      HasShaderType[llvm::to_underlying(Stages::Vertex)];
  const bool HasMeshStage = HasShaderType[llvm::to_underlying(Stages::Mesh)];
  const bool HasAmplificationStage =
      HasShaderType[llvm::to_underlying(Stages::Amplification)];

  if (HasAnyRayTracingStage) {
    if (HasComputeStage || HasVertexStage || HasMeshStage ||
        HasAmplificationStage)
      return llvm::createStringError(
          "RayTracing shaders cannot be combined with Compute, Vertex, "
          "Amplification, or Mesh shaders.");
    if (!HasShaderType[llvm::to_underlying(Stages::RayGeneration)])
      return llvm::createStringError(
          "RayTracing pipeline requires at least one RayGeneration shader.");
    Kind = ShaderPipelineKind::RayTracing;
    return llvm::Error::success();
  }

  if (!HitGroups.empty() || RTConfig || SBT)
    return llvm::createStringError(
        "HitGroups / RayTracingPipelineConfig / ShaderBindingTable are only "
        "valid on a RayTracing pipeline.");

  if (HasComputeStage) {
    if (Shaders.size() > 1)
      return llvm::createStringError(
          "Compute Pipeline is only allowed to have Compute Shader.");
    Kind = ShaderPipelineKind::Compute;
    return llvm::Error::success();
  }

  if (HasVertexStage) {
    if (HasAmplificationStage || HasMeshStage)
      return llvm::createStringError("Vertex and Mesh/Amplification Shaders "
                                     "cannot be used in the same pipeline.");

    const bool HasHS = HasShaderType[llvm::to_underlying(Stages::Hull)];
    const bool HasDS = HasShaderType[llvm::to_underlying(Stages::Domain)];
    if (HasHS != HasDS)
      return llvm::createStringError(
          "Hull and Domain shaders must be used together");

    const bool IsTessellated = HasHS && HasDS;
    const bool IsPatchList = Bindings.Topology == PrimitiveTopology::PatchList;
    if (IsTessellated != IsPatchList)
      return llvm::createStringError(
          "Tessellation pipelines must use PatchList topology");
    if (IsPatchList &&
        (!Bindings.PatchControlPoints || *Bindings.PatchControlPoints < 1 ||
         *Bindings.PatchControlPoints > 32))
      return llvm::createStringError(
          "PatchList topology requires PatchControlPoints in the range 1..32.");
    if (!IsPatchList && Bindings.PatchControlPoints)
      return llvm::createStringError(
          "PatchControlPoints is only valid with PatchList topology.");

    Kind = ShaderPipelineKind::TraditionalRaster;
    return llvm::Error::success();
  }

  if (HasMeshStage) {
    Kind = ShaderPipelineKind::MeshShaderRaster;
    return llvm::Error::success();
  }

  // As we add more pipeline types this error message should be updated with
  // more required shader types.
  return llvm::createStringError(
      "The pipeline misses a Compute, Vertex, Mesh, or RayGeneration Shader.");
}

llvm::Error offloadtest::Pipeline::validateDispatchParameters() {
  switch (Kind) {
  case ShaderPipelineKind::Compute:
  case ShaderPipelineKind::MeshShaderRaster:
    if (DispatchParameters.VertexCount)
      return llvm::createStringError(
          "DispatchParameters.VertexCount set on a Compute or Mesh Shader "
          "pipeline. Only allowed on a TraditionalRaster pipeline.");
    break;
  case ShaderPipelineKind::TraditionalRaster:
    if (DispatchParameters.DispatchGroupCount !=
        std::array<uint32_t, 3>{1, 1, 1})
      return llvm::createStringError(
          "DispatchParameters.DispatchGroupCount set on a TraditionalRaster "
          "pipeline. Only allowed on a Compute pipeline.");
    break;
  case ShaderPipelineKind::RayTracing:
    // DispatchGroupCount is reinterpreted as { Width, Height, Depth } for
    // DispatchRays. VertexCount is not meaningful for an RT dispatch.
    if (DispatchParameters.VertexCount)
      return llvm::createStringError(
          "DispatchParameters.VertexCount set on a RayTracing pipeline. Only "
          "allowed on a TraditionalRaster pipeline.");
    break;
  }

  return llvm::Error::success();
}
