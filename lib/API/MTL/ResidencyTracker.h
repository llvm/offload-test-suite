//===- ResidencyTracker.h - Metal Residency Tracker -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_MTL_RESIDENCYTRACKER_H
#define OFFLOADTEST_API_MTL_RESIDENCYTRACKER_H

#include "llvm/Support/Error.h"
#include <memory>
#include <mutex>
#include <utility>

// Forward declarations
namespace MTL {
class Device;
class ResidencySet;
} // namespace MTL

namespace offloadtest {
struct MetalResidencyTracker {
  std::mutex ResidencyMutex;
  MTL::ResidencySet *ResidencySet = nullptr;

  template <typename ClosureT> decltype(auto) withLock(ClosureT &&Closure) {
    std::lock_guard<std::mutex> Lock(ResidencyMutex);
    return std::forward<ClosureT>(Closure)(ResidencySet);
  }

  static llvm::Expected<std::shared_ptr<MetalResidencyTracker>>
  create(MTL::Device *Device);

  MetalResidencyTracker() = default;
  MetalResidencyTracker(const MetalResidencyTracker &) = delete;
  MetalResidencyTracker(MetalResidencyTracker &&) = delete;
  MetalResidencyTracker &operator=(const MetalResidencyTracker &) = delete;
  MetalResidencyTracker &operator=(MetalResidencyTracker &&) = delete;
  ~MetalResidencyTracker();
};
} // namespace offloadtest

#endif // OFFLOADTEST_API_MTL_RESIDENCYTRACKER_H
