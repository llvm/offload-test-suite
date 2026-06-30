#include "OffloadMigration.h"

#include "API/Device.h"
#include "API/FormatConversion.h"

// Needed for direct Root Signature binding
#include "API/DX/Buffer.h"
#include "API/DX/CommandBuffer.h"
#include "API/DX/Descriptors.h"
#include "API/DX/PipelineState.h"

namespace offloadtest {

static BufferUsage bufferUsageFromResourceKind(ResourceKind Kind) {
  // Determine Buffer Usage
  switch (Kind) {
  case ResourceKind::Buffer:
  case ResourceKind::StructuredBuffer:
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::RWByteAddressBuffer:
    return BufferUsage::Storage;
  case ResourceKind::ConstantBuffer:
    return BufferUsage::ConstantBuffer;
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
  case ResourceKind::Sampler:
  case ResourceKind::SampledTexture2D:
  case ResourceKind::AccelerationStructure:
    llvm_unreachable("Invalid case, ResourceKind is not a buffer.");
  }
  llvm_unreachable("All ResourceKind cases handled");
}

static BufferShaderAccessType bufferShaderAccessTypeFromResourceKind(
    const Resource &Resource, BufferShaderAccessTypeParams &OutParams) {
  // Determine Buffer Access Type
  switch (Resource.Kind) {
  case ResourceKind::Buffer:
  case ResourceKind::RWBuffer: {
    auto FmtOrErr =
        toFormat(Resource.BufferPtr->Format, Resource.BufferPtr->Channels);
    if (!FmtOrErr) {
      printf("Invalid format! FMT: %d, CHANNELS: %d\n",
             Resource.BufferPtr->Format, Resource.BufferPtr->Channels);
      assert(false && "Invalid format.");
    }
    OutParams.Fmt = *FmtOrErr;
    return BufferShaderAccessType::Typed;
  }
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
    OutParams.StructureStride = Resource.BufferPtr->getElementSize();
    return BufferShaderAccessType::Structured;
  case ResourceKind::ByteAddressBuffer:
  case ResourceKind::RWByteAddressBuffer:
  case ResourceKind::ConstantBuffer:
    return BufferShaderAccessType::Raw;
  case ResourceKind::Texture2D:
  case ResourceKind::RWTexture2D:
  case ResourceKind::Sampler:
  case ResourceKind::SampledTexture2D:
  case ResourceKind::AccelerationStructure:
    llvm_unreachable(
        "Invalid case, non-buffers should have been filtered out.");
  }
  llvm_unreachable("All ResourceKind cases handled");
}

static llvm::Expected<std::unique_ptr<AccelerationStructure>>
createAS(Device &Dev, Resource &R) {
  assert(R.TLASPtr && "AS resource must be resolved to a TLAS");
  assert(R.getArraySize() == 1 && "AS arrays not yet supported");
  const uint32_t InstanceCount =
      static_cast<uint32_t>(R.TLASPtr->Instances.size());
  auto SizesOrErr = Dev.getTLASBuildSizes(InstanceCount);
  if (!SizesOrErr)
    return SizesOrErr.takeError();
  return Dev.createTLAS(*SizesOrErr, InstanceCount);
}

llvm::Error copyBackResource(offloadtest::ComputeEncoder &ReadbackEncoder,
                             ResourcePair &R) {
  if (R.first->isTexture()) {
    for (const ResourceSet &RS : R.second) {
      if (RS.Readback == nullptr)
        continue;

      if (auto Err =
              ReadbackEncoder.copyTextureToBuffer(*RS.Texture, *RS.Readback))
        return Err;
    }
  } else if (R.first->isBuffer()) {
    for (const ResourceSet &RS : R.second) {
      if (RS.Readback == nullptr)
        continue;

      if (auto Err = ReadbackEncoder.copyBufferToBuffer(
              *RS.Buffer, 0, *RS.Readback, 0, RS.Buffer->getSizeInBytes()))
        return Err;

      if (!RS.Buffer->getDesc().HasCounter)
        continue;

      if (auto Err = ReadbackEncoder.copyCounterToBuffer(*RS.Buffer,
                                                         *RS.CounterReadback))
        return Err;
    }
  }

  return llvm::Error::success();
}

llvm::Error readBack(Device &Dev, Pipeline &P, SharedInvocationState &IS) {
  auto MemCpyBack = [&Dev](ResourcePair &R) -> llvm::Error {
    if (!R.first->isReadWrite())
      return llvm::Error::success();

    auto *RSIt = R.second.begin();
    auto *DataIt = R.first->BufferPtr->Data.begin();
    for (; RSIt != R.second.end() && DataIt != R.first->BufferPtr->Data.end();
         ++RSIt, ++DataIt) {
      offloadtest::Buffer &Readback = *RSIt->Readback;
      auto DataPtrOrErr = Readback.map();
      if (!DataPtrOrErr)
        return DataPtrOrErr.takeError();
      const void *DataPtr = *DataPtrOrErr;

      if (R.first->isTexture()) {
        const TextureCreateDesc &Desc = RSIt->Texture->getDesc();
        const uint32_t SrcStrideInBytes =
            Dev.getTextureUploadRowStrideInBytes(Desc);
        const uint32_t DstStrideInBytes =
            Desc.Width * getFormatSizeInBytes(Desc.Fmt);
        assert(DstStrideInBytes <= SrcStrideInBytes &&
               "Destination should not have padding and thus should be <= "
               "than SrcStride where we do expect potential padding.");
        uint8_t *Dst = (uint8_t *)DataIt->get();
        const uint8_t *Src = (const uint8_t *)DataPtr;

        for (uint32_t Y = 0; Y < Desc.Height; ++Y) {
          memcpy(Dst, Src, DstStrideInBytes);
          Dst += DstStrideInBytes;
          Src += SrcStrideInBytes;
        }
      } else {
        memcpy(DataIt->get(), DataPtr, R.first->size());
      }

      Readback.unmap();

      if (R.first->HasCounter) {
        offloadtest::Buffer &CounterReadback = *RSIt->CounterReadback;
        auto CounterPtrOrErr = CounterReadback.map();
        if (!CounterPtrOrErr)
          return CounterPtrOrErr.takeError();
        const uint32_t *CounterPtr = (const uint32_t *)*CounterPtrOrErr;
        R.first->BufferPtr->Counters.push_back(*CounterPtr);
        CounterReadback.unmap();
      }
    }

    return llvm::Error::success();
  };

  for (auto &Table : IS.DescTables)
    for (auto &R : Table.Resources)
      if (auto Err = MemCpyBack(R))
        return Err;

  for (auto &R : IS.RootResources)
    if (auto Err = MemCpyBack(R))
      return Err;

  // If there is no render target, return early.
  if (!IS.RTReadback)
    return llvm::Error::success();

  auto DataPtrOrErr = IS.RTReadback->map();
  if (!DataPtrOrErr)
    return DataPtrOrErr.takeError();
  const void *Mapped = *DataPtrOrErr;

  const uint32_t SrcStrideInBytes =
      Dev.getTextureUploadRowStrideInBytes(IS.RenderTarget->getDesc());

  P.Bindings.RTargetBufferPtr->copyFromTexture(Mapped, SrcStrideInBytes);
  IS.RTReadback->unmap();
  return llvm::Error::success();
}

static llvm::Error createRenderTarget(Device &Dev, Pipeline &P,
                                      SharedInvocationState &IS) {
  if (!P.Bindings.RTargetBufferPtr)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "No render target bound for graphics pipeline.");
  const CPUBuffer &OutBuf = *P.Bindings.RTargetBufferPtr;

