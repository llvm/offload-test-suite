//===- DX/Descriptors.cpp - DirectX Desriptors API ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "API/DX/Descriptors.h"
#include "API/DX/AccelerationStructure.h"
#include "API/DX/Buffer.h"
#include "API/DX/Sampler.h"
#include "API/DX/Texture.h"

#include "Support/WinError.h"

using namespace offloadtest;

DXDescriptorSets::DXDescriptorSets(llvm::SmallVector<DXDescriptorSet> Sets)
    : DescriptorSets(GPUAPI::DirectX), Sets(std::move(Sets)) {}

bool DXDescriptorSets::classof(const DescriptorSets *S) {
  return S->getAPI() == GPUAPI::DirectX;
}

DXDescriptorPool::DXDescriptorPool(ComPtr<ID3D12DescriptorHeap> CSUHeap,
                                   ComPtr<ID3D12DescriptorHeap> SamplerHeap,
                                   uint32_t CSUIncSize, uint32_t SamplerIncSize)
    : DescriptorPool(GPUAPI::DirectX), CSUHeap(CSUHeap),
      SamplerHeap(SamplerHeap), CSUIncSize(CSUIncSize),
      SamplerIncSize(SamplerIncSize) {
  CSUHandle = this->CSUHeap->GetCPUDescriptorHandleForHeapStart();
  CSUHandleGPU = this->CSUHeap->GetGPUDescriptorHandleForHeapStart();

  SamplerHandle = this->SamplerHeap->GetCPUDescriptorHandleForHeapStart();
  SamplerHandleGPU = this->SamplerHeap->GetGPUDescriptorHandleForHeapStart();
}

llvm::Expected<std::unique_ptr<DescriptorPool>>
DXDescriptorPool::create(ComPtr<ID3D12DeviceX> Dev) {
  const uint32_t DescriptorCount = 4096;
  const uint32_t SamplerCount = 512;

  const D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DescriptorCount,
      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};

  ComPtr<ID3D12DescriptorHeap> CSUHeap;
  if (auto Err = HR::toError(
          Dev->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&CSUHeap)),
          "Failed to create descriptor heap."))
    return Err;

  const D3D12_DESCRIPTOR_HEAP_DESC SamplerHeapDesc = {
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, SamplerCount,
      D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
  ComPtr<ID3D12DescriptorHeap> SamplerHeap;
  if (auto Err = HR::toError(Dev->CreateDescriptorHeap(
                                 &SamplerHeapDesc, IID_PPV_ARGS(&SamplerHeap)),
                             "Failed to create sampler descriptor heap."))
    return Err;

  const uint32_t CSUIncSize = Dev->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  const uint32_t SamplerIncSize =
      Dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

  return std::make_unique<DXDescriptorPool>(CSUHeap, SamplerHeap, CSUIncSize,
                                            SamplerIncSize);
}

void DXDescriptorPool::allocateDescriptors(uint32_t Count,
                                           D3D12_CPU_DESCRIPTOR_HANDLE &CPU,
                                           D3D12_GPU_DESCRIPTOR_HANDLE &GPU) {
  const uint32_t Idx = CSUAllocator.fetch_add(Count);
  const uint32_t Offset = Idx * CSUIncSize;
  CPU = {CSUHandle.ptr + Offset};
  GPU = {CSUHandleGPU.ptr + Offset};
}

void DXDescriptorPool::allocateSamplers(uint32_t Count,
                                        D3D12_CPU_DESCRIPTOR_HANDLE &CPU,
                                        D3D12_GPU_DESCRIPTOR_HANDLE &GPU) {
  const uint32_t Idx = SamplerAllocator.fetch_add(Count);
  const uint32_t Offset = Idx * SamplerIncSize;
  CPU = {SamplerHandle.ptr + Offset};
  GPU = {SamplerHandleGPU.ptr + Offset};
}

void DXDescriptorPool::reset() {
  CSUAllocator.store(0);
  SamplerAllocator.store(0);
}

bool DXDescriptorPool::classof(const DescriptorPool *P) {
  return P->getAPI() == GPUAPI::DirectX;
}

DXDescriptorSetsBuilder::DXDescriptorSetsBuilder(
    ComPtr<ID3D12DeviceX> Dev, llvm::SmallVector<DXDescriptorSet> Sets,
    uint32_t CSUIncSize, uint32_t SamplerIncSize)
    : DescriptorSetsBuilder(GPUAPI::DirectX), Dev(Dev), Sets(std::move(Sets)),
      CSUIncSize(CSUIncSize), SamplerIncSize(SamplerIncSize) {
  SetStates.reserve(this->Sets.size());
  for (const auto &Set : this->Sets) {
    SetStates.push_back({Set.CSUHandle, Set.SamplerHandle});
  }
}

