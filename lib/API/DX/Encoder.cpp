//===- DX/Encoder.cpp - DirectX Encoder API -------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include <d3dx12.h>

#include "API/DX/AccelerationStructure.h"
#include "API/DX/Buffer.h"
#include "API/DX/CommandBuffer.h"
#include "API/DX/Descriptors.h"
#include "API/DX/Device.h"
#include "API/DX/Encoder.h"
#include "API/DX/PipelineState.h"
#include "API/DX/SBT.h"
#include "API/DX/Texture.h"
#include "Support/WinError.h"

#include "DXResources.h"

#include "llvm/Support/FormatVariadic.h"

using namespace offloadtest;

void DXComputeEncoder::addUAVBarrier() {
  CB.addPendingUAVBarrier();
  CB.flushBarrier();
}

DXComputeEncoder::~DXComputeEncoder() { endEncoding(); }

void DXComputeEncoder::bindDescriptorSets(const PipelineState &PSO,
                                          const DescriptorSets &DSets) {
  const auto &DXPSO = llvm::cast<DXPipelineState>(PSO);
  const DXDescriptorSets &DSetsDX = llvm::cast<DXDescriptorSets>(DSets);
  assert(DSetsDX.Sets.size() == DXPSO.Layout.Sets.size() &&
         "Descriptor layout must match DescriptorSets");

  CB.CmdList->SetComputeRootSignature(DXPSO.RootSig.Get());

  for (size_t I = 0, J = 0; I < DSetsDX.Sets.size(); ++I) {
    const auto &Set = DSetsDX.Sets[I];
    const auto &SetLayout = DXPSO.Layout.Sets[I];

    if (SetLayout.DescriptorCount > 0) {
      CB.CmdList->SetComputeRootDescriptorTable(J, Set.CSUHandleGPU);
      assert(DXPSO.Layout.RSigLayout[J].ParameterType ==
                 RootParameterType::DescriptorTable &&
             "Descriptor layout doesn't match root signature.");
      J += 1;
    }

    if (SetLayout.SamplerCount > 0) {
      CB.CmdList->SetComputeRootDescriptorTable(J, Set.SamplerHandleGPU);
      assert(DXPSO.Layout.RSigLayout[J].ParameterType ==
                 RootParameterType::SamplerTable &&
             "Descriptor layout doesn't match root signature.");
      J += 1;
    }
  }
}

void DXComputeEncoder::bindComputeDescriptorSets(const PipelineState &PSO,
                                                 const DescriptorSets &DSets) {
  return bindDescriptorSets(PSO, DSets);
}

void DXComputeEncoder::bindRayTracingDescriptorSets(
    const PipelineState &PSO, const DescriptorSets &DSets) {
  return bindDescriptorSets(PSO, DSets);
}

