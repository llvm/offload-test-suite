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

/// verifies an offload test Result
/// Calls the test, corresponding to the Rule specified in the Result,
/// On the Actual and Expected Buffers
/// \param R Result to verify
/// \returns Success if the test passes according to the specified Rule
llvm::Error verifyResult(offloadtest::Result R);

#endif // OFFLOADTEST_SUPPORT_CHECK_H
