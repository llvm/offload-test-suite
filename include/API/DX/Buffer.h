//===- DX/Buffer.h - Offload API DX Buffer API ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_BUFFER_H
#define OFFLOADTEST_API_DX_BUFFER_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/Buffer.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXBuffer : public offloadtest::Buffer {
public:
  ComPtr<ID3D12Resource> Buffer;
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;
  uint64_t CounterOffsetInBytes;

  // Contract: If a command on the command buffer needs a resource to be in a
  // different state it should always transition it back to the PreferredState
  // afterwards. The PreferredState is the state of the most common use case for
  // that resource. This allows us to do state transitions without state
  // tracking.
  D3D12_RESOURCE_STATES PreferredState;

  D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle;
  D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle;
  D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle;

  DXBuffer(ComPtr<ID3D12Resource> Buffer, llvm::StringRef Name,
           BufferCreateDesc Desc, size_t SizeInBytes,
           uint64_t CounterOffsetInBytes, D3D12_RESOURCE_STATES PreferredState,
           D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,
           D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle,
           D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle);
  DXBuffer(const DXBuffer &) = delete;
  DXBuffer(DXBuffer &&) = delete;
  DXBuffer &operator=(const DXBuffer &) = delete;
  DXBuffer &operator=(DXBuffer &&) = delete;

  size_t getSizeInBytes() const override;

  size_t querySparseTileSizeInBytes(const Device & /*Dev*/) const override;

  llvm::Expected<void *> map() override;

  void unmap() override;

  const BufferCreateDesc &getDesc() const override;

  static bool classof(const offloadtest::Buffer *B);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_BUFFER_H