llvm::Error DXComputeEncoder::dispatch(const offloadtest::PipelineState &PSO,
                                       uint32_t GroupCountX,
                                       uint32_t GroupCountY,
                                       uint32_t GroupCountZ) {
  const auto &DXPSO = llvm::cast<DXPipelineState>(PSO);
  addUAVBarrier();
  insertDebugSignpost(llvm::formatv("Dispatch [{0},{1},{2}]", GroupCountX,
                                    GroupCountY, GroupCountZ)
                          .str());
  CB.CmdList->SetPipelineState(DXPSO.PSO.Get());
  CB.CmdList->Dispatch(GroupCountX, GroupCountY, GroupCountZ);
  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::copyBufferToBuffer(offloadtest::Buffer &Src,
                                                 size_t SrcOffset,
                                                 offloadtest::Buffer &Dst,
                                                 size_t DstOffset,
                                                 size_t Size) {
  auto &DXSrc = static_cast<DXBuffer &>(Src);
  auto &DXDst = static_cast<DXBuffer &>(Dst);

  // NOTE: Edge case in case of all the following being the case
  // - multiple calls of copyBufferToBuffer with the same Dst Buffer
  // - The Dst Buffer having a PreferredState of
  // D3D12_RESOURCE_STATE_COPY_DEST
  // - Each Src Buffer having a PreferredState of
  // D3D12_RESOURCE_STATE_COPY_SOURCE
  // In that case no barrier would be emitted
  // and a race condition would occur. There are ways to solve this with
  // legacy barriers, but switching to enhanced barriers is a better solution
  // to this problem.

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Buffer.Get(), DXSrc.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_SOURCE);
  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Buffer.Get(), DXDst.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_DEST);
  CB.flushBarrier();

  insertDebugSignpost(llvm::formatv("CopyBuffer {0}B", Size).str());
  CB.CmdList->CopyBufferRegion(DXDst.Buffer.Get(), DstOffset,
                               DXSrc.Buffer.Get(), SrcOffset, Size);

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Buffer.Get(),
                             D3D12_RESOURCE_STATE_COPY_SOURCE,
                             DXSrc.PreferredState);
  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                             DXDst.PreferredState);

  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::copyBufferToTexture(Buffer &Src, Texture &Dst) {
  auto &DXSrc = llvm::cast<DXBuffer>(Src);
  auto &DXDst = llvm::cast<DXTexture>(Dst);

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Buffer.Get(), DXSrc.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_SOURCE);

  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Resource.Get(), DXDst.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_DEST);
  CB.flushBarrier();

  const D3D12_RESOURCE_DESC TexDesc = DXDst.Resource->GetDesc();
  const uint32_t NumSubresources = TexDesc.MipLevels;
  llvm::SmallVector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Layouts(
      NumSubresources);
  ComPtr<ID3D12DeviceX> Device;
  DXDst.Resource->GetDevice(IID_PPV_ARGS(&Device));
  Device->GetCopyableFootprints(&TexDesc, 0, NumSubresources, 0, Layouts.data(),
                                nullptr, nullptr, nullptr);
  for (uint32_t Sub = 0; Sub < NumSubresources; ++Sub) {
    const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(DXDst.Resource.Get(), Sub);
    const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(DXSrc.Buffer.Get(),
                                               Layouts[Sub]);
    CB.CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);
  }

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Buffer.Get(),
                             D3D12_RESOURCE_STATE_COPY_SOURCE,
                             DXSrc.PreferredState);

  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Resource.Get(),
                             D3D12_RESOURCE_STATE_COPY_DEST,
                             DXDst.PreferredState);

  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::copyCounterToBuffer(offloadtest::Buffer &Src,
                                                  offloadtest::Buffer &Dst) {
  auto &DXSrc = llvm::cast<DXBuffer>(Src);
  auto &DXDst = llvm::cast<DXBuffer>(Dst);

  if (!DXSrc.Desc.HasCounter)
    return llvm::createStringError(
        "Counter resource passed does not hvae a counter.");

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Buffer.Get(), DXSrc.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_SOURCE);
  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Buffer.Get(), DXDst.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_DEST);
  CB.flushBarrier();

  insertDebugSignpost("copyCounterToBuffer 4B");
  CB.CmdList->CopyBufferRegion(DXDst.Buffer.Get(), 0, DXSrc.Buffer.Get(),
                               DXSrc.CounterOffsetInBytes, sizeof(uint32_t));

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Buffer.Get(),
                             D3D12_RESOURCE_STATE_COPY_SOURCE,
                             DXSrc.PreferredState);
  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                             DXDst.PreferredState);

  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::copyTextureToBuffer(Texture &Src, Buffer &Dst) {
  auto &DXSrc = llvm::cast<DXTexture>(Src);
  auto &DXDst = llvm::cast<DXBuffer>(Dst);

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Resource.Get(), DXSrc.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_SOURCE);

  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Buffer.Get(), DXDst.PreferredState,
                             D3D12_RESOURCE_STATE_COPY_DEST);

  CB.flushBarrier();

  const uint32_t ElementSize = getFormatSizeInBytes(DXSrc.Desc.Fmt);
  const D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint{
      0, CD3DX12_SUBRESOURCE_FOOTPRINT(
             getDXGIFormat(DXSrc.Desc.Fmt), DXSrc.Desc.Width, DXSrc.Desc.Height,
             1, getAlignedTexturePitch(DXSrc.Desc.Width, ElementSize))};
  const CD3DX12_TEXTURE_COPY_LOCATION DstLoc(DXDst.Buffer.Get(), Footprint);
  const CD3DX12_TEXTURE_COPY_LOCATION SrcLoc(DXSrc.Resource.Get(), 0);
  CB.CmdList->CopyTextureRegion(&DstLoc, 0, 0, 0, &SrcLoc, nullptr);

  if (DXSrc.PreferredState != D3D12_RESOURCE_STATE_COPY_SOURCE)
    CB.addResourceTransition(DXSrc.Resource.Get(),
                             D3D12_RESOURCE_STATE_COPY_SOURCE,
                             DXSrc.PreferredState);

  if (DXDst.PreferredState != D3D12_RESOURCE_STATE_COPY_DEST)
    CB.addResourceTransition(DXDst.Buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                             DXDst.PreferredState);

  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) {
  if (Items.empty())
    return llvm::Error::success();
  if (!CB.Dev || !CB.Dev->Device)
    return llvm::createStringError(
        std::errc::not_supported,
        "Ray tracing not supported on this command buffer's device.");
  DXDevice *Dev = CB.Dev;

  // BuildRaytracingAccelerationStructure() lives on ID3D12GraphicsCommandList4.
  ComPtr<ID3D12GraphicsCommandList4> CmdList4;
  if (auto Err = HR::toError(CB.CmdList.As(&CmdList4),
                             "Failed to query ID3D12GraphicsCommandList4."))
    return Err;

  // Flush a pending barrier before reading, like dispatch(): a TLAS build must
  // observe BLASes built in the previous batch.
  CB.flushBarrier();

  // Per the ComputeEncoder::batchBuildAS() contract, the caller guarantees no
  // inter-item memory dependencies within a batch (BLAS and TLAS go in
  // separate batches, so a TLAS never sees BLASes from the same call). Each
  // item also gets its own scratch resource, so there's no aliasing between
  // the builds — no intra-loop UAV barrier is needed.
  for (const auto &Item : Items) {
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC Desc = {};
    llvm::SmallVector<D3D12_RAYTRACING_GEOMETRY_DESC> GeomDescs;
    uint64_t ScratchSize = 0;

    if (const auto *BLAS = llvm::dyn_cast<const BLASBuildRequest *>(Item)) {
      auto *DXAS = llvm::cast<DXAccelerationStructure>(BLAS->AS);
      Desc.DestAccelerationStructureData = DXAS->getGPUVirtualAddress();
      Desc.Inputs.Type =
          D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
      Desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

      if (const auto *Tris =
              std::get_if<llvm::SmallVector<TriangleGeometryDesc>>(
                  &BLAS->Geometry)) {
        GeomDescs.reserve(Tris->size());
        for (const auto &T : *Tris) {
          D3D12_RAYTRACING_GEOMETRY_DESC GD = {};
          GD.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
          if (T.Opaque)
            GD.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
          auto *VB = llvm::cast<DXBuffer>(T.VertexBuffer);
          GD.Triangles.VertexBuffer.StartAddress =
              VB->Buffer->GetGPUVirtualAddress() + T.VertexBufferOffset;
          GD.Triangles.VertexBuffer.StrideInBytes = T.VertexStride;
          GD.Triangles.VertexCount = T.VertexCount;
          GD.Triangles.VertexFormat = getDXGIFormat(T.VertexFormat);
          if (T.IndexBuffer) {
            auto *IB = llvm::cast<DXBuffer>(T.IndexBuffer);
            GD.Triangles.IndexBuffer =
                IB->Buffer->GetGPUVirtualAddress() + T.IndexBufferOffset;
            GD.Triangles.IndexCount = T.IndexCount;
            GD.Triangles.IndexFormat = getDXGIIndexFormat(T.IdxFormat);
          }
          GeomDescs.push_back(GD);
        }
      } else {
        const auto &AABBs =
            std::get<llvm::SmallVector<AABBGeometryDesc>>(BLAS->Geometry);
        GeomDescs.reserve(AABBs.size());
        for (const auto &A : AABBs) {
          D3D12_RAYTRACING_GEOMETRY_DESC GD = {};
          GD.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
          if (A.Opaque)
            GD.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
          auto *AB = llvm::cast<DXBuffer>(A.AABBBuffer);
          GD.AABBs.AABBs.StartAddress =
              AB->Buffer->GetGPUVirtualAddress() + A.AABBBufferOffset;
          GD.AABBs.AABBs.StrideInBytes = A.AABBStride;
          GD.AABBs.AABBCount = A.AABBCount;
          GeomDescs.push_back(GD);
        }
      }
      Desc.Inputs.NumDescs = static_cast<UINT>(GeomDescs.size());
      Desc.Inputs.pGeometryDescs = GeomDescs.data();
      ScratchSize = BLAS->AS->getSizes().ScratchDataSizeInBytes;
    } else {
      const auto *TLAS = llvm::cast<const TLASBuildRequest *>(Item);
      auto *DXAS = llvm::cast<DXAccelerationStructure>(TLAS->AS);
      Desc.DestAccelerationStructureData = DXAS->getGPUVirtualAddress();
      Desc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
      Desc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
      Desc.Inputs.NumDescs = static_cast<UINT>(TLAS->Instances.size());

      // D3D12_RAYTRACING_INSTANCE_DESC has the same byte layout as
      // VkAccelerationStructureInstanceKHR. Serialize and upload via the
      // shared abstract-API helper using an upload-heap (CpuToGpu) buffer.
      llvm::SmallVector<D3D12_RAYTRACING_INSTANCE_DESC> Native;
      Native.reserve(TLAS->Instances.size());
      for (const auto &Inst : TLAS->Instances) {
        D3D12_RAYTRACING_INSTANCE_DESC NI = {};
        static_assert(sizeof(NI.Transform) == sizeof(Inst.Transform),
                      "Transform layout mismatch");
        memcpy(&NI.Transform, Inst.Transform, sizeof(Inst.Transform));
        // D3D12_RAYTRACING_INSTANCE_DESC packs InstanceID into a 24-bit
        // bitfield; truncate explicitly so the value matches the VK path
        // (vkInstanceCustomIndex is likewise 24-bit) instead of relying on
        // silent narrowing.
        NI.InstanceID = Inst.InstanceID & 0xFFFFFFu;
        NI.InstanceMask = Inst.InstanceMask;
        NI.InstanceContributionToHitGroupIndex =
            Inst.InstanceContributionToHitGroupIndex & 0xFFFFFFu;
        // Bits in AccelerationStructureInstanceFlags match
        // D3D12_RAYTRACING_INSTANCE_FLAGS by design.
        NI.Flags = static_cast<D3D12_RAYTRACING_INSTANCE_FLAGS>(Inst.Flags);
        auto *BLASPtr = llvm::cast<DXAccelerationStructure>(Inst.BLAS);
        NI.AccelerationStructure = BLASPtr->getGPUVirtualAddress();
        Native.push_back(NI);
      }
      const size_t Bytes =
          Native.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

      const BufferCreateDesc UploadDesc = BufferCreateDesc::uploadBuffer();
      auto InstBufOrErr = offloadtest::createBufferWithData(
          *Dev, "TLAS-Instances", UploadDesc, Native.data(), Bytes, nullptr,
          nullptr);
      if (!InstBufOrErr)
        return InstBufOrErr.takeError();
      auto *DXInstBuf = llvm::cast<DXBuffer>(InstBufOrErr->get());
      Desc.Inputs.InstanceDescs = DXInstBuf->Buffer->GetGPUVirtualAddress();

      CB.KeepAliveOwned.push_back(std::move(*InstBufOrErr));
      ScratchSize = TLAS->AS->getSizes().ScratchDataSizeInBytes;
    }

    const BufferCreateDesc ScratchDesc = BufferCreateDesc::scratchBuffer();
    auto ScratchOrErr =
        Dev->createBuffer("AS-Scratch", ScratchDesc, ScratchSize);
    if (!ScratchOrErr)
      return ScratchOrErr.takeError();
    auto *DXScratchBuf = llvm::cast<DXBuffer>(ScratchOrErr->get());
    Desc.ScratchAccelerationStructureData =
        DXScratchBuf->Buffer->GetGPUVirtualAddress();
    CB.KeepAliveOwned.push_back(std::move(*ScratchOrErr));

    insertDebugSignpost("BuildRaytracingAccelerationStructure");
    CmdList4->BuildRaytracingAccelerationStructure(&Desc, 0, nullptr);
  }

  // Signal that this batch's AS writes need a barrier before the next reader.
  CB.addPendingUAVBarrier();
  return llvm::Error::success();
}

