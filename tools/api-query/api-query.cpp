//===- api-query.cpp - HLSL API Query Tool --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "API/Capabilities.h"
#include "API/Device.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/InitLLVM.h"


using namespace llvm;
using namespace offloadtest;

int main(int ArgC, char **ArgV) {
  InitLLVM const X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "GPU API Query Tool");

  ExitOnError const ExitOnErr("api-query: error: ");

  if (auto Err = Device::initialize())
    logAllUnhandledErrors(std::move(Err), errs(), "api-query: error: ");

  outs() << "Devices:\n";
  for (const auto &D : Device::devices()) {
    outs() << "- API: " << D->getAPIName() << "\n";
    outs() << "  Description: " << D->getDescription() << "\n";
    outs() << "  Features: \n";
    for (const auto &C : D->getCapabilities()) {
      outs() << "    ";
      C.second.print(outs());
      outs() << "\n";
    }
    D->printExtra(outs());
  }
  Device::uninitialize();
  return 0;
}
