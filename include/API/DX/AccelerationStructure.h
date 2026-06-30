//===- DX/AccelerationStructure.h - Offload API DX AccelStruct API --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_ACCELERATIONSTRUCTURE_H
#define OFFLOADTEST_API_DX_ACCELERATIONSTRUCTURE_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/AccelerationStructure.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXAccelerationStructure : public offloadtest::AccelerationStructure {
public:
  ComPtr<ID3D12Resource> Resource;
  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;

  DXAccelerationStructure(ComPtr<ID3D12Resource> Resource,
                          const AccelerationStructureSizes &Sizes)
      : offloadtest::AccelerationStructure(GPUAPI::DirectX, Sizes),
        Resource(Resource) {}

  D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress() const {
    return Resource->GetGPUVirtualAddress();
  }

  static bool classof(const offloadtest::AccelerationStructure *AS) {
    return AS->getAPI() == GPUAPI::DirectX;
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_ACCELERATIONSTRUCTURE_H