llvm::Error DXComputeEncoder::dispatchRays(const PipelineState &PSO,
                                           const ShaderBindingTable &SBT,
                                           uint32_t Width, uint32_t Height,
                                           uint32_t Depth) {
  if (!llvm::isa<DXRayTracingPipelineState>(&PSO))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a RayTracing PipelineState.");
  if (!llvm::isa<DXShaderBindingTable>(&SBT))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a DirectX ShaderBindingTable.");
  const auto &DXRTPSO = llvm::cast<DXRayTracingPipelineState>(PSO);
  const auto &DXSBT = llvm::cast<DXShaderBindingTable>(SBT);

  // SetPipelineState1 and DispatchRays live on ID3D12GraphicsCommandList4.
  // The AS-build path (line ~3000 above) follows the same query pattern.
  ComPtr<ID3D12GraphicsCommandList4> CmdList4;
  if (auto Err =
          HR::toError(CB.CmdList.As(&CmdList4),
                      "ID3D12GraphicsCommandList4 query failed; raytracing "
                      "is unsupported on this command list."))
    return Err;

  addUAVBarrier();
  insertDebugSignpost(
      llvm::formatv("DispatchRays [{0},{1},{2}]", Width, Height, Depth).str());

  // Global root signature is shared with the compute bind point; bind it on
  // the underlying command list before SetPipelineState1.
  CB.CmdList->SetComputeRootSignature(DXRTPSO.RootSig.Get());
  CmdList4->SetPipelineState1(DXRTPSO.StateObject.Get());

  D3D12_DISPATCH_RAYS_DESC RaysDesc{};
  RaysDesc.RayGenerationShaderRecord = DXSBT.RayGenRange;
  RaysDesc.MissShaderTable = DXSBT.MissRange;
  RaysDesc.HitGroupTable = DXSBT.HitGroupRange;
  RaysDesc.CallableShaderTable = DXSBT.CallableRange;
  RaysDesc.Width = Width;
  RaysDesc.Height = Height;
  RaysDesc.Depth = Depth;
  CmdList4->DispatchRays(&RaysDesc);
  return llvm::Error::success();
}

