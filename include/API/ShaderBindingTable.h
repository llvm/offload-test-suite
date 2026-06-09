//===- ShaderBindingTable.h - Offload RT Shader Binding Table -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_SHADERBINDINGTABLE_H
#define OFFLOADTEST_API_SHADERBINDINGTABLE_H

#include "API/API.h"

#include <cstdint>

namespace offloadtest {

struct ShaderBindingTableDesc;

/// Runtime shader binding table built from a RayTracing PipelineState plus a
/// ShaderBindingTableDesc. Concrete subclasses (one per backend) hold the
/// device-side records and any address ranges needed by the backend's
/// DispatchRays call.
class ShaderBindingTable {
  GPUAPI API;

public:
  virtual ~ShaderBindingTable();
  ShaderBindingTable(const ShaderBindingTable &) = delete;
  ShaderBindingTable &operator=(const ShaderBindingTable &) = delete;

  GPUAPI getAPI() const { return API; }

protected:
  explicit ShaderBindingTable(GPUAPI API) : API(API) {}
};

/// Per-region SBT layout numbers.
///
/// Every backend lays an SBT out as four concatenated regions (raygen, miss,
/// hit-group, callable). Within a region every record is
/// `[shader-identifier][LocalRootData][padding-to-stride]`, where stride is
/// `align(identifierSize + max-LocalRootData-in-region, RecordAlign)` and
/// the region itself is aligned to `BaseAlign`. The numbers match the
/// alignment rules of both D3D12
/// (`D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES` / `…_RECORD_BYTE_ALIGNMENT` /
/// `…_TABLE_BYTE_ALIGNMENT`) and Vulkan
/// (`shaderGroupHandleSize` / `shaderGroupHandleAlignment` /
/// `shaderGroupBaseAlignment`); backend-specific constants are passed in.
struct SBTRegionLayout {
  uint32_t Stride = 0;
  uint32_t Size = 0;
  uint32_t Offset = 0; // byte offset from the start of the SBT buffer
};

struct SBTLayout {
  SBTRegionLayout RayGen;
  SBTRegionLayout Miss;
  SBTRegionLayout HitGroup;
  SBTRegionLayout Callable;
  uint32_t TotalSize = 0;
};

/// Compute the per-region layout for an SBT description.
///
/// \p IdentifierSize is the size of one shader identifier (32 bytes on
/// D3D12; `shaderGroupHandleSize` on Vulkan).
/// \p RecordAlign is the per-record alignment (D3D12: 32 bytes; Vulkan:
/// `shaderGroupHandleAlignment`).
/// \p BaseAlign is the per-region alignment (D3D12: 64 bytes; Vulkan:
/// `shaderGroupBaseAlignment`).
SBTLayout computeSBTLayout(uint32_t IdentifierSize, uint32_t RecordAlign,
                           uint32_t BaseAlign,
                           const ShaderBindingTableDesc &Desc);

} // namespace offloadtest

#endif // OFFLOADTEST_API_SHADERBINDINGTABLE_H
