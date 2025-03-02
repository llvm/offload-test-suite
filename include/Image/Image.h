//===- Image.h - Image Description ------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_IMAGE_IMAGE_H
#define OFFLOADTEST_IMAGE_IMAGE_H

#include "Image/Color.h"
#include "Support/Pipeline.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/bit.h"
#include "llvm/Support/Error.h"

#include <cassert>
#include <cstdint>

namespace llvm {
class raw_ostream;
}
namespace offloadtest {

class ImageComparatorBase {
public:
  virtual ~ImageComparatorBase() {}
  virtual void processPixel(Color L, Color R) = 0;
  virtual void print(llvm::raw_ostream &OS) {}
  virtual bool result() { return true; }
};

class ImageComparatorRef {
  std::unique_ptr<ImageComparatorBase> Comp;
  ImageComparatorRef() = delete;

public:
  ImageComparatorRef(std::unique_ptr<ImageComparatorBase> &&C)
      : Comp(std::move(C)) {}
  ImageComparatorRef(ImageComparatorRef &&) = default;
  void processPixel(Color L, Color R) { Comp->processPixel(L, R); }

  void print(llvm::raw_ostream &OS) { Comp->print(OS); };

  bool result() { return Comp->result(); }
};

template <typename T, typename... ArgTs>
std::enable_if_t<std::is_base_of_v<ImageComparatorBase, T>, ImageComparatorRef>
make_comparator(ArgTs &&...Args) {
  return ImageComparatorRef(std::make_unique<T>(Args...));
}

class ImageRef {
  uint32_t Height;
  uint32_t Width;
  uint8_t Depth;
  uint8_t Channels;
  bool IsFloat;

protected:
  llvm::StringRef Data;
  ImageRef(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F)
      : Height(H), Width(W), Depth(D), Channels(C), IsFloat(F), Data() {}

public:
  ImageRef() = default;
  ImageRef(const ImageRef &I) = default;
  ImageRef(ImageRef &&II) = default;
  ImageRef &operator=(const ImageRef &) = default;
  ImageRef &operator=(ImageRef &&) = default;

  ImageRef(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F,
           llvm::StringRef S)
      : Height(H), Width(W), Depth(D), Channels(C), IsFloat(F), Data(S) {
    assert((Channels == 3 || Channels == 4) && "Channels must be 3 or 4");
    assert(llvm::popcount(Depth) == 1 && "Depth must be a power of 2");
    assert(Depth <= 8 && "Depth must be <= 8 (64-bit)");
    assert(Data.size() == size() && "Data size does not match properties");
  }

  ImageRef(const Buffer &B)
      : ImageRef(
            B.OutputProps.Height, B.OutputProps.Width,
            B.getSingleElementSize(), B.Channels,
            B.Format == DataFormat::Float32 || B.Format == DataFormat::Float64,
            llvm::StringRef(B.Data.get(), B.size())) {}

  uint32_t getHeight() const { return Height; }
  uint32_t getWidth() const { return Width; }
  uint8_t getDepth() const { return Depth; }
  uint32_t getBitDepth() const { return Depth * 8; }
  uint8_t getChannels() const { return Channels; }
  bool isFloat() const { return IsFloat; }

  size_t size() const { return Data.size(); }
  const char *data() const { return Data.data(); }
  bool empty() const { return Data.empty(); }
};

class Image : public ImageRef {
  std::unique_ptr<char[]> OwnedData;

  Image(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F)
      : ImageRef(H, W, D, C, F) {
    uint64_t Sz = static_cast<uint64_t>(H) * static_cast<uint64_t>(W) *
                  static_cast<uint64_t>(D) * static_cast<uint64_t>(C);
    OwnedData = std::make_unique<char[]>(Sz);
    Data = llvm::StringRef(OwnedData.get(), Sz);
  }

  Image(uint32_t H, uint32_t W, uint8_t D, uint8_t C, bool F,
        std::unique_ptr<char[]> &&Ptr)
      : ImageRef(H, W, D, C, F), OwnedData(std::move(Ptr)) {
    uint64_t Sz = static_cast<uint64_t>(H) * static_cast<uint64_t>(W) *
                  static_cast<uint64_t>(D) * static_cast<uint64_t>(C);
    Data = llvm::StringRef(OwnedData.get(), Sz);
  }

  friend class ImageComparatorDiffImage;

public:
  // Not default constructable.
  Image() = delete;
  // Not copyable.
  Image(const Image &I) = delete;
  Image &operator=(const Image &I) = delete;
  // Movable.
  Image(Image &&I) = default;
  Image &operator=(Image &&I) = default;

  ImageRef getRef() const { return ImageRef(*this); }

  static llvm::Error writePNG(ImageRef I, llvm::StringRef Path);

  static Image translateImage(ImageRef I, uint8_t Depth, uint8_t Channels,
                              bool Float);
  static llvm::Expected<Image> loadPNG(llvm::StringRef Path);

  static llvm::Error
  compareImages(ImageRef LHS, ImageRef RHS,
                llvm::MutableArrayRef<ImageComparatorRef> Comparators);

  char *data() { return OwnedData.get(); }
};

} // namespace offloadtest

#endif // OFFLOADTEST_IMAGE_IMAGE_H
