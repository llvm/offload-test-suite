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
#include "API/FormatConversion.h"

#include "Config.h"

#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>

using namespace offloadtest;

Buffer::~Buffer() {}

CommandBuffer::~CommandBuffer() {}

Fence::~Fence() {}

Queue::~Queue() {}

Texture::~Texture() {}

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

llvm::Expected<VertexBuffer>
offloadtest::createVertexBuffer(Device &Dev, const ParsedVertexBuffer &PVB) {
  BufferCreateDesc BufDesc = {};
  BufDesc.Location = MemoryLocation::CpuToGpu;
  BufDesc.Usage = BufferUsage::VertexBuffer;
  auto BufOrErr =
      Dev.createBuffer("VertexBuffer", BufDesc, PVB.InterleavedSize);
  if (!BufOrErr)
    return BufOrErr.takeError();

  VertexBufferDesc VBDesc;
  for (const auto &S : PVB.Streams)
    VBDesc.Streams.push_back({S.Name, S.Fmt});

  // TODO: Generalize VB data copy so that we can deduplicate that from each
  // backend.

  return VertexBuffer{VBDesc, *BufOrErr};
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
