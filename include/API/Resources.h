//===- Resources.h - Offload API shared resource types --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_RESOURCES_H
#define OFFLOADTEST_API_RESOURCES_H

namespace offloadtest {

enum class MemoryLocation {
  GpuOnly,
  CpuToGpu,
  GpuToCpu,
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_RESOURCES_H
