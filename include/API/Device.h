//===- Device.h - Offload API Device API ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_DEVICE_H
#define OFFLOADTEST_API_DEVICE_H

#include "API/API.h"
#include "API/Capabilities.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/iterator_range.h"

#include <memory>
#include <string>

namespace llvm {
class raw_ostream;
}

namespace offloadtest {

struct Pipeline;

class Device {
protected:
  std::string Description;

public:
  virtual const Capabilities &getCapabilities() = 0;
  virtual llvm::StringRef getAPIName() const = 0;
  virtual GPUAPI getAPI() const = 0;
  virtual llvm::Error executeProgram(llvm::StringRef Program, Pipeline &P) = 0;
  virtual void printExtra(llvm::raw_ostream &OS) {}

  virtual ~Device() = 0;

  llvm::StringRef getDescription() const { return Description; }

  static void registerDevice(std::shared_ptr<Device> D);
  static llvm::Error initialize();
  static void uninitialize();

  using DeviceArray = llvm::SmallVector<std::shared_ptr<Device>>;
  using DeviceIterator = DeviceArray::iterator;
  using DeviceRange = llvm::iterator_range<DeviceIterator>;

  static DeviceIterator begin();
  static DeviceIterator end();
  static inline DeviceRange devices() { return DeviceRange(begin(), end()); }
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_DEVICE_H
