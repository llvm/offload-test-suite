//===- DX/MemoryHeap.h - Offload API DX MemoryHeap API --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DX_MEMORYHEAP_H
#define OFFLOADTEST_API_DX_MEMORYHEAP_H

#include <d3d12.h>
#include <wrl/client.h>

// The windows headers define these macros which conflict with the C++ standard
// library. Undefining them before including any LLVM C++ code prevents errors.
#undef max
#undef min

#include "API/DX/Common.h"
#include "API/Device.h"

#include <cstdint>
#include <string>

namespace offloadtest {
using Microsoft::WRL::ComPtr;

class DXMemoryHeap : public offloadtest::MemoryHeap {
public:
  std::string Name;
  ComPtr<ID3D12Heap> Heap;

  DXMemoryHeap(ComPtr<ID3D12Heap> Heap, llvm::StringRef Name);

  static bool classof(const offloadtest::MemoryHeap *H);

  static llvm::Expected<std::unique_ptr<DXMemoryHeap>>
  create(ID3D12DeviceX *Device, llvm::StringRef Name, size_t SizeInBytes);
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DX_MEMORYHEAP_H
