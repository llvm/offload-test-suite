//===- DX/Common.h - Offload API DX API -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_COMMON_H
#define OFFLOADTEST_API_DX_COMMON_H

#include <d3d12.h>
#include <d3dx12.h>
#include <dxcore.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

namespace offloadtest {
using ID3D12DeviceX = ID3D12Device5;
using ID3D12GraphicsCommandListX = ID3D12GraphicsCommandList6;
using Microsoft::WRL::ComPtr;
} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_COMMON_H
