//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RandomProgramProbabilities.h"

namespace reconvergence {
namespace random_program_probabilities {

const uint32_t kPercentDenominator = 100;
const uint32_t kNoiseDenominator = 10000;

const uint32_t kGenerateElsePercent = 50;
const uint32_t kReuseThenBlockForElsePercent = 10;
const uint32_t kDivergentBreakPercent = 10;
const uint32_t kDivergentContinuePercent = 10;
const uint32_t kElectExtraOpPercent = 10;
const uint32_t kElectReturnPercent = 30;
const uint32_t kReturnInMainPercent = 5;
const uint32_t kReturnInFunctionLoopPercent = 20;
const uint32_t kReturnInNestedFunctionLoopPercent = 50;
const uint32_t kDivergentReturnPercent = 10;
const uint32_t kInsertBallotPercent = 20;
const uint32_t kInsertStorePercent = 10;
const uint32_t kEmptyLoopNoisePerTenThousand = 3;
const uint32_t kInfiniteLoopNoisePerTenThousand = 10;

const uint32_t kTopLevelOpChoices = 11;
const uint32_t kLoopKindChoices = 3;
const uint32_t kCallOrReturnChoices = 5;
const uint32_t kCallChoice = 0;
const uint32_t kDoWhileKindChoices = 2;
const uint32_t kSwitchKindChoices = 4;

} // namespace random_program_probabilities
} // namespace reconvergence