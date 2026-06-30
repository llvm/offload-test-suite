//===- DX/PipelineState.h - Offload API DX PipelineState API --------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_PIPELINESTATE_H
#define OFFLOADTEST_API_DX_PIPELINESTATE_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/Device.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

enum class RootParameterType : uint32_t {
  DescriptorTable = 0,
  SamplerTable,
  Constant,
  CBV,
  SRV,
  UAV,
};

struct RootSignatureLayout {
  RootParameterType ParameterType : 3;
  uint32_t Count : 29;

  RootSignatureLayout(RootParameterType ParameterType, uint32_t Count)
      : ParameterType(ParameterType), Count(Count) {}
  RootSignatureLayout() = delete;
};

struct DescriptorCountPair {
  uint32_t DescriptorCount;
  uint32_t SamplerCount;
};

struct DescriptorSetsLayout {
  llvm::SmallVector<RootSignatureLayout> RSigLayout;
  llvm::SmallVector<DescriptorCountPair> Sets;
};

class DXPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  ComPtr<ID3D12RootSignature> RootSig;
  DescriptorSetsLayout Layout;

  ComPtr<ID3D12PipelineState> PSO;
  // Only set for graphics pipelines.
  std::optional<D3D_PRIMITIVE_TOPOLOGY> Topology;
  // True for pipelines created via createPipelineRT — used by SBT / dispatch
  // code to safely downcast to DXRayTracingPipelineState (parallel to
  // VulkanPipelineState::IsRayTracing).
  bool IsRayTracing = false;

  DXPipelineState(llvm::StringRef Name, ComPtr<ID3D12RootSignature> RootSig,
                  DescriptorSetsLayout Layout, ComPtr<ID3D12PipelineState> PSO,
                  std::optional<D3D_PRIMITIVE_TOPOLOGY> Topology,
                  bool IsRT = false);

  static bool classof(const offloadtest::PipelineState *B);
};

/// RT pipeline state: holds the ID3D12StateObject + cached
/// ID3D12StateObjectProperties for SBT identifier queries plus a
/// shader-name → identifier-pointer map. The `void *` identifiers are
/// owned by Properties — keep it alive for the SBT's lifetime.
class DXRayTracingPipelineState : public DXPipelineState {
public:
  ComPtr<ID3D12StateObject> StateObject;
  ComPtr<ID3D12StateObjectProperties> Properties;
  llvm::StringMap<const void *> ShaderIdentifiers;

  DXRayTracingPipelineState(llvm::StringRef Name,
                            ComPtr<ID3D12RootSignature> RootSig,
                            DescriptorSetsLayout Layout,
                            ComPtr<ID3D12StateObject> SO,
                            ComPtr<ID3D12StateObjectProperties> Props);

  static bool classof(const offloadtest::PipelineState *B);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_PIPELINESTATE_H