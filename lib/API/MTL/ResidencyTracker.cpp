//===- MTL/ResidencyTracker.cpp - Metal Residency Tracker ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ResidencyTracker.h"

#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"

namespace offloadtest {

llvm::Expected<std::shared_ptr<MetalResidencyTracker>>
MetalResidencyTracker::create(MTL::Device *Device) {
  std::shared_ptr<MetalResidencyTracker> Tracker =
      std::make_shared<MetalResidencyTracker>();

  MTL::ResidencySetDescriptor *Desc =
      MTL::ResidencySetDescriptor::alloc()->init();
  Desc->setInitialCapacity(128);

  NS::Error *Err = nullptr;

  Tracker->ResidencySet = Device->newResidencySet(Desc, &Err);

  Desc->release();

  if (!Tracker->ResidencySet)
    return llvm::createStringError("Failed to create a metal residency set.");

  return Tracker;
}

MetalResidencyTracker::~MetalResidencyTracker() {
  if (ResidencySet)
    ResidencySet->release();
}

} // namespace offloadtest
