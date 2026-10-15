//===- HostInfo.h - Host operating system information ----------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_HOST_INFO_H
#define OFFLOADTEST_HOST_INFO_H

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"

namespace offloadtest {

struct HostInfo {
  llvm::SmallString<16> OperatingSystem;
  llvm::SmallString<16> OSVersion;
  llvm::SmallString<8> Architecture;

  HostInfo(llvm::StringRef OperatingSystem, llvm::StringRef OSVersion,
           llvm::StringRef Architecture)
      : OperatingSystem(OperatingSystem), OSVersion(OSVersion),
        Architecture(Architecture) {}

  static llvm::Expected<HostInfo> query();
};

} // namespace offloadtest

#endif // OFFLOADTEST_HOST_INFO_H
