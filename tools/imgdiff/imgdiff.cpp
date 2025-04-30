//===- imgdiff.cpp - Image Comparison Tool --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/Color.h"
#include "Image/Image.h"
#include "Image/ImageComparators.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/InitLLVM.h"
#include <string>

using namespace llvm;
using namespace offloadtest;

static cl::opt<std::string> ExpectedPath(cl::Positional,
                                         cl::desc("<expected image>"),
                                         cl::value_desc("filename"));

static cl::opt<std::string> ActualPath(cl::Positional,
                                       cl::desc("<actual image>"),
                                       cl::value_desc("filename"));

enum class ComparisonMode { ExactMatch, CIE76 };

static cl::opt<ComparisonMode>
    Mode("mode", cl::desc("Comparison Mode"), cl::init(ComparisonMode::CIE76),
         cl::values(clEnumValN(ComparisonMode::ExactMatch, "exact",
                               "Exact Match of decoded image data"),
                    clEnumValN(ComparisonMode::CIE76, "cie76",
                               "CIE76 Color Difference")));

static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"));

static cl::opt<std::string> RulesFilename("rules", cl::desc("Rules filename"),
                                          cl::value_desc("filename"));

int main(int ArgC, char **ArgV) {
  InitLLVM const X(ArgC, ArgV);
  cl::ParseCommandLineOptions(ArgC, ArgV, "Image Comparison Tool");
  ExitOnError const ExitOnErr("imgdiff: error: ");

  Image ExpectedImage = ExitOnErr(Image::loadPNG(ExpectedPath));
  Image ActualImage = ExitOnErr(Image::loadPNG(ActualPath));
  if (ExpectedImage.size() != ActualImage.size())
    ExitOnErr(createStringError(std::errc::executable_format_error,
                                "Image sizes do not match"));

  switch (Mode) {
  case ComparisonMode::ExactMatch:
    if (0 !=
        memcmp(ExpectedImage.data(), ActualImage.data(), ExpectedImage.size()))
      ExitOnErr(createStringError(std::errc::executable_format_error,
                                  "Images do not match"));
    return 0;
  case ComparisonMode::CIE76: {
    llvm::SmallVector<ImageComparatorRef> Cmps;

    if (!RulesFilename.empty()) {
      ErrorOr<std::unique_ptr<MemoryBuffer>> RulesFile =
          MemoryBuffer::getFileOrSTDIN(RulesFilename);
      ExitOnErr(errorCodeToError(RulesFile.getError()));
      std::vector<CompareCheck> Rules;
      yaml::Input YIn((*RulesFile)->getBuffer());
      YIn >> Rules;
      ExitOnErr(llvm::errorCodeToError(YIn.error()));
      Cmps.push_back(make_comparator<ImageComparatorDistance>(Rules));
    } else {
      Cmps.push_back(make_comparator<ImageComparatorDistance>());
    }
    if (!OutputFilename.empty())
      Cmps.push_back(make_comparator<ImageComparatorDiffImage>(
          ExpectedImage.getHeight(), ExpectedImage.getWidth(), OutputFilename));

    ExitOnErr(Image::compareImages(ExpectedImage, ActualImage, Cmps));

    bool Success = true;
    for (auto &Cmp : Cmps)
      Success = Cmp.result() && Success;

    for (auto &Cmp : Cmps)
      Cmp.print(llvm::errs());

    if (Success)
      return 0;
  }
  }

  return 1;
}
