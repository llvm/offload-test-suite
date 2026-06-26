//===- MTL/MTLDescriptorHeap.cpp - Metal Descriptor Heap ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MTLDescriptorHeap.h"
#include "MetalIRConverter.h"
#include "ResidencyTracker.h"

using namespace offloadtest;

static NS::UInteger getDescriptorHeapBindPoint(MTLDescriptorHeapType Type) {
  switch (Type) {
  case MTLDescriptorHeapType::CBV_SRV_UAV:
    return kIRDescriptorHeapBindPoint;
  case MTLDescriptorHeapType::Sampler:
    return kIRSamplerHeapBindPoint;
  }
  llvm_unreachable("All cases handled.");
}

MTLGPUDescriptorHandle &
MTLGPUDescriptorHandle::addOffset(int32_t OffsetInDescriptors) {
  Ptr = MTL::GPUAddress(int64_t(Ptr) + int64_t(OffsetInDescriptors) *
                                           sizeof(IRDescriptorTableEntry));
  return *this;
}

llvm::Expected<std::unique_ptr<MTLDescriptorHeap>> MTLDescriptorHeap::create(
    MTL::Device *Device, const MTLDescriptorHeapDesc &Desc,
    std::shared_ptr<MetalResidencyTracker> ResidencyTracker) {
  if (!Device)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Invalid MTL::Device pointer.");

  if (Desc.NumDescriptors == 0)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Invalid descriptor heap description.");

  MTL::Buffer *Buf =
      Device->newBuffer(Desc.NumDescriptors * sizeof(IRDescriptorTableEntry),
                        MTL::ResourceStorageModeShared);
  if (!Buf)
    return llvm::createStringError(std::errc::not_enough_memory,
                                   "Failed to create MTLDescriptorHeap.");
  ResidencyTracker->withLock(
      [&](MTL::ResidencySet *RS) { RS->addAllocation(Buf); });
  return std::make_unique<MTLDescriptorHeap>(Desc, Buf,
                                             std::move(ResidencyTracker));
}

MTLDescriptorHeap::~MTLDescriptorHeap() {
  if (Buffer) {
    ResidencyTracker->withLock(
        [&](MTL::ResidencySet *RS) { RS->removeAllocation(Buffer); });
    Buffer->release();
  }
}

MTLGPUDescriptorHandle
MTLDescriptorHeap::getGPUDescriptorHandleForHeapStart() const {
  return MTLGPUDescriptorHandle{Buffer->gpuAddress()};
}

IRDescriptorTableEntry *
MTLDescriptorHeap::getEntryHandle(uint32_t Index) const {
  assert(Index < Desc.NumDescriptors && "Descriptor index out of bounds.");
  return static_cast<IRDescriptorTableEntry *>(Buffer->contents()) + Index;
}

void MTLDescriptorHeap::bind(MTL::RenderCommandEncoder *Encoder) {
  // Dynamic resource indexing
  const NS::UInteger BindPoint = getDescriptorHeapBindPoint(Desc.Type);
  Encoder->setVertexBuffer(Buffer, 0, BindPoint);
  Encoder->setFragmentBuffer(Buffer, 0, BindPoint);
}

void MTLDescriptorHeap::bind(MTL::ComputeCommandEncoder *Encoder) {
  // Dynamic resource indexing
  const NS::UInteger BindPoint = getDescriptorHeapBindPoint(Desc.Type);
  Encoder->setBuffer(Buffer, 0, BindPoint);
}