  auto TexOrErr = offloadtest::createRenderTargetFromCPUBuffer(Dev, OutBuf);
  if (!TexOrErr)
    return TexOrErr.takeError();

  IS.RenderTarget = std::move(*TexOrErr);

  // Create readback buffer sized for the pixel data with row pitch padded
  // up to D3D12_TEXTURE_DATA_PITCH_ALIGNMENT, which is what D3D12 requires
  // for the placed footprint used by CopyTextureRegion. The compaction
  // back to a tight layout happens in readBack() via GetCopyableFootprints.
  BufferCreateDesc BufDesc = {};
  BufDesc.Location = MemoryLocation::GpuToCpu;
  BufDesc.Usage = BufferUsage::Storage;
  auto BufOrErr = Dev.createBuffer(
      "RTReadback", BufDesc, IS.RenderTarget->calculateLinearSizeInBytes(Dev));
  if (!BufOrErr)
    return BufOrErr.takeError();
  IS.RTReadback = std::move(*BufOrErr);

  return llvm::Error::success();
}

static llvm::Error createDepthStencil(Device &Dev, Pipeline &P,
                                      SharedInvocationState &IS) {
  auto TexOrErr = offloadtest::createDefaultDepthStencilTarget(
      Dev, P.Bindings.RTargetBufferPtr->OutputProps.Width,
      P.Bindings.RTargetBufferPtr->OutputProps.Height);
  if (!TexOrErr)
    return TexOrErr.takeError();
  IS.DepthStencil = std::move(*TexOrErr);
  return llvm::Error::success();
}

