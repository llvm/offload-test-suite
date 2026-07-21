//===- Util.h - Internal shared helper functions --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Helper functions that are shared between the various backends, but which
// should not be exposed to the user of the graphics layer.
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_UTIL_H
#define OFFLOADTEST_API_UTIL_H

#include "API/API.h"
#include "API/CommandBuffer.h"

#include <cstdint>

namespace offloadtest {

llvm::Error findAndValidateRenderPassTextureSize(const RenderPassBeginDesc &,
                                                 uint32_t *OutWidth,
                                                 uint32_t *OutHeight);

enum class IntelGpuEra { UnknownOrLegacy, Gen7_to_10, Gen11_to_14_and_Xe };

IntelGpuEra getIntelGpuEra(uint16_t DeviceId);
} // namespace offloadtest

#endif // OFFLOADTEST_API_UTIL_H
