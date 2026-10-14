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
#include "Support/HostInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/YAMLParser.h"

using namespace llvm;
using namespace offloadtest;

static void printQuotedYAMLScalar(raw_ostream &OS, StringRef Value) {
  OS << '"' << yaml::escape(Value, /*EscapePrintable=*/false) << '"';
}

int main(int ArgC, char **ArgV) {
  const InitLLVM X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "GPU API Query Tool");

  const ExitOnError ExitOnErr("api-query: error: ");

  const HostInfo Host = ExitOnErr(HostInfo::query());
  outs() << "Host:\n";
  outs() << "  Operating System: ";
  printQuotedYAMLScalar(outs(), Host.OperatingSystem);
  outs() << "\n  OS Version: ";
  printQuotedYAMLScalar(outs(), Host.OSVersion);
  outs() << "\n  Architecture: ";
  printQuotedYAMLScalar(outs(), Host.Architecture);
  outs() << '\n';

  const DeviceConfig Config;
  auto DevicesOrErr = initializeDevices(Config);
  if (!DevicesOrErr) {
    logAllUnhandledErrors(DevicesOrErr.takeError(), errs(),
                          "api-query: error: ");
    return 1;
  }
  auto Devices = std::move(*DevicesOrErr);

  outs() << "Devices:\n";
  for (const auto &D : Devices) {
    outs() << "- API: " << D->getAPIName() << "\n";
    outs() << "  Description: " << D->getDescription() << "\n";
    outs() << "  Driver: " << D->getDriverName() << "\n";
    // Driver version strings can be vendor-specific freeform text that
    // may contain ':' characters (e.g. Qualcomm's Vulkan driverInfo is
    // "Driver Build: ..."). Emit as a double-quoted YAML scalar so
    // lit.cfg.py's yaml.safe_load() can parse the output.
    outs() << "  Driver Version: ";
    printQuotedYAMLScalar(outs(), D->getDriverVersion());
    outs() << '\n';
    outs() << "  GPUGeneration: " << D->getGPUGeneration() << "\n";
    outs() << "  FamilyPrefix: " << format_hex(D->getFamilyPrefix(), 6) << "\n";
    outs() << "  Features: \n";
    for (const auto &C : D->getCapabilities()) {
      outs() << "    ";
      C.second.print(outs());
      outs() << "\n";
    }
    D->printExtra(outs());
  }

  return 0;
}