llvm::Error createResources(Device &Dev, Pipeline &P,
                            SharedInvocationState &IS) {
  auto EncOrErr = IS.CB->createComputeEncoder();
  if (!EncOrErr)
    return EncOrErr.takeError();
  auto Enc = std::move(*EncOrErr);

  auto CreateBuffer =
      [&Dev, &Enc,
       &IS](Resource &R,
            llvm::SmallVectorImpl<ResourcePair> &Resources) -> llvm::Error {
    ResourceBundle ResBundle;
    if (R.isBuffer()) {
      BufferCreateDesc CreateDesc = {};
      CreateDesc.Location = MemoryLocation::GpuOnly;
      CreateDesc.Backing =
          R.IsReserved ? MemoryBacking::Sparse : MemoryBacking::Automatic;
      CreateDesc.Usage = bufferUsageFromResourceKind(R.Kind);
      CreateDesc.AccessType = bufferShaderAccessTypeFromResourceKind(
          R, CreateDesc.AccessTypeParams);
      CreateDesc.HasCounter = R.HasCounter;

      for (auto &Data : R.BufferPtr->Data) {
        std::unique_ptr<offloadtest::Buffer> UploadBuffer;
        std::unique_ptr<offloadtest::MemoryHeap> BackingMemoryHeap;

        std::unique_ptr<offloadtest::Buffer> Buffer;
        if (R.IsReserved) {
          auto BufferOrErr = createSparseBufferWithData(
              Dev, Dev.getGraphicsQueue(), "Sparse Buffer", CreateDesc,
              R.size(), R.TilesMapped, Data.get(), R.size(), *Enc.get(),
              UploadBuffer, BackingMemoryHeap);
          if (!BufferOrErr)
            return BufferOrErr.takeError();

          Buffer = std::move(*BufferOrErr);
        } else {
          auto BufferOrErr =
              createBufferWithData(Dev, "Buffer", CreateDesc, Data.get(),
                                   R.size(), Enc.get(), &UploadBuffer);
          if (!BufferOrErr)
            return BufferOrErr.takeError();

          Buffer = std::move(*BufferOrErr);
        }

        std::unique_ptr<offloadtest::Buffer> ReadbackBuffer;
        std::unique_ptr<offloadtest::Buffer> CounterReadbackBuffer;
        if (getDescriptorKind(R.Kind) == DescriptorKind::UAV) {
          const BufferCreateDesc ReadbackDesc =
              BufferCreateDesc::readbackBuffer();
          auto ReadbackOrErr = Dev.createBuffer("Readback", ReadbackDesc,
                                                Buffer->getSizeInBytes());
          if (!ReadbackOrErr)
            return ReadbackOrErr.takeError();
          ReadbackBuffer = std::move(*ReadbackOrErr);

          if (R.HasCounter) {
            auto CounterReadbackOrErr =
                Dev.createBuffer("Readback", ReadbackDesc, sizeof(uint32_t));
            if (!CounterReadbackOrErr)
              return CounterReadbackOrErr.takeError();
            CounterReadbackBuffer = std::move(*CounterReadbackOrErr);
          }
        }

        IS.KeepAliveBuffers.push_back(std::move(UploadBuffer));
        ResourceSet RSet(std::move(Buffer), std::move(BackingMemoryHeap),
                         std::move(ReadbackBuffer),
                         std::move(CounterReadbackBuffer));
        ResBundle.push_back(std::move(RSet));
      }
    } else if (R.isTexture()) {
      auto FormatOrErr = toFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();

      LLVM_ENABLE_BITMASK_ENUMS_IN_NAMESPACE();

      TextureCreateDesc CreateDesc = {};
      CreateDesc.Location = MemoryLocation::GpuOnly;
      CreateDesc.Backing =
          R.IsReserved ? MemoryBacking::Sparse : MemoryBacking::Automatic;
      CreateDesc.Usage = TextureUsage::Sampled;
      if (R.Kind == ResourceKind::RWTexture2D)
        CreateDesc.Usage |= TextureUsage::Storage;
      CreateDesc.Fmt = *FormatOrErr;
      CreateDesc.Width = R.BufferPtr->OutputProps.Width;
      CreateDesc.Height = R.BufferPtr->OutputProps.Height;
      CreateDesc.MipLevels = R.BufferPtr->OutputProps.MipLevels;

      for (auto &Data : R.BufferPtr->Data) {
        std::unique_ptr<offloadtest::Buffer> UploadBuffer;
        std::unique_ptr<offloadtest::MemoryHeap> BackingMemoryHeap;

        std::unique_ptr<offloadtest::Texture> Texture;
        if (R.IsReserved) {
          auto TextureOrErr = createSparseTextureWithData(
              Dev, Dev.getGraphicsQueue(), "Sparse Texture", CreateDesc,
              Data.get(), R.size(), *Enc.get(), UploadBuffer,
              BackingMemoryHeap);
          if (!TextureOrErr)
            return TextureOrErr.takeError();

          Texture = std::move(*TextureOrErr);
        } else {
          auto TextureOrErr =
              createTextureWithData(Dev, R.Name, CreateDesc, Data.get(),
                                    R.size(), Enc.get(), &UploadBuffer);
          if (!TextureOrErr)
            return TextureOrErr.takeError();

          Texture = std::move(*TextureOrErr);
        }

        std::unique_ptr<Buffer> ReadbackBuffer;
        if (getDescriptorKind(R.Kind) == DescriptorKind::UAV) {
          const BufferCreateDesc ReadbackDesc =
              BufferCreateDesc::readbackBuffer();
          auto ReadbackOrErr =
              Dev.createBuffer("Readback", ReadbackDesc,
                               Texture->calculateLinearSizeInBytes(Dev));
          if (!ReadbackOrErr)
            return ReadbackOrErr.takeError();
          ReadbackBuffer = std::move(*ReadbackOrErr);
        }

        std::unique_ptr<Sampler> Sampler;
        if (R.Kind == ResourceKind::SampledTexture2D) {
          SamplerCreateDesc Desc = {
              R.SamplerPtr->MinFilter,    R.SamplerPtr->MagFilter,
              R.SamplerPtr->Address,      R.SamplerPtr->MinLOD,
              R.SamplerPtr->MaxLOD,       R.SamplerPtr->MipLODBias,
              R.SamplerPtr->ComparisonOp, R.SamplerPtr->Kind,
          };

          auto SamplerOrErr = Dev.createSampler(R.SamplerPtr->Name, Desc);
          if (!SamplerOrErr)
            return SamplerOrErr.takeError();
          Sampler = std::move(*SamplerOrErr);
        }

        IS.KeepAliveBuffers.push_back(std::move(UploadBuffer));
        ResourceSet RSet(std::move(Texture), std::move(Sampler),
                         std::move(BackingMemoryHeap),
                         std::move(ReadbackBuffer));
        ResBundle.push_back(std::move(RSet));
      }
    } else if (R.isAccelerationStructure()) {
      auto ASOrErr = createAS(Dev, R);
      if (!ASOrErr)
        return ASOrErr.takeError();
      ResBundle.emplace_back(ASOrErr->get());
      auto Inserted =
          IS.TLASes.try_emplace(R.TLASPtr->Name, std::move(*ASOrErr));
      assert(Inserted.second && "TLAS bound to multiple resources NYI");
      (void)Inserted;
    } else if (R.isSampler()) {
      SamplerCreateDesc Desc = {
          R.SamplerPtr->MinFilter,    R.SamplerPtr->MagFilter,
          R.SamplerPtr->Address,      R.SamplerPtr->MinLOD,
          R.SamplerPtr->MaxLOD,       R.SamplerPtr->MipLODBias,
          R.SamplerPtr->ComparisonOp, R.SamplerPtr->Kind,
      };

      auto SamplerOrErr = Dev.createSampler(R.SamplerPtr->Name, Desc);
      if (!SamplerOrErr)
        return SamplerOrErr.takeError();

      ResourceSet RSet(std::move(*SamplerOrErr));
      ResBundle.push_back(std::move(RSet));
    } else {
      return llvm::createStringError(std::errc::not_supported,
                                     "Unrecognized resource type.");
    }

    Resources.push_back(std::make_pair(&R, std::move(ResBundle)));
    return llvm::Error::success();
  };

  if (P.isRaster()) {
    // Create render target and depth/stencil
    if (auto Err = createRenderTarget(Dev, P, IS))
      return Err;
    llvm::outs() << "Render target created.\n";
    // TODO: Always created for graphics pipelines. Consider making this
    // conditional on the pipeline definition.
    if (auto Err = createDepthStencil(Dev, P, IS))
      return Err;
    llvm::outs() << "Depth stencil created.\n";
  }

  for (auto &D : P.Sets) {
    IS.DescTables.emplace_back(DescriptorTable());
    DescriptorTable &Table = IS.DescTables.back();
    for (auto &R : D.Resources)
      if (auto Err = CreateBuffer(R, Table.Resources))
        return Err;
  }

  Enc->endEncoding();

  // Setup root descriptors
  for (auto &R : P.Settings.DX.RootParams) {
    if (R.Kind != dx::RootParamKind::RootDescriptor)
      continue;
    auto &Resource = std::get<dx::RootResource>(R.Data);
    if (!Resource.IsReserved && Resource.TilesMapped.has_value()) {
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Error: Cannot define tiles mapped without declaring resource as "
          "reserved.");
    }
    if (auto Err = CreateBuffer(Resource, IS.RootResources))
      return Err;
  }

  if (P.isTraditionalRaster() && P.Bindings.VertexBufferPtr) {
    const CPUBuffer *VBuffer = P.Bindings.VertexBufferPtr;

    BufferCreateDesc BufDesc = {};
    BufDesc.Location = MemoryLocation::CpuToGpu;
    BufDesc.Usage = BufferUsage::VertexBuffer;
    auto BufOrErr = createBufferWithData(Dev, "VertexBuffer", BufDesc,
                                         VBuffer->Data[0].get(),
                                         VBuffer->size(), nullptr, nullptr);
    if (!BufOrErr)
      return BufOrErr.takeError();
    IS.VB = std::move(*BufOrErr);
    llvm::outs() << "Vertex buffer created.\n";
  }

  return llvm::Error::success();
}

