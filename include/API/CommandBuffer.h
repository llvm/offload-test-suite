//===- CommandBuffer.h - Offload Command Buffer API -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the CommandBuffer base class for recording and submitting GPU work.
// Each backend (DirectX, Vulkan, Metal) provides a concrete subclass that
// wraps the native command recording objects. LLVM-style RTTI is provided for
// downcasting to the backend-specific type.
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_COMMANDBUFFER_H
#define OFFLOADTEST_API_COMMANDBUFFER_H

#include "API/API.h"

#include <cassert>

namespace offloadtest {

class CommandBuffer {
  GPUAPI API;

public:
  explicit CommandBuffer(GPUAPI API) : API(API) {}
  virtual ~CommandBuffer();
  CommandBuffer(const CommandBuffer &) = delete;
  CommandBuffer &operator=(const CommandBuffer &) = delete;

  GPUAPI getAPI() const { return API; }

  template <typename T> T &as() {
    assert(API == T::BackendAPI && "CommandBuffer backend mismatch");
    return static_cast<T &>(*this);
  }
  template <typename T> const T &as() const {
    assert(API == T::BackendAPI && "CommandBuffer backend mismatch");
    return static_cast<const T &>(*this);
  }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_COMMANDBUFFER_H