void DXComputeEncoder::endEncodingImpl() { popDebugGroup(); }

llvm::Error
DXRenderEncoder::bindCommonDrawState(const offloadtest::PipelineState &PSO) {
  if (!ViewportSet)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Viewport must be set before drawing.");
  if (!ScissorSet)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Scissor must be set before drawing.");

  const auto &DXPSO = llvm::cast<DXPipelineState>(PSO);
  CB.CmdList->SetGraphicsRootSignature(DXPSO.RootSig.Get());
  CB.CmdList->SetPipelineState(DXPSO.PSO.Get());
  // Mesh-shader pipelines bypass the input assembler and carry no IA
  // topology; only bind one when the pipeline actually has one.
  if (DXPSO.Topology)
    CB.CmdList->IASetPrimitiveTopology(*DXPSO.Topology);
  return llvm::Error::success();
}

DXRenderEncoder::DXRenderEncoder(DXCommandBuffer &CB,
                                 const offloadtest::RenderPassBeginDesc &Desc)
    : RenderEncoder(GPUAPI::DirectX), CB(CB), Desc(Desc) {}

DXRenderEncoder::~DXRenderEncoder() { endEncoding(); }

bool DXRenderEncoder::classof(const CommandEncoder *E) {
  return E->getAPI() == GPUAPI::DirectX;
}

