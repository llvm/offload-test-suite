
#include <d3dx12.h>

#include "API/DX/CommandBuffer.h"
#include "API/DX/Descriptors.h"
#include "API/DX/Encoder.h"
#include "API/DX/RenderPass.h"
#include "API/DX/Texture.h"
#include "API/Util.h"

#include "Support/WinError.h"

#include "llvm/Support/Casting.h"

using namespace offloadtest;

llvm::Expected<std::unique_ptr<DXCommandBuffer>>
DXCommandBuffer::create(ComPtr<ID3D12DeviceX> Device) {
  auto CB = std::unique_ptr<DXCommandBuffer>(new DXCommandBuffer());
  if (auto Err = HR::toError(
          Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                         IID_PPV_ARGS(&CB->Allocator)),
          "Failed to create command allocator."))
    return Err;
  if (auto Err = HR::toError(
          Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                    CB->Allocator.Get(), nullptr,
                                    IID_PPV_ARGS(&CB->CmdList)),
          "Failed to create command list."))
    return Err;
  return CB;
}

bool DXCommandBuffer::classof(const CommandBuffer *CB) {
  return CB->getKind() == GPUAPI::DirectX;
}

void DXCommandBuffer::addPendingUAVBarrier() { PendingUAVBarrier = true; }
void DXCommandBuffer::addResourceTransition(ID3D12Resource *Resource,
                                            D3D12_RESOURCE_STATES StateBefore,
                                            D3D12_RESOURCE_STATES StateAfter) {

  for (auto TransIt = PendingTransitions.begin(),
            End = PendingTransitions.end();
       TransIt != End; ++TransIt) {
    if (TransIt->Transition.pResource != Resource)
      continue;

    assert(StateBefore == TransIt->Transition.StateAfter);

    if (StateAfter == TransIt->Transition.StateBefore) {
      // We actually need the resource to be in the original state!
      // Remove the transition from the list of Pending Transitions.
      PendingTransitions.erase(TransIt);

      // NOTE: This could cause an execution barrier issue since legacy
      // barriers don't allow us to express execution barriers, cache
      // flushes, and layout transitions separately.
      // As a workaround we add a UAV barrier so an execution barrier and
      // cache flush is inserted.
      addPendingUAVBarrier();
    } else {
      TransIt->Transition.StateAfter = StateAfter;
    }

    return;
  }

  PendingTransitions.push_back(
      CD3DX12_RESOURCE_BARRIER::Transition(Resource, StateBefore, StateAfter));
}

void DXCommandBuffer::flushBarrier() {

  if (PendingUAVBarrier) {
    PendingTransitions.push_back(CD3DX12_RESOURCE_BARRIER::UAV(nullptr));
    PendingUAVBarrier = false;
  }

  if (!PendingTransitions.empty()) {
    CmdList->ResourceBarrier(PendingTransitions.size(),
                             PendingTransitions.data());
    PendingTransitions.clear();
  }
}

void DXCommandBuffer::bindPool(const DescriptorPool &Pool) {
  const DXDescriptorPool &PoolDX = llvm::cast<DXDescriptorPool>(Pool);
  ID3D12DescriptorHeap *const Heaps[] = {PoolDX.CSUHeap.Get(),
                                         PoolDX.SamplerHeap.Get()};
  CmdList->SetDescriptorHeaps(2, Heaps);
}

llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
DXCommandBuffer::createComputeEncoder() {
  auto Enc = std::make_unique<DXComputeEncoder>(*this);
  Enc->pushDebugGroup("ComputeEncoder");
  return Enc;
}

llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
DXCommandBuffer::createRenderEncoder(
    const offloadtest::RenderPassBeginDesc &Desc) {
  // The pass carries format / load / store policy; the begin desc supplies
  // the actual textures. Walk both in lockstep.
  if (!Desc.Pass)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc is missing its RenderPass.");
  auto &DXPass = llvm::cast<DXRenderPass>(*Desc.Pass);
  const offloadtest::RenderPassDesc &PassDesc = DXPass.Desc;

  if (Desc.ColorAttachments.size() != PassDesc.ColorAttachments.size())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc color attachment count does not match its "
        "RenderPass.");
  if (PassDesc.DepthStencil.has_value() != (Desc.DepthStencil != nullptr))
    return llvm::createStringError(std::errc::invalid_argument,
                                   "RenderPassBeginDesc depth-stencil "
                                   "presence does not match its RenderPass.");

  if (auto Err = findAndValidateRenderPassTextureSize(Desc, nullptr, nullptr))
    return Err;

  // Validate attachments and gather the RTV / DSV CPU handles. RT and DSV
  // descriptors are owned by the textures themselves; this just collects
  // them for OMSetRenderTargets.
  llvm::SmallVector<DXTexture *, 8> RTTextures;
  llvm::SmallVector<D3D12_CPU_DESCRIPTOR_HANDLE, 8> RTVHandles;
  RTTextures.reserve(Desc.ColorAttachments.size());
  RTVHandles.reserve(Desc.ColorAttachments.size());
  for (offloadtest::Texture *Tex : Desc.ColorAttachments) {
    if (!Tex)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RenderPassBeginDesc has a null color attachment texture.");
    auto &DXTex = llvm::cast<DXTexture>(*Tex);
    if (DXTex.RTVHandle.ptr == 0)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Color attachment texture was not created with RenderTarget usage.");
    RTTextures.push_back(&DXTex);
    RTVHandles.push_back(DXTex.RTVHandle);
  }

  DXTexture *DSTexture = nullptr;
  D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
  if (Desc.DepthStencil) {
    auto &DXDS = llvm::cast<DXTexture>(*Desc.DepthStencil);
    if (DXDS.DSVHandle.ptr == 0)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Depth-stencil texture was not created with DepthStencil usage.");
    DSTexture = &DXDS;
    DSVHandle = DXDS.DSVHandle;
  }

  // State transitions
  for (offloadtest::Texture *Tex : Desc.ColorAttachments) {
    auto &DXTex = llvm::cast<DXTexture>(*Tex);
    if (DXTex.PreferredState != D3D12_RESOURCE_STATE_RENDER_TARGET)
      this->addResourceTransition(DXTex.Resource.Get(), DXTex.PreferredState,
                                  D3D12_RESOURCE_STATE_RENDER_TARGET);
  }
  if (Desc.DepthStencil) {
    auto &DXTex = llvm::cast<DXTexture>(*Desc.DepthStencil);
    if (DXTex.PreferredState != D3D12_RESOURCE_STATE_DEPTH_WRITE)
      this->addResourceTransition(DXTex.Resource.Get(), DXTex.PreferredState,
                                  D3D12_RESOURCE_STATE_DEPTH_WRITE);
  }

  this->flushBarrier();

  CmdList->OMSetRenderTargets(static_cast<UINT>(RTVHandles.size()),
                              RTVHandles.data(),
                              /*RTsSingleHandleToDescriptorRange=*/false,
                              Desc.DepthStencil ? &DSVHandle : nullptr);

  for (size_t I = 0; I < PassDesc.ColorAttachments.size(); ++I) {
    if (PassDesc.ColorAttachments[I].Load != offloadtest::LoadAction::Clear)
      continue;
    if (!RTTextures[I]->Desc.OptimizedClearValue)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "LoadAction::Clear requires the render target to have been "
          "created with an OptimizedClearValue.");
    const auto *CV =
        std::get_if<ClearColor>(&*RTTextures[I]->Desc.OptimizedClearValue);
    assert(CV && "RenderTarget OptimizedClearValue must be a ClearColor");
    const float ClearArr[4] = {CV->R, CV->G, CV->B, CV->A};
    CmdList->ClearRenderTargetView(RTVHandles[I], ClearArr, 0, nullptr);
  }
  if (PassDesc.DepthStencil) {
    D3D12_CLEAR_FLAGS Flags = static_cast<D3D12_CLEAR_FLAGS>(0);
    if (PassDesc.DepthStencil->DepthLoad == offloadtest::LoadAction::Clear)
      Flags |= D3D12_CLEAR_FLAG_DEPTH;
    if (PassDesc.DepthStencil->StencilLoad == offloadtest::LoadAction::Clear)
      Flags |= D3D12_CLEAR_FLAG_STENCIL;
    if (Flags != 0) {
      if (!DSTexture->Desc.OptimizedClearValue)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "LoadAction::Clear requires the depth-stencil texture to have "
            "been created with an OptimizedClearValue.");
      const auto *CV =
          std::get_if<ClearDepthStencil>(&*DSTexture->Desc.OptimizedClearValue);
      assert(CV &&
             "DepthStencil OptimizedClearValue must be a ClearDepthStencil");
      CmdList->ClearDepthStencilView(DSVHandle, Flags, CV->Depth, CV->Stencil,
                                     0, nullptr);
    }
  }

  auto Enc = std::make_unique<DXRenderEncoder>(*this, Desc);
  Enc->pushDebugGroup("RenderEncoder");
  return Enc;
}