llvm::Expected<std::unique_ptr<DescriptorSets>>
buildDescriptorSets(Device &Dev, DescriptorPool &Pool, PipelineState &Pipeline,
                    llvm::ArrayRef<DescriptorTable> DescTables) {
  auto BuilderOrErr = Dev.createDescriptorSetsBuilder(Pool, Pipeline);
  if (!BuilderOrErr)
    return BuilderOrErr.takeError();
  auto Builder = std::move(*BuilderOrErr);

  uint32_t SetIndex = 0;
  for (auto &T : DescTables) {
    for (auto &R : T.Resources) {
      VKBind BindVK = {};
      if (R.first->VKBinding.has_value()) {
        const VulkanBinding &VKBinding = *R.first->VKBinding;
        BindVK.Binding = VKBinding.Binding;
        if (VKBinding.CounterBinding)
          BindVK.CounterBinding = *VKBinding.CounterBinding;
      }

      if (R.first->isAccelerationStructure()) {
        assert(R.second[0].AS != nullptr &&
               "Acceleration structure was nullptr ?!");
        Builder->read(SetIndex, R.second[0].AS, BindVK);
      } else if (R.first->isBuffer()) {
        llvm::SmallVector<const Buffer *> Buffers;
        for (const auto &Set : R.second)
          Buffers.push_back(Set.Buffer.get());

        if (R.first->Kind == ResourceKind::ConstantBuffer)
          Builder->constant(SetIndex, Buffers, BindVK);
        else if (R.first->isReadWrite())
          Builder->write(SetIndex, Buffers, BindVK);
        else
          Builder->read(SetIndex, Buffers, BindVK);
      } else if (R.first->isTexture()) {
        llvm::SmallVector<const Texture *> Textures;
        llvm::SmallVector<const Sampler *> Samplers;
        for (const auto &Set : R.second) {
          if (Set.Sampler.get() != nullptr)
            Samplers.push_back(Set.Sampler.get());
          Textures.push_back(Set.Texture.get());
        }

        if (R.first->isReadWrite()) {
          assert(Samplers.empty() &&
                 "Combined image samplers cannot be bound as write");
          Builder->write(SetIndex, Textures, BindVK);
        } else {
          Builder->read(SetIndex, Textures, Samplers, BindVK);
        }
      } else if (R.first->isSampler()) {
        llvm::SmallVector<const Sampler *> Samplers;
        for (const auto &Set : R.second)
          Samplers.push_back(Set.Sampler.get());

        Builder->sampler(SetIndex, Samplers, BindVK);
      }
    }
    SetIndex += 1;
  }

  return Builder->build();
}