// See DXComputeEncoder for why these are no-ops.
void DXRenderEncoder::pushDebugGroup(llvm::StringRef Label) {}
void DXRenderEncoder::popDebugGroup() {}
void DXRenderEncoder::insertDebugSignpost(llvm::StringRef Label) {}

void DXRenderEncoder::setViewport(const offloadtest::Viewport &VP) {
  D3D12_VIEWPORT DXVP = {};
  DXVP.TopLeftX = VP.X;
  DXVP.TopLeftY = VP.Y;
  DXVP.Width = VP.Width;
  DXVP.Height = VP.Height;
  DXVP.MinDepth = VP.MinDepth;
  DXVP.MaxDepth = VP.MaxDepth;
  CB.CmdList->RSSetViewports(1, &DXVP);
  ViewportSet = true;
}

void DXRenderEncoder::setScissor(const offloadtest::ScissorRect &Rect) {
  const D3D12_RECT DXRect = {Rect.X, Rect.Y,
                             static_cast<LONG>(Rect.X + Rect.Width),
                             static_cast<LONG>(Rect.Y + Rect.Height)};
  CB.CmdList->RSSetScissorRects(1, &DXRect);
  ScissorSet = true;
}

void DXRenderEncoder::setVertexBuffer(uint32_t Slot, offloadtest::Buffer *VB,
                                      size_t Offset, uint32_t Stride) {
  assert(Slot < D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT &&
         "Vertex buffer slot exceeds D3D12 IA input resource slot count");
  assert(Slot == 0 && "Pipeline input layout only describes slot 0");
  if (VB) {
    auto &DXVB = llvm::cast<DXBuffer>(*VB);
    D3D12_VERTEX_BUFFER_VIEW VBView = {};
    VBView.BufferLocation = DXVB.Buffer->GetGPUVirtualAddress() + Offset;
    VBView.SizeInBytes = static_cast<UINT>(DXVB.getSizeInBytes() - Offset);
    VBView.StrideInBytes = Stride;
    CB.CmdList->IASetVertexBuffers(Slot, 1, &VBView);
  } else {
    CB.CmdList->IASetVertexBuffers(Slot, 1, nullptr);
  }
}

