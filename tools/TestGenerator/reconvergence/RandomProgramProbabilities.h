//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef RANDOMPROGRAMPROBABILITIES_H_
#define RANDOMPROGRAMPROBABILITIES_H_

#include <cstdint>

namespace reconvergence {
namespace random_program_probabilities {

extern const uint32_t kPercentDenominator;
extern const uint32_t kNoiseDenominator;

extern const uint32_t kGenerateElsePercent;
extern const uint32_t kReuseThenBlockForElsePercent;
extern const uint32_t kDivergentBreakPercent;
extern const uint32_t kDivergentContinuePercent;
extern const uint32_t kElectExtraOpPercent;
extern const uint32_t kElectReturnPercent;
extern const uint32_t kReturnInMainPercent;
extern const uint32_t kReturnInFunctionLoopPercent;
extern const uint32_t kReturnInNestedFunctionLoopPercent;
extern const uint32_t kDivergentReturnPercent;
extern const uint32_t kInsertBallotPercent;
extern const uint32_t kInsertStorePercent;
extern const uint32_t kEmptyLoopNoisePerTenThousand;
extern const uint32_t kInfiniteLoopNoisePerTenThousand;

extern const uint32_t kTopLevelOpChoices;
extern const uint32_t kLoopKindChoices;
extern const uint32_t kCallOrReturnChoices;
extern const uint32_t kCallChoice;
extern const uint32_t kDoWhileKindChoices;
extern const uint32_t kSwitchKindChoices;

} // namespace random_program_probabilities
} // namespace reconvergence

#endif // RANDOMPROGRAMPROBABILITIES_H_