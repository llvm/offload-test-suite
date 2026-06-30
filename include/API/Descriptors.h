//===- Descriptors.h - Offload API Buffer ---------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DESCRIPTORS_H
#define OFFLOADTEST_API_DESCRIPTORS_H

#include "API/API.h"

#include "llvm/ADT/ArrayRef.h"

namespace offloadtest {

class AccelerationStructure;
class Buffer;
class Sampler;
class Texture;

struct VKBind {
  uint32_t Binding;
  uint32_t CounterBinding;
};

class DescriptorPool {
  GPUAPI API;

public:
  virtual ~DescriptorPool() {} // TOOD(manon): Move this def to a .cpp file
  GPUAPI getAPI() const { return API; }

  virtual void reset() = 0;

protected:
  explicit DescriptorPool(GPUAPI API) : API(API) {}
};

class DescriptorSets {
  GPUAPI API;

public:
  virtual ~DescriptorSets() {} // TOOD(manon): Move this def to a .cpp file

  GPUAPI getAPI() const { return API; }

protected:
  explicit DescriptorSets(GPUAPI API) : API(API) {}
};

class DescriptorSetsBuilder {
  GPUAPI API;

public:
  virtual ~DescriptorSetsBuilder() {} // TODO(manon): move to .cpp file

  GPUAPI getAPI() const { return API; }

  virtual DescriptorSetsBuilder &
  constant(uint32_t SetIndex, llvm::ArrayRef<const Buffer *>, VKBind) = 0;

  virtual DescriptorSetsBuilder &
  read(uint32_t SetIndex, llvm::ArrayRef<const Buffer *>, VKBind) = 0;
  virtual DescriptorSetsBuilder &
  read(uint32_t SetIndex, llvm::ArrayRef<const Texture *>, VKBind) = 0;
  virtual DescriptorSetsBuilder &
  read(uint32_t SetIndex, llvm::ArrayRef<const AccelerationStructure *>,
       VKBind) = 0;

  virtual DescriptorSetsBuilder &
  write(uint32_t SetIndex, llvm::ArrayRef<const Buffer *>, VKBind) = 0;
  virtual DescriptorSetsBuilder &
  write(uint32_t SetIndex, llvm::ArrayRef<const Texture *>, VKBind) = 0;

  virtual DescriptorSetsBuilder &
  sampler(uint32_t SetIndex, llvm::ArrayRef<const Sampler *>, VKBind) = 0;

  virtual std::unique_ptr<DescriptorSets> build() = 0;

protected:
  explicit DescriptorSetsBuilder(GPUAPI API) : API(API) {}
};
} // namespace offloadtest

#endif // OFFLOADTEST_API_DESCRIPTORS_H
