//===- OffloadTest/Support/VkError.h - Vulkan Error Utils -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_SUPPORT_VKERROR_H
#define OFFLOADTEST_SUPPORT_VKERROR_H

#include "llvm/ADT/Twine.h"
#include "llvm/Support/Error.h"

#include <vulkan/vulkan.h>

namespace VK {
inline llvm::Error toError(VkResult Result, const llvm::Twine &Msg) {
  if (Result != VK_SUCCESS)
    return llvm::createStringError(
        Msg + " (VkResult = " + llvm::Twine(static_cast<int>(Result)) + ")");
  return llvm::Error::success();
}
} // namespace VK

#endif // OFFLOADTEST_SUPPORT_VKERROR_H
