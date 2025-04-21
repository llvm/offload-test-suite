//===- Check.h - Functions for checking test results-------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_SUPPORT_CHECK_H
#define OFFLOADTEST_SUPPORT_CHECK_H

#include "Pipeline.h"

bool getResult(offloadtest::Result R);

bool testBufferExact(offloadtest::Buffer *B1, offloadtest::Buffer *B2);

bool testBufferFuzzy(offloadtest::Buffer *B1, offloadtest::Buffer *B2,
                     unsigned ULPT, offloadtest::DenormMode DM);

#endif // OFFLOADTEST_SUPPORT_CHECK_H