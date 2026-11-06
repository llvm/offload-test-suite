//===- HostInfo.cpp - Host operating system information -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Support/HostInfo.h"

#include "llvm/Support/Errc.h"

#ifdef _WIN32
#include "llvm/Support/raw_ostream.h"
#include <Windows.h>
#include <winternl.h>
#else
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include <sys/utsname.h>
#include <utility>

#ifdef OFFLOADTEST_HAVE_SYSCTLBYNAME
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#endif

namespace offloadtest {

#ifdef _WIN32

#ifdef __clang__
// GetProcAddress returns an untyped function pointer, which we need to cast to
// the appropriate function type.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
#endif
using RtlGetVersionFn = NTSTATUS(WINAPI *)(PRTL_OSVERSIONINFOW);

llvm::Expected<HostInfo> HostInfo::query() {
  HMODULE NtDll = GetModuleHandleW(L"ntdll.dll");
  auto RtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
      NtDll ? GetProcAddress(NtDll, "RtlGetVersion") : nullptr);
  if (!RtlGetVersion)
    return llvm::createStringError(llvm::errc::not_supported,
                                   "RtlGetVersion is unavailable");

  RTL_OSVERSIONINFOW Version = {};
  Version.dwOSVersionInfoSize = sizeof(Version);
  if (RtlGetVersion(&Version) != 0)
    return llvm::createStringError(llvm::errc::io_error,
                                   "RtlGetVersion failed");

  SYSTEM_INFO SystemInfo = {};
  GetNativeSystemInfo(&SystemInfo);
  const char *Architecture = "Unknown";
  switch (SystemInfo.wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_AMD64:
    Architecture = "x86_64";
    break;
  case PROCESSOR_ARCHITECTURE_ARM64:
    Architecture = "arm64";
    break;
  case PROCESSOR_ARCHITECTURE_INTEL:
    Architecture = "x86";
    break;
  }

  HostInfo Info{"Windows", "", Architecture};
  llvm::raw_svector_ostream VersionStream(Info.OSVersion);
  VersionStream << Version.dwMajorVersion << '.' << Version.dwMinorVersion
                << '.' << Version.dwBuildNumber;
  return Info;
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#else

static llvm::StringRef getReleaseValue(llvm::StringRef Contents,
                                       llvm::StringRef Key) {
  while (!Contents.empty()) {
    auto [Line, Rest] = Contents.split('\n');
    Contents = Rest;
    auto [LineKey, Value] = Line.split('=');
    if (LineKey != Key)
      continue;
    if (Value.size() >= 2 && Value.front() == '"' && Value.back() == '"')
      Value = Value.drop_front().drop_back();
    return Value;
  }
  return {};
}

#ifdef OFFLOADTEST_HAVE_SYSCTLBYNAME
static llvm::SmallString<16> querySysctl(const char *Name) {
  size_t Size = 0;
  if (sysctlbyname(Name, nullptr, &Size, nullptr, 0) != 0 || Size == 0)
    return {};

  llvm::SmallString<16> Value;
  Value.resize_for_overwrite(Size);
  if (sysctlbyname(Name, Value.data(), &Size, nullptr, 0) != 0)
    return {};
  if (Size > 0 && Value[Size - 1] == '\0')
    --Size;
  Value.resize(Size);
  return Value;
}
#endif

llvm::Expected<HostInfo> HostInfo::query() {
  struct utsname Uname = {};
  if (uname(&Uname) != 0)
    return llvm::createStringError(llvm::errc::io_error, "uname failed");

  HostInfo Info{Uname.sysname, Uname.release, Uname.machine};
  if (auto Release = llvm::MemoryBuffer::getFile("/etc/os-release")) {
    llvm::StringRef Contents = (*Release)->getBuffer();
    if (llvm::StringRef Name = getReleaseValue(Contents, "NAME"); !Name.empty())
      Info.OperatingSystem.assign(Name);
    if (llvm::StringRef Version = getReleaseValue(Contents, "VERSION_ID");
        !Version.empty())
      Info.OSVersion.assign(Version);
  }

#ifdef OFFLOADTEST_HAVE_SYSCTLBYNAME
  if (llvm::SmallString<16> Version = querySysctl("kern.osproductversion");
      !Version.empty())
    Info.OSVersion = std::move(Version);
#endif

  return Info;
}

#endif

} // namespace offloadtest
