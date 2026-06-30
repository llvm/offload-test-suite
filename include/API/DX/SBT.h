//===- DX/SBT.h - Offload API DX Shader Binding Table API -----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_SBT_H
#define OFFLOADTEST_API_DX_SBT_H

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

class DXShaderBindingTable : public offloadtest::ShaderBindingTable {
public:
  ComPtr<ID3D12Resource> Buffer;
  // Pre-built ranges for D3D12_DISPATCH_RAYS_DESC. Sizes are zero for
  // empty regions; raygen is a single record so it uses the no-stride
  // variant.
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE RayGenRange{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MissRange{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HitGroupRange{};
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CallableRange{};

  DXShaderBindingTable(ComPtr<ID3D12Resource> Buf,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE RG,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MS,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HG,
                       D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CL)
      : offloadtest::ShaderBindingTable(GPUAPI::DirectX), Buffer(Buf),
        RayGenRange(RG), MissRange(MS), HitGroupRange(HG), CallableRange(CL) {}

  static bool classof(const offloadtest::ShaderBindingTable *S) {
    return S->getAPI() == GPUAPI::DirectX;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_SBT_H