void DXRenderEncoder::bindDescriptorSets(const PipelineState &PSO,
                                         const DescriptorSets &DSets) {
  const auto &DXPSO = llvm::cast<DXPipelineState>(PSO);
  const DXDescriptorSets &DSetsDX = llvm::cast<DXDescriptorSets>(DSets);
  assert(DSetsDX.Sets.size() == DXPSO.Layout.Sets.size() &&
         "Descriptor layout must match DescriptorSets");

  CB.CmdList->SetGraphicsRootSignature(DXPSO.RootSig.Get());

  for (size_t I = 0, J = 0; I < DSetsDX.Sets.size(); ++I) {
    const auto &Set = DSetsDX.Sets[I];
    const auto &SetLayout = DXPSO.Layout.Sets[I];

    if (SetLayout.DescriptorCount > 0) {
      CB.CmdList->SetGraphicsRootDescriptorTable(J, Set.CSUHandleGPU);
      assert(DXPSO.Layout.RSigLayout[J].ParameterType ==
                 RootParameterType::DescriptorTable &&
             "Descriptor layout doesn't match root signature.");
      J += 1;
    }

    if (SetLayout.SamplerCount > 0) {
      CB.CmdList->SetGraphicsRootDescriptorTable(J, Set.SamplerHandleGPU);
      assert(DXPSO.Layout.RSigLayout[J].ParameterType ==
                 RootParameterType::SamplerTable &&
             "Descriptor layout doesn't match root signature.");
      J += 1;
    }
  }
}

llvm::Error
DXRenderEncoder::drawInstanced(const offloadtest::PipelineState &PSO,
                               uint32_t VertexCount, uint32_t InstanceCount,
                               uint32_t FirstVertex, uint32_t FirstInstance) {
  if (auto Err = bindCommonDrawState(PSO))
    return Err;
  CB.CmdList->DrawInstanced(VertexCount, InstanceCount, FirstVertex,
                            FirstInstance);
  return llvm::Error::success();
}

llvm::Error DXRenderEncoder::dispatchMesh(const offloadtest::PipelineState &PSO,
                                          uint32_t GroupCountX,
                                          uint32_t GroupCountY,
                                          uint32_t GroupCountZ) {
  if (auto Err = bindCommonDrawState(PSO))
    return Err;
  CB.CmdList->DispatchMesh(GroupCountX, GroupCountY, GroupCountZ);
  return llvm::Error::success();
}

void DXRenderEncoder::endEncodingImpl() {
  // State transitions
  for (offloadtest::Texture *Tex : Desc.ColorAttachments) {
    auto &DXTex = llvm::cast<DXTexture>(*Tex);
    if (DXTex.PreferredState != D3D12_RESOURCE_STATE_RENDER_TARGET)
      CB.addResourceTransition(DXTex.Resource.Get(),
                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                               DXTex.PreferredState);
  }
  if (Desc.DepthStencil) {
    auto &DXTex = llvm::cast<DXTexture>(*Desc.DepthStencil);
    if (DXTex.PreferredState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
      CB.addResourceTransition(DXTex.Resource.Get(),
                               D3D12_RESOURCE_STATE_DEPTH_WRITE,
                               DXTex.PreferredState);
  }

  popDebugGroup();
}
