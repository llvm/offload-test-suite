//===- DX/Texture.h - Offload API DX Texture API --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_TEXTURE_H
#define OFFLOADTEST_API_DX_TEXTURE_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/Texture.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXTexture : public offloadtest::Texture {
public:
  ComPtr<ID3D12Resource> Resource;

  // Contract: If a command on the command buffer needs a resource to be in a
  // different state it should always transition it back to the PreferredState
  // afterwards. The PreferredState is the state of the most common use case for
  // that resource. This allows us to do state transitions without state
  // tracking.
  D3D12_RESOURCE_STATES PreferredState;

  // Optional descriptors, depending on Desc.Usage.
  // A zero ptr means no descriptor was created for that view type.
  D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle = {};
  D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle = {};

  std::string Name;
  TextureCreateDesc Desc;

  DXTexture(ComPtr<ID3D12Resource> Resource, llvm::StringRef Name,
            TextureCreateDesc Desc, D3D12_RESOURCE_STATES PreferredState,
            D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle);

  TileShape querySparseTileShape(const Device &Dev) const override;

  const TextureCreateDesc &getDesc() const override;

  static bool classof(const offloadtest::Texture *T);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_TEXTURE_H