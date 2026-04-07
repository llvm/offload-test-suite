//===- MetalIRConverter.h - Metal IR Converter ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_MTL_METALIRCONVERTER_H
#define OFFLOADTEST_API_MTL_METALIRCONVERTER_H

#pragma push_macro("IR_RUNTIME_METALCPP")
#define IR_RUNTIME_METALCPP
#include "Metal/Metal.hpp"
#include "metal_irconverter.h"
#include "metal_irconverter_runtime.h"
#pragma pop_macro("IR_RUNTIME_METALCPP")

#endif // OFFLOADTEST_API_MTL_METALIRCONVERTER_H
