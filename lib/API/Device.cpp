//===- DX/Device.cpp - DirectX Device API ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/Device.h"
#include "API/Encoder.h"
#include "API/FormatConversion.h"

#include "Config.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

using namespace offloadtest;

CommandEncoder::~CommandEncoder() {}

Buffer::~Buffer() {}

CommandBuffer::~CommandBuffer() {}

Fence::~Fence() {}

Queue::~Queue() {}

Texture::~Texture() {}

RenderPass::~RenderPass() {}

AccelerationStructure::~AccelerationStructure() {}

Device::~Device() {}

llvm::Expected<llvm::SmallVector<std::unique_ptr<Device>>>
offloadtest::initializeDevices(const DeviceConfig Config) {
  llvm::SmallVector<std::unique_ptr<Device>> Devices;
  llvm::Error Err = llvm::Error::success();

#ifdef OFFLOADTEST_ENABLE_D3D12
  if (auto E = initializeDX12Devices(Config, Devices))
    Err = llvm::joinErrors(std::move(Err), std::move(E));
#endif

#ifdef OFFLOADTEST_ENABLE_VULKAN
  if (auto E = initializeVulkanDevices(Config, Devices))
    Err = llvm::joinErrors(std::move(Err), std::move(E));
#endif

#ifdef OFFLOADTEST_ENABLE_METAL
  if (auto E = initializeMetalDevices(Config, Devices))
    Err = llvm::joinErrors(std::move(Err), std::move(E));
#endif

  if (Devices.empty()) {
    if (Err)
      return std::move(Err);
    return llvm::createStringError(std::errc::no_such_device,
                                   "No GPU devices found.");
  }
  // Log errors from backends that failed while others succeeded.
  if (Err)
    llvm::logAllUnhandledErrors(std::move(Err), llvm::errs());
  return Devices;
}

llvm::Expected<std::unique_ptr<Texture>>
offloadtest::createRenderTargetFromCPUBuffer(Device &Dev,
                                             const CPUBuffer &Buf) {
  auto TexFmtOrErr = toFormat(Buf.Format, Buf.Channels);
  if (!TexFmtOrErr)
    return TexFmtOrErr.takeError();

  TextureCreateDesc Desc = {};
  Desc.Location = MemoryLocation::GpuOnly;
  Desc.Usage = TextureUsage::RenderTarget;
  Desc.Fmt = *TexFmtOrErr;
  Desc.Width = Buf.OutputProps.Width;
  Desc.Height = Buf.OutputProps.Height;
  Desc.MipLevels = 1;
  Desc.OptimizedClearValue = ClearColor{};

  if (auto Err = validateTextureDescMatchesCPUBuffer(Desc, Buf))
    return Err;

  return Dev.createTexture("RenderTarget", Desc);
}

llvm::Expected<std::unique_ptr<Texture>>
offloadtest::createDefaultDepthStencilTarget(Device &Dev, uint32_t Width,
                                             uint32_t Height) {
  TextureCreateDesc Desc = {};
  Desc.Location = MemoryLocation::GpuOnly;
  Desc.Usage = TextureUsage::DepthStencil;
  Desc.Fmt = Format::D32FloatS8Uint;
  Desc.Width = Width;
  Desc.Height = Height;
  Desc.MipLevels = 1;
  Desc.OptimizedClearValue = ClearDepthStencil{1.0f, 0};

  return Dev.createTexture("DepthStencil", Desc);
}

// This is a separate function because recursion is not allowed in this code
// base.
static llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
createUploadBufferWithData(Device &Dev, std::string Name, const void *Data,
                           size_t SizeInBytes) {

  // Create Upload buffer
  const BufferCreateDesc UploadDesc = BufferCreateDesc::uploadBuffer();
  const std::string UploadBufferName = Name + " (Upload Buffer)";

  auto UploadBufferOrErr =
      Dev.createBuffer(UploadBufferName, UploadDesc, SizeInBytes);
  if (!UploadBufferOrErr)
    return UploadBufferOrErr.takeError();
  auto UploadBuffer = std::move(*UploadBufferOrErr);

  // Copy data over
  auto MappedPtrOrErr = UploadBuffer->map();
  if (!MappedPtrOrErr)
    return MappedPtrOrErr.takeError();
  void *MappedPtr = *MappedPtrOrErr;
  memcpy(MappedPtr, Data, SizeInBytes);
  UploadBuffer->unmap();

  return std::move(UploadBuffer);
}

llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
offloadtest::createBufferWithData(
    Device &Dev, std::string Name, const BufferCreateDesc &Desc,
    const void *Data, size_t SizeInBytes, ComputeEncoder *Encoder,
    std::unique_ptr<offloadtest::Buffer> *OutUploadBuffer) {
  auto BufferOrErr = Dev.createBuffer(Name, Desc, SizeInBytes);
  if (!BufferOrErr)
    return BufferOrErr.takeError();
  auto Buffer = std::move(*BufferOrErr);

  if (Desc.Location == MemoryLocation::GpuOnly) {
    if (OutUploadBuffer == nullptr)
      return llvm::createStringError(
          "An upload buffer is required to create a GpuOnly buffer with data.");

    // Create Upload buffer
    auto UploadBufferOrErr =
        createUploadBufferWithData(Dev, Name, Data, SizeInBytes);
    if (!UploadBufferOrErr)
      return UploadBufferOrErr.takeError();
    *OutUploadBuffer = std::move(*UploadBufferOrErr);

    // Copy Buffer to Buffer
    if (auto Err = Encoder->copyBufferToBuffer(**OutUploadBuffer, 0, *Buffer, 0,
                                               SizeInBytes))
      return Err;

  } else {
    // Copy data over
    auto MappedPtrOrErr = Buffer->map();
    if (!MappedPtrOrErr)
      return MappedPtrOrErr.takeError();
    void *MappedPtr = *MappedPtrOrErr;
    memcpy(MappedPtr, Data, SizeInBytes);
    Buffer->unmap();
  }

  return Buffer;
}

llvm::Expected<std::unique_ptr<offloadtest::Texture>>
offloadtest::createTextureWithData(
    Device &Dev, std::string Name, const TextureCreateDesc &Desc,
    const void *Data, size_t SizeInBytes, ComputeEncoder *Encoder,
    std::unique_ptr<offloadtest::Buffer> *OutUploadBuffer) {

  const uint64_t PackedRowStrideInBytes =
      Desc.Width * getFormatSizeInBytes(Desc.Fmt);
  if (SizeInBytes < PackedRowStrideInBytes * Desc.Height)
    return llvm::createStringError(
        "Data upload is not enough for texture size.");

  auto TextureOrErr = Dev.createTexture(Name, Desc);
  if (!TextureOrErr)
    return TextureOrErr.takeError();
  auto Texture = std::move(*TextureOrErr);

  if (Desc.Location == MemoryLocation::GpuOnly) {
    if (OutUploadBuffer == nullptr)
      return llvm::createStringError("An upload buffer is required to create a "
                                     "GpuOnly texture with data.");

    const uint64_t TexRowStrideInBytes =
        Dev.getTextureUploadRowStrideInBytes(Desc);
    const uint64_t UploadBufferSizeInBytes =
        (Desc.Height - 1) * TexRowStrideInBytes + PackedRowStrideInBytes;

    // Create Upload buffer
    const BufferCreateDesc UploadDesc = BufferCreateDesc::uploadBuffer();
    std::string UploadBufferName = Name + " (Upload Buffer)";
    auto UploadBufferOrErr =
        Dev.createBuffer(UploadBufferName, UploadDesc, UploadBufferSizeInBytes);
    if (!UploadBufferOrErr)
      return UploadBufferOrErr.takeError();
    *OutUploadBuffer = std::move(*UploadBufferOrErr);

    auto MappedPtrOrErr = (*OutUploadBuffer)->map();
    if (!MappedPtrOrErr)
      return MappedPtrOrErr.takeError();

    uint8_t *DstPtr = (uint8_t *)*MappedPtrOrErr;
    const uint8_t *SrcPtr = (const uint8_t *)Data;

    for (uint32_t Y = 0; Y < Desc.Height; ++Y) {
      memcpy(DstPtr, SrcPtr, PackedRowStrideInBytes);
      DstPtr += TexRowStrideInBytes;
      SrcPtr += PackedRowStrideInBytes;
    }
    (*OutUploadBuffer)->unmap();

    // Copy Buffer to Buffer
    if (auto Err = Encoder->copyBufferToTexture(**OutUploadBuffer, *Texture))
      return Err;

  } else {
    auto MappedStrideInBytesOrErr = Texture->getMappedRowPitchInBytes();
    if (!MappedStrideInBytesOrErr)
      return MappedStrideInBytesOrErr.takeError();
    const uint32_t MappedStrideInBytes = *MappedStrideInBytesOrErr;

    // Copy data over
    auto MappedPtrOrErr = Texture->map();
    if (!MappedPtrOrErr)
      return MappedPtrOrErr.takeError();

    uint8_t *DstPtr = (uint8_t *)*MappedPtrOrErr;
    const uint8_t *SrcPtr = (const uint8_t *)Data;

    for (uint32_t Y = 0; Y < Desc.Height; ++Y) {
      memcpy(DstPtr, SrcPtr, PackedRowStrideInBytes);
      DstPtr += MappedStrideInBytes;
      SrcPtr += PackedRowStrideInBytes;
    }

    Texture->unmap();
  }

  return Texture;
}
