#include "API/DX/PipelineState.h"

using namespace offloadtest;

DXPipelineState::DXPipelineState(llvm::StringRef Name,
                                 ComPtr<ID3D12RootSignature> RootSig,
                                 DescriptorSetsLayout Layout,
                                 ComPtr<ID3D12PipelineState> PSO,
                                 std::optional<D3D_PRIMITIVE_TOPOLOGY> Topology,
                                 bool IsRT)
    : offloadtest::PipelineState(GPUAPI::DirectX), Name(Name), RootSig(RootSig),
      Layout(std::move(Layout)), PSO(PSO), Topology(Topology),
      IsRayTracing(IsRT) {}

bool DXPipelineState::classof(const offloadtest::PipelineState *B) {
  return B->getAPI() == GPUAPI::DirectX;
}

DXRayTracingPipelineState::DXRayTracingPipelineState(
    llvm::StringRef Name, ComPtr<ID3D12RootSignature> RootSig,
    DescriptorSetsLayout Layout, ComPtr<ID3D12StateObject> SO,
    ComPtr<ID3D12StateObjectProperties> Props)
    : DXPipelineState(Name, RootSig, std::move(Layout), /*PSO=*/nullptr,
                      std::nullopt,
                      /*IsRT=*/true),
      StateObject(SO), Properties(Props) {}

bool DXRayTracingPipelineState::classof(const offloadtest::PipelineState *B) {
  if (B->getAPI() != GPUAPI::DirectX)
    return false;
  return static_cast<const DXPipelineState *>(B)->IsRayTracing;
}
