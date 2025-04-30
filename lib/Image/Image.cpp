//===- Image.cpp - Image Description ----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#include "Image/Image.h"
#include "Image/Color.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/SwapByteOrder.h"

#include <png.h>

#include <algorithm>
#include <limits.h>
#include <stdio.h>

using namespace offloadtest;

template <typename DstType, typename SrcType>
static void translatePixelData(Image &Dst, ImageRef Src, bool ForWrite) {
  const uint64_t Pixels = Dst.getHeight() * Dst.getWidth();
  const uint32_t CopiedChannels =
      std::min(Dst.getChannels(), Src.getChannels());
  DstType *DstPtr = reinterpret_cast<DstType *>(Dst.data());
  const SrcType *SrcPtr = reinterpret_cast<const SrcType *>(Src.data());
  for (uint64_t I = 0; I < Pixels; ++I) {
    for (uint32_t J = 0; J < CopiedChannels; ++J, ++SrcPtr, ++DstPtr) {
      *DstPtr = ColorUtils::convertColor<DstType>(*SrcPtr);

      if (ForWrite && sizeof(DstType) > 1 && !llvm::sys::IsBigEndianHost)
        llvm::sys::swapByteOrder(*DstPtr);
    }
    // If the destination has more channels fill it with saturated values.
    for (uint32_t J = 0; J < CopiedChannels - Dst.getChannels();
         ++J, ++DstPtr) {
      if constexpr (std::is_floating_point<DstType>())
        *DstPtr = 1.0;
      else
        *DstPtr = std::numeric_limits<DstType>::max();
    }
    // If the source has more channels skip them.
    for (uint32_t J = 0; J < CopiedChannels - Src.getChannels(); ++J, ++SrcPtr)
      ;
  }
}

template <typename SrcType>
static void translatePixelSrc(Image &Dst, ImageRef Src, bool ForWrite) {

  switch (Dst.getDepth()) {
  case 1:
    assert(!Dst.isFloat() && "No float8 support!");
    translatePixelData<uint8_t, SrcType>(Dst, Src, ForWrite);
    break;
  case 2:
    assert(!Dst.isFloat() && "No float16 support!");
    translatePixelData<uint16_t, SrcType>(Dst, Src, ForWrite);
    break;
  case 4:
    if (Dst.isFloat())
      translatePixelData<float, SrcType>(Dst, Src, ForWrite);
    else
      translatePixelData<uint32_t, SrcType>(Dst, Src, ForWrite);
    break;
  case 8:
    if (Dst.isFloat())
      translatePixelData<double, SrcType>(Dst, Src, ForWrite);
    else
      translatePixelData<uint64_t, SrcType>(Dst, Src, ForWrite);
    break;
  default:
    llvm_unreachable("Destination depth out of expected range.");
  }
}

static void translatePixels(Image &Dst, ImageRef Src, bool ForWrite = false) {
  switch (Src.getDepth()) {
  case 1:
    assert(!Src.isFloat() && "No float8 support!");
    translatePixelSrc<uint8_t>(Dst, Src, ForWrite);
    break;
  case 2:
    assert(!Src.isFloat() && "No float16 support!");
    translatePixelSrc<uint16_t>(Dst, Src, ForWrite);
    break;
  case 4:
    if (Src.isFloat())
      translatePixelSrc<float>(Dst, Src, ForWrite);
    else
      translatePixelSrc<uint32_t>(Dst, Src, ForWrite);
    break;
  case 8:
    if (Src.isFloat())
      translatePixelSrc<double>(Dst, Src, ForWrite);
    else
      translatePixelSrc<uint64_t>(Dst, Src, ForWrite);
    break;
  default:
    llvm_unreachable("Source depth out of expected range.");
  }
}

Image Image::translateImage(ImageRef Src, uint8_t Depth, uint8_t Channels,
                            bool Float) {
  Image NewImage =
      Image(Src.getHeight(), Src.getWidth(), Depth, Channels, Float);
  translatePixels(NewImage, Src);
  return NewImage;
}