static llvm::Error bindDXComputeRootSignature(Pipeline &P,
                                              SharedInvocationState &IS,
                                              DescriptorPool &Pool) {
  const DXCommandBuffer &DXCB = llvm::cast<DXCommandBuffer>(*IS.CB);
  const DXDescriptorPool &PoolDX = llvm::cast<DXDescriptorPool>(Pool);
  const DXPipelineState &DXPipeline =
      llvm::cast<DXPipelineState>(*IS.Pipeline.get());

  D3D12_GPU_DESCRIPTOR_HANDLE Handle = PoolDX.CSUHandleGPU;
  const uint32_t Inc = PoolDX.CSUIncSize;
  DXCB.CmdList->SetComputeRootSignature(DXPipeline.RootSig.Get());

  uint32_t ConstantOffset = 0u;
  uint32_t RootParamIndex = 0u;
  uint32_t DescriptorTableIndex = 0u;
  auto *RootDescIt = IS.RootResources.begin();
  for (const auto &Param : P.Settings.DX.RootParams) {
    switch (Param.Kind) {
    case dx::RootParamKind::Constant: {
      auto &Constant = std::get<dx::RootConstant>(Param.Data);
      if (Constant.BufferPtr->ArraySize != 1)
        return llvm::createStringError(
            std::errc::value_too_large,
            "Root constant cannot refer to resource arrays.");
      const uint32_t NumValues = Constant.BufferPtr->size() / sizeof(uint32_t);
      DXCB.CmdList->SetComputeRoot32BitConstants(
          RootParamIndex++, NumValues, Constant.BufferPtr->Data.back().get(),
          ConstantOffset);
      ConstantOffset += NumValues;
      break;
    }
    case dx::RootParamKind::DescriptorTable:
      // TODO(manon): Add support for descriptor tables containing samplers
      DXCB.CmdList->SetComputeRootDescriptorTable(RootParamIndex++, Handle);
      Handle.ptr += P.Sets[DescriptorTableIndex++].Resources.size() * Inc;
      break;
    case dx::RootParamKind::RootDescriptor:
      assert(RootDescIt != IS.RootResources.end());
      if (RootDescIt->first->getArraySize() != 1)
        return llvm::createStringError(
            std::errc::value_too_large,
            "Root descriptor cannot refer to resource arrays.");

      const DXBuffer *BufferDX = llvm::cast_if_present<DXBuffer>(
          RootDescIt->second.back().Buffer.get());
      if (!BufferDX) {
        return llvm::createStringError(
            std::errc::value_too_large,
            "Root descriptor can only refer to buffers.");
      }

      const D3D12_GPU_VIRTUAL_ADDRESS VirtualAddress =
          BufferDX->Buffer->GetGPUVirtualAddress();
      switch (getDescriptorKind(RootDescIt->first->Kind)) {
      case DescriptorKind::SRV:
        DXCB.CmdList->SetComputeRootShaderResourceView(RootParamIndex++,
                                                       VirtualAddress);
        break;
      case DescriptorKind::UAV:
        DXCB.CmdList->SetComputeRootUnorderedAccessView(RootParamIndex++,
                                                        VirtualAddress);
        break;
      case DescriptorKind::CBV:
        DXCB.CmdList->SetComputeRootConstantBufferView(RootParamIndex++,
                                                       VirtualAddress);
        break;
      case DescriptorKind::SAMPLER:
        llvm_unreachable(
            "Samplers cannot be written directly into the Root Signature.");
      }
      ++RootDescIt;
      break;
    }
  }

  return llvm::Error::success();
}

