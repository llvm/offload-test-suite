#include "OffloadMigration.h"

#include "API/Device.h"
#include "API/FormatConversion.h"

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
  auto SizesOrErr =
      Dev.getTLASBuildSizes(static_cast<uint32_t>(R.TLASPtr->Instances.size()));
  if (!SizesOrErr)
    return SizesOrErr.takeError();
  return Dev.createTLAS(*SizesOrErr);
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
      if (R.BufferPtr->OutputProps.MipLevels != 1)
        return llvm::createStringError(std::errc::not_supported,
                                       "Multiple mip levels are not yet "
                                       "supported for DirectX textures.");

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
      CreateDesc.MipLevels = 1;

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
              createTextureWithData(Dev, "Texture", CreateDesc, Data.get(),
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

        IS.KeepAliveBuffers.push_back(std::move(UploadBuffer));
        ResourceSet RSet(std::move(Texture), std::move(BackingMemoryHeap),
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
    } else {
      return llvm::createStringError(std::errc::not_supported,
                                     "Samplers are not yet implemented.");
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

llvm::Error createRenderTarget(Device &Dev, Pipeline &P,
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

llvm::Error createDepthStencil(Device &Dev, Pipeline &P,
                               SharedInvocationState &IS) {
  auto TexOrErr = offloadtest::createDefaultDepthStencilTarget(
      Dev, P.Bindings.RTargetBufferPtr->OutputProps.Width,
      P.Bindings.RTargetBufferPtr->OutputProps.Height);
  if (!TexOrErr)
    return TexOrErr.takeError();
  IS.DepthStencil = std::move(*TexOrErr);
  return llvm::Error::success();
}

} // namespace offloadtest