static llvm::Error writePNGImpl(ImageRef Img, llvm::StringRef OutputPath) {
  png_structp PNG =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!PNG)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed creating PNG data");
  png_infop PNGInfo = png_create_info_struct(PNG);
  FILE *F = nullptr;
  auto ScopeExit = llvm::make_scope_exit([&PNG, &PNGInfo, &F]() {
    png_destroy_write_struct(&PNG, &PNGInfo);
    if (F)
      fclose(F);
  });
  if (!PNGInfo)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed writing PNG info");

  F = fopen(OutputPath.data(), "wb");
  if (!F)
    return llvm::createStringError(std::errc::io_error, "Failed openiong file");
  const unsigned ImgFormat =
      Img.getChannels() == 4 ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB;
  assert((Img.getChannels() == 3 || Img.getChannels() == 4) &&
         "Only support RGB and RGBA images.");
  png_init_io(PNG, F);
  png_set_IHDR(PNG, PNGInfo, Img.getWidth(), Img.getHeight(), Img.getBitDepth(),
               ImgFormat, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  png_colorp ColorP =
      (png_colorp)png_malloc(PNG, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
  if (!ColorP)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed creating color palette");
  png_set_PLTE(PNG, PNGInfo, ColorP, PNG_MAX_PALETTE_LENGTH);
  png_write_info(PNG, PNGInfo);
  png_set_packing(PNG);

  png_bytepp Rows =
      (png_bytepp)png_malloc(PNG, Img.getHeight() * sizeof(png_bytep));
  const uint64_t RowSize = Img.getWidth() * Img.getChannels() * Img.getDepth();
  // Step one row back from the end
  const uint8_t *Row = reinterpret_cast<const uint8_t *>(Img.data()) +
                       (RowSize * Img.getHeight()) - RowSize;
  for (uint32_t I = 0; I < Img.getHeight(); ++I, Row -= RowSize)
    Rows[I] = const_cast<png_bytep>(Row);

  png_write_image(PNG, Rows);
  png_write_end(PNG, PNGInfo);
  png_free(PNG, ColorP);
  return llvm::Error::success();
}

llvm::Error Image::writePNG(ImageRef Img, llvm::StringRef Path) {
  const uint32_t NewDepth = std::min(static_cast<uint32_t>(Img.getDepth()), 2u);

  // If the image depth is > 1, we need to translate it to get the right
  // endianness.
  if (Img.getDepth() > 1) {
    Image Translated = Image(Img.getHeight(), Img.getWidth(), NewDepth,
                             Img.getChannels(), false);
    translatePixels(Translated, Img, /* ForWrite */ true);
    return writePNGImpl(Translated, Path);
  }
  return writePNGImpl(Img, Path);
}

llvm::Expected<Image> Image::loadPNG(llvm::StringRef Path) {
  png_image PNG;
  memset(&PNG, 0, sizeof(png_image));
  PNG.version = PNG_IMAGE_VERSION;

  if (png_image_begin_read_from_file(&PNG, Path.data()) == 0)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed reading PNG header from file");

  auto ScopeExit = llvm::make_scope_exit([&PNG]() { png_image_free(&PNG); });

  PNG.format = PNG_FORMAT_RGB;
  const size_t Size = PNG_IMAGE_SIZE(PNG);
  std::unique_ptr<char[]> Buffer = std::make_unique<char[]>(Size);

  if (png_image_finish_read(&PNG, NULL /*background*/,
                            reinterpret_cast<png_bytep>(Buffer.get()),
                            0 /*row_stride*/, NULL /*colormap*/) == 0)
    return llvm::createStringError(std::errc::io_error,
                                   "Failed reading PNG data from file");
  const uint32_t BytesPerPixel =
      static_cast<uint32_t>(Size / (PNG.height * PNG.width));
  const uint32_t Channels = 3;
  Image Result =
      Image(PNG.height, PNG.width, /*Depth*/ BytesPerPixel / Channels, Channels,
            false, std::move(Buffer));
  return Result;
}

llvm::Error
Image::compareImages(ImageRef LHS, ImageRef RHS,
                     llvm::MutableArrayRef<ImageComparatorRef> Comparators) {
  if (LHS.getHeight() != RHS.getHeight() || LHS.getWidth() != RHS.getWidth())
    return llvm::createStringError(
        std::errc::not_supported,
        "Cannot compute a difference of images with different dimensions.");
  assert(LHS.getChannels() >= 3 &&
         "Cannot operate on images with less than 3 channels.");
  assert(RHS.getChannels() >= 3 &&
         "Cannot operate on images with less than 3 channels.");
  const uint32_t CmpDepth = 4;
  const uint32_t CmpChannels = 3u;

  Image L = Image::translateImage(LHS, CmpDepth, CmpChannels, true);
  Image R = Image::translateImage(RHS, CmpDepth, CmpChannels, true);

  const float *LPtr = reinterpret_cast<const float *>(L.data());
  const float *RPtr = reinterpret_cast<const float *>(R.data());

  const uint64_t PixelCt = static_cast<uint64_t>(LHS.getHeight()) *
                           static_cast<uint64_t>(LHS.getWidth());
  for (uint64_t I = 0; I < PixelCt; ++I, LPtr += 3, RPtr += 3) {
    const Color L = Color(LPtr[0], LPtr[1], LPtr[2]);
    const Color R = Color(RPtr[0], RPtr[1], RPtr[2]);
    for (auto &Cmp : Comparators)
      Cmp.processPixel(L, R);
  }

  return llvm::Error::success();
}