static llvm::Error createComputeCommands(Pipeline &P, SharedInvocationState &IS,
                                         Device &Dev, DescriptorPool &Pool) {
  IS.CB->bindPool(Pool);

  auto DescSetsOrErr =
      buildDescriptorSets(Dev, Pool, *IS.Pipeline, IS.DescTables);
  if (!DescSetsOrErr)
    return DescSetsOrErr.takeError();
  auto DescSets = std::move(*DescSetsOrErr);

  if (P.Settings.DX.RootParams.size() > 0) {
    if (auto Err = bindDXComputeRootSignature(P, IS, Pool))
      return Err;
  }

  auto EncoderOrErr = IS.CB->createComputeEncoder();
  if (!EncoderOrErr)
    return EncoderOrErr.takeError();
  auto &Encoder = *EncoderOrErr.get();

  if (P.isRayTracing()) {
    if (P.Settings.DX.RootParams.empty())
      Encoder.bindRayTracingDescriptorSets(*IS.Pipeline, *DescSets);

    if (auto Err = Encoder.dispatchRays(
            *IS.Pipeline, *IS.SBT, P.DispatchParameters.DispatchGroupCount[0],
            P.DispatchParameters.DispatchGroupCount[1],
            P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
  } else {
    if (P.Settings.DX.RootParams.empty())
      Encoder.bindComputeDescriptorSets(*IS.Pipeline, *DescSets);

    if (auto Err = Encoder.dispatch(*IS.Pipeline.get(),
                                    P.DispatchParameters.DispatchGroupCount[0],
                                    P.DispatchParameters.DispatchGroupCount[1],
                                    P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
  }
  Encoder.endEncoding();

  return llvm::Error::success();
}

static llvm::Error createGraphicsCommands(Pipeline &P,
                                          SharedInvocationState &IS,
                                          Device &Dev, DescriptorPool &Pool) {
  IS.CB->bindPool(Pool);
  auto DescSetsOrErr =
      buildDescriptorSets(Dev, Pool, *IS.Pipeline, IS.DescTables);
  if (!DescSetsOrErr)
    return DescSetsOrErr.takeError();
  auto DescSets = std::move(*DescSetsOrErr);

  RenderPassBeginDesc BeginDesc = {};
  BeginDesc.Pass = IS.RenderPass.get();
  BeginDesc.ColorAttachments.push_back(IS.RenderTarget.get());
  BeginDesc.DepthStencil = IS.DepthStencil.get();

  auto EncOrErr = IS.CB->createRenderEncoder(BeginDesc);
  if (!EncOrErr)
    return EncOrErr.takeError();
  auto &Encoder = *EncOrErr.get();

  Encoder.bindDescriptorSets(*IS.Pipeline, *DescSets);

  Viewport VP;
  VP.Width = static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Width);
  VP.Height =
      static_cast<float>(P.Bindings.RTargetBufferPtr->OutputProps.Height);
  Encoder.setViewport(VP);

  ScissorRect Scissor;
  Scissor.Width = static_cast<uint32_t>(VP.Width);
  Scissor.Height = static_cast<uint32_t>(VP.Height);
  Encoder.setScissor(Scissor);

  if (P.isTraditionalRaster()) {
    if (IS.VB)
      Encoder.setVertexBuffer(0, IS.VB.get(), 0, P.Bindings.getVertexStride());

    if (auto Err = Encoder.drawInstanced(*IS.Pipeline.get(), P.getVertexCount(),
                                         /*InstanceCount=*/1))
      return Err;
  } else {
    if (auto Err = Encoder.dispatchMesh(
            *IS.Pipeline.get(), P.DispatchParameters.DispatchGroupCount[0],
            P.DispatchParameters.DispatchGroupCount[1],
            P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
  }

  Encoder.endEncoding();

  return llvm::Error::success();
}

llvm::Error executeUnitTest(Device &Dev, Pipeline &P) {
  llvm::outs() << "Configuring execution on device: " << Dev.Description
               << "\n";

  auto DescriptorPoolOrErr = Dev.createDescriptorPool();
  if (!DescriptorPoolOrErr)
    return DescriptorPoolOrErr.takeError();
  auto DescriptorPool = std::move(*DescriptorPoolOrErr);

  SharedInvocationState State;
  auto CBOrErr = Dev.createCommandBuffer();
  if (!CBOrErr)
    return CBOrErr.takeError();
  State.CB = std::move(*CBOrErr);
  llvm::outs() << "Command buffer created.\n";

  if (auto Err = createResources(Dev, P, State))
    return Err;
  llvm::outs() << "Buffers created.\n";

  if (!P.AccelStructs.BLAS.empty() || !P.AccelStructs.TLAS.empty()) {
    auto EncOrErr = State.CB->createComputeEncoder();
    if (!EncOrErr)
      return EncOrErr.takeError();
    if (auto Err = offloadtest::buildPipelineAccelerationStructures(
            Dev, **EncOrErr, P, State.BLASes, State.TLASes,
            State.ASInputBuffers))
      return Err;
    (*EncOrErr)->endEncoding();
  }

  BindingsDesc BndDesc = {};
  for (auto &S : P.Sets) {
    DescriptorSetLayoutDesc Layout;
    for (auto &R : S.Resources) {
      ResourceBindingDesc ResourceBinding = {};
      ResourceBinding.Kind = R.Kind;
      ResourceBinding.DXBinding.Register = R.DXBinding.Register;
      ResourceBinding.DXBinding.Space = R.DXBinding.Space;
      ResourceBinding.VKBinding = R.VKBinding;
      ResourceBinding.DescriptorCount = R.getArraySize();

      Layout.ResourceBindings.push_back(ResourceBinding);
    }

    BndDesc.DescriptorSetDescs.push_back(Layout);
  }

  if (P.isCompute()) {
    // This is an arbitrary distinction that we could alter in the future.
    if (P.Shaders.size() != 1 || P.Shaders[0].Stage != Stages::Compute)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Compute pipeline must have exactly one compute shader.");

    ShaderContainer CS = {};
    CS.EntryPoint = P.Shaders[0].Entry;
    CS.Shader = P.Shaders[0].Shader.get();

    auto PipelineStateOrErr =
        Dev.createPipelineCs("Compute Pipeline State", BndDesc, CS);
    if (!PipelineStateOrErr)
      return PipelineStateOrErr.takeError();
    State.Pipeline = std::move(*PipelineStateOrErr);
    llvm::outs() << "Compute Pipeline created.\n";
    if (auto Err = createComputeCommands(P, State, Dev, *DescriptorPool))
      return Err;
    llvm::outs() << "Compute command list created.\n";

  } else if (P.isRaster()) {

    // Begin a render pass: bind RT/DSV and clear depth-stencil. Color
    // load action is Load — the existing inline code didn't clear color.
    ColorAttachmentFormatDesc ColorAttachment = {};
    ColorAttachment.Fmt = State.RenderTarget->getDesc().Fmt;
    ColorAttachment.Load = LoadAction::Load;
    ColorAttachment.Store = StoreAction::Store;

    DepthStencilAttachmentFormatDesc DSAttachment = {};
    DSAttachment.Fmt = State.DepthStencil->getDesc().Fmt;
    DSAttachment.DepthLoad = LoadAction::Clear;
    DSAttachment.DepthStore = StoreAction::Store;
    DSAttachment.StencilLoad = LoadAction::DontCare;
    DSAttachment.StencilStore = StoreAction::DontCare;

    RenderPassDesc PassDesc;
    PassDesc.ColorAttachments.push_back(ColorAttachment);
    PassDesc.DepthStencil = DSAttachment;

    auto RenderPassOrErr = Dev.createRenderPass(PassDesc);
    if (!RenderPassOrErr)
      return RenderPassOrErr.takeError();
    State.RenderPass = std::move(*RenderPassOrErr);
    llvm::outs() << "Render pass created.\n";

    if (P.isTraditionalRaster()) {
      ShaderContainer VS = {};
      ShaderContainer PS = {};
      for (auto &Shader : P.Shaders) {
        if (Shader.Stage == Stages::Vertex) {
          VS.EntryPoint = Shader.Entry;
          VS.Shader = Shader.Shader.get();
        } else if (Shader.Stage == Stages::Pixel) {
          PS.EntryPoint = Shader.Entry;
          PS.Shader = Shader.Shader.get();
        }
      }

      TraditionalRasterPipelineCreateDesc PipelineDesc = {};
      PipelineDesc.Topology = P.Bindings.Topology;
      PipelineDesc.PatchControlPoints = P.Bindings.PatchControlPoints;
      PipelineDesc.DSFormat = Format::D32FloatS8Uint;
      for (auto &Shader : P.Shaders) {
        ShaderContainer SC = {};
        SC.EntryPoint = Shader.Entry;
        SC.Shader = Shader.Shader.get();
        PipelineDesc.setShader(Shader.Stage, std::move(SC));
      }

      // Create the input layout based on the vertex attributes.
      for (auto &Attr : P.Bindings.VertexAttributes) {
        auto FormatOrErr = toFormat(Attr.Format, Attr.Channels);
        if (!FormatOrErr)
          return FormatOrErr.takeError();

        InputLayoutDesc Layout = {};
        Layout.Name = Attr.Name;
        Layout.Fmt = *FormatOrErr;
        Layout.OffsetInBytes = Attr.Offset;
        PipelineDesc.InputLayout.push_back(Layout);
      }

      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();
      PipelineDesc.RTFormats.push_back(*FormatOrErr);

      auto PipelineStateOrErr = Dev.createTraditionalRasterPipeline(
          "Graphics Pipeline State", BndDesc, PipelineDesc);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Traditional Raster Pipeline created.\n";

    } else if (P.isMeshShaderRaster()) {
      MeshShaderRasterPipelineCreateDesc PipelineDesc = {};
      PipelineDesc.Topology = P.Bindings.Topology;
      PipelineDesc.DSFormat = Format::D32FloatS8Uint;
      for (auto &Shader : P.Shaders) {
        ShaderContainer SC = {};
        SC.EntryPoint = Shader.Entry;
        SC.Shader = Shader.Shader.get();
        PipelineDesc.setShader(Shader.Stage, std::move(SC));
      }

      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();
      PipelineDesc.RTFormats.push_back(*FormatOrErr);

      auto PipelineStateOrErr = Dev.createMeshShaderRasterPipeline(
          "Mesh Shader Pipeline State", BndDesc, PipelineDesc);

      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      State.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Mesh Shader Pipeline created.\n";
    }

    if (auto Err = createGraphicsCommands(P, State, Dev, *DescriptorPool))
      return Err;
    llvm::outs() << "Graphics command list created complete.\n";
  } else if (P.isRayTracing()) {
    if (P.Shaders.empty() || !P.SBT || !P.RTConfig)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RayTracing pipeline requires Shaders, "
          "ShaderBindingTable, and RayTracingPipelineConfig.");

    RayTracingPipelineCreateDesc RTDesc{};
    RTDesc.Library = P.Shaders.front().Shader.get();
    RTDesc.HitGroups = P.HitGroups;
    RTDesc.Config = *P.RTConfig;
    RTDesc.Shaders.reserve(P.Shaders.size());
    for (const auto &Sh : P.Shaders)
      RTDesc.Shaders.push_back({Sh.Stage, Sh.Entry});

    auto PSOOrErr =
        Dev.createPipelineRT("RayTracing Pipeline State", BndDesc, RTDesc);
    if (!PSOOrErr)
      return PSOOrErr.takeError();
    State.Pipeline = std::move(*PSOOrErr);
    llvm::outs() << "RayTracing Pipeline created.\n";

    auto SBTOrErr = Dev.createShaderBindingTable(*State.Pipeline, *P.SBT);
    if (!SBTOrErr)
      return SBTOrErr.takeError();
    State.SBT = std::move(*SBTOrErr);
    llvm::outs() << "Shader Binding Table created.\n";

    if (auto Err = createComputeCommands(P, State, Dev, *DescriptorPool))
      return Err;
    llvm::outs() << "RayTracing command list created.\n";
  } else {
    return llvm::createStringError("Pipeline was neither Compute nor Raster");
  }

  auto EncoderOrErr = State.CB->createComputeEncoder();
  if (!EncoderOrErr)
    return EncoderOrErr.takeError();
  auto ReadbackEncoder = std::move(*EncoderOrErr);

  if (State.RenderTarget) {
    if (auto Err = ReadbackEncoder->copyTextureToBuffer(*State.RenderTarget,
                                                        *State.RTReadback))
      return Err;
  }

  for (auto &Table : State.DescTables)
    for (auto &R : Table.Resources)
      if (auto Err = copyBackResource(*ReadbackEncoder, R))
        return Err;

  for (auto &R : State.RootResources)
    if (auto Err = copyBackResource(*ReadbackEncoder, R))
      return Err;

  ReadbackEncoder->endEncoding();

  auto SubmitResult = Dev.getGraphicsQueue().submit(std::move(State.CB));
  if (!SubmitResult)
    return SubmitResult.takeError();
  llvm::outs() << "Compute commands executed.\n";
  if (auto Err = SubmitResult->waitForCompletion())
    return Err;
  if (auto Err = readBack(Dev, P, State))
    return Err;
  llvm::outs() << "Read data back.\n";

  return llvm::Error::success();
}

} // namespace offloadtest