DescriptorSetsBuilder &
DXDescriptorSetsBuilder::constant(uint32_t SetIndex,
                                  llvm::ArrayRef<const Buffer *> B, VKBind) {
  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (const Buffer *Buf : B) {
    const DXBuffer &BufferDX = llvm::cast<DXBuffer>(*Buf);
    Dev->CopyDescriptorsSimple(1, State.CSUHandle, BufferDX.CBVHandle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    State.CSUHandle.ptr += CSUIncSize;
  }
  return *this;
}

DescriptorSetsBuilder &
DXDescriptorSetsBuilder::read(uint32_t SetIndex,
                              llvm::ArrayRef<const Buffer *> B, VKBind) {
  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (const Buffer *Buf : B) {
    const DXBuffer &BufferDX = llvm::cast<DXBuffer>(*Buf);
    Dev->CopyDescriptorsSimple(1, State.CSUHandle, BufferDX.SRVHandle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    State.CSUHandle.ptr += CSUIncSize;
  }
  return *this;
}
DescriptorSetsBuilder &
DXDescriptorSetsBuilder::read(uint32_t SetIndex,
                              llvm::ArrayRef<const Texture *> T,
                              llvm::ArrayRef<const Sampler *> S, VKBind) {
  assert((S.empty() || S.size() == T.size()) &&
         "Sampler list must either be empty or match "
         "texture list when binding descriptors.");

  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (size_t I = 0, N = T.size(); I < N; ++I) {
    const DXTexture &TextureDX = llvm::cast<DXTexture>(*T[I]);
    assert((S.empty() || S[I] == nullptr) &&
           "DirectX 12 does not support combined image samplers.");
    Dev->CopyDescriptorsSimple(1, State.CSUHandle, TextureDX.SRVHandle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    State.CSUHandle.ptr += CSUIncSize;
  }
  return *this;
}
DescriptorSetsBuilder &
DXDescriptorSetsBuilder::read(uint32_t SetIndex,
                              llvm::ArrayRef<const AccelerationStructure *> A,
                              VKBind) {
  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (const AccelerationStructure *AS : A) {
    const DXAccelerationStructure &AccelStructDX =
        llvm::cast<DXAccelerationStructure>(*AS);
    Dev->CopyDescriptorsSimple(1, State.CSUHandle, AccelStructDX.SRVHandle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    State.CSUHandle.ptr += CSUIncSize;
  }
  return *this;
}

DescriptorSetsBuilder &
DXDescriptorSetsBuilder::write(uint32_t SetIndex,
                               llvm::ArrayRef<const Buffer *> B, VKBind) {
  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (const Buffer *Buf : B) {
    const DXBuffer &BufferDX = llvm::cast<DXBuffer>(*Buf);
    Dev->CopyDescriptorsSimple(1, State.CSUHandle, BufferDX.UAVHandle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    State.CSUHandle.ptr += CSUIncSize;
  }
  return *this;
}
DescriptorSetsBuilder &
DXDescriptorSetsBuilder::write(uint32_t SetIndex,
                               llvm::ArrayRef<const Texture *> T, VKBind) {
  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (const Texture *Tex : T) {
    const DXTexture &TextureDX = llvm::cast<DXTexture>(*Tex);
    Dev->CopyDescriptorsSimple(1, State.CSUHandle, TextureDX.UAVHandle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    State.CSUHandle.ptr += CSUIncSize;
  }
  return *this;
}

DescriptorSetsBuilder &
DXDescriptorSetsBuilder::sampler(uint32_t SetIndex,
                                 llvm::ArrayRef<const Sampler *> S, VKBind) {
  if (SetIndex >= SetStates.size())
    return *this;
  SetState &State = SetStates[SetIndex];
  for (const Sampler *Sampl : S) {
    const DXSampler &SamplerDX = llvm::cast<DXSampler>(*Sampl);
    Dev->CopyDescriptorsSimple(1, State.SamplerHandle, SamplerDX.Handle,
                               D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    State.SamplerHandle.ptr += SamplerIncSize;
  }
  return *this;
}

std::unique_ptr<DescriptorSets> DXDescriptorSetsBuilder::build() {
  return std::make_unique<DXDescriptorSets>(std::move(Sets));
}

bool DXDescriptorSetsBuilder::classof(const DescriptorSetsBuilder *B) {
  return B->getAPI() == GPUAPI::DirectX;
}
