//===- Capabilities.h - Offload API Capabilities API ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_CAPABILITIES_H
#define OFFLOADTEST_API_CAPABILITIES_H

#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>

namespace offloadtest {

class CapabilityValueBase {
public:
  virtual ~CapabilityValueBase() = default;

  // Returns the class ID for this type.
  static const void *classID() { return &ID; }

  // Check whether this instance is a subclass of the class identified by
  // ClassID.
  virtual bool isA(const void *const ClassID) const {
    return ClassID == classID();
  }

  // Check whether this instance is a subclass of CapabilityValueT.
  template <typename CapabilityValueT> bool isA() const {
    return isA(CapabilityValueT::classID());
  }

  virtual std::string toString() const = 0;

private:
  virtual void anchor();
  static char ID;
};

template <typename ThisT, typename ValueT>
class CapabilityValue : public CapabilityValueBase {
public:
  CapabilityValue(ValueT V) : Value(V) {}

  static const void *classID() { return &ThisT::ID; }

  bool isA(const void *const ClassID) const override {
    return ClassID == classID() || CapabilityValueBase::isA(ClassID);
  }

  ValueT getValue() const { return Value; }

  std::string toString() const override { return ThisT::valueToString(Value); }

protected:
  ValueT Value;
};

class CapabilityValueBool : public CapabilityValue<CapabilityValueBool, bool> {
public:
  CapabilityValueBool(bool V) : CapabilityValue<CapabilityValueBool, bool>(V) {}
  static char ID;
  static std::string valueToString(bool V) {
    return std::string(V ? "true" : "false");
  }
};

class CapabilityValueUnsigned
    : public CapabilityValue<CapabilityValueUnsigned, uint32_t> {
public:
  CapabilityValueUnsigned(uint32_t V)
      : CapabilityValue<CapabilityValueUnsigned, uint32_t>(V) {}
  static char ID;
  static std::string valueToString(uint32_t V) { return std::to_string(V); }
};

namespace detail {
template <typename T> struct CapabilityTypeHelper {};

template <> struct CapabilityTypeHelper<bool> {
  using Capability = CapabilityValueBool;
};

template <> struct CapabilityTypeHelper<uint32_t> {
  using Capability = CapabilityValueUnsigned;
};
} // namespace detail

template <typename T> struct CapabilityPrinter;

template <typename T>
using CapabilityType = typename detail::CapabilityTypeHelper<T>::Capability;

template <typename T>
class CapabilityValueEnum : public CapabilityValue<CapabilityValueEnum<T>, T> {
public:
  CapabilityValueEnum(T V) : CapabilityValue<CapabilityValueEnum<T>, T>(V) {}
  static char ID;
  static std::string valueToString(T V) {
    return CapabilityPrinter<T>::toString(V);
  }
};

class Capability {
  std::string Name;
  std::shared_ptr<CapabilityValueBase> CapabilityData;

public:
  Capability(llvm::StringRef N, std::shared_ptr<CapabilityValueBase> &&D)
      : Name(N), CapabilityData(D) {}

  ~Capability() = default;
  Capability(const Capability &) = default;

  llvm::StringRef getName() const { return Name; }

  const CapabilityValueBase *getData() const { return CapabilityData.get(); }

  std::string getValueSting() const { return CapabilityData->toString(); }

  void print(llvm::raw_ostream &OS) const {
    OS << getName() << ": " << getValueSting();
  }
};

template <typename T>
std::enable_if_t<std::is_enum_v<T>, Capability>
makeCapability(llvm::StringRef N, T Value) {
  return Capability(N, std::make_shared<CapabilityValueEnum<T>>(Value));
}

template <typename T>
std::enable_if_t<!std::is_enum_v<T>, Capability>
makeCapability(llvm::StringRef N, T Value) {
  return Capability(N, std::make_shared<CapabilityType<T>>(Value));
}

using Capabilities = llvm::StringMap<Capability>;

} // namespace offloadtest

#endif // OFFLOADTEST_API_CAPABILITIES_H
