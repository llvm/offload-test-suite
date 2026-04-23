//===- FormatConversion.h - Refactoring helpers ---------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Transitional helpers for converting between the legacy DataFormat + Channels
// description system and the unified Format enum. This file should be deleted
// once the pipeline is fully migrated to use Format directly.
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_FORMATCONVERSION_H
#define OFFLOADTEST_API_FORMATCONVERSION_H

#include "API/Resources.h"
#include "API/Texture.h"
#include "Support/Pipeline.h"

#include "llvm/Support/Error.h"

namespace offloadtest {

// Bridge for code that still describes textures as DataFormat + Channels (e.g.
// render targets bound via CPUBuffer). Once the pipeline is refactored to use
// Format directly, this function can be removed.
inline llvm::Expected<Format> toFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int16:
    switch (Channels) {
    case 1:
      return Format::R16Sint;
    case 2:
      return Format::RG16Sint;
    case 4:
      return Format::RGBA16Sint;
    }
    break;
  case DataFormat::UInt16:
    switch (Channels) {
    case 1:
      return Format::R16Uint;
    case 2:
      return Format::RG16Uint;
    case 4:
      return Format::RGBA16Uint;
    }
    break;
  case DataFormat::Int32:
    switch (Channels) {
    case 1:
      return Format::R32Sint;
    case 2:
      return Format::RG32Sint;
    case 4:
      return Format::RGBA32Sint;
    }
    break;
  case DataFormat::UInt32:
    switch (Channels) {
    case 1:
      return Format::R32Uint;
    case 2:
      return Format::RG32Uint;
    case 4:
      return Format::RGBA32Uint;
    }
    break;
  case DataFormat::Float32:
    switch (Channels) {
    case 1:
      return Format::R32Float;
    case 2:
      return Format::RG32Float;
    case 4:
      return Format::RGBA32Float;
    }
    break;
  case DataFormat::Depth32:
    // D32FloatS8Uint is not expressible as DataFormat + Channels because the
    // stencil component is uint8, not a second Depth32 channel. Once the
    // pipeline uses Format directly, this limitation goes away.
    if (Channels == 1)
      return Format::D32Float;
    break;
  // No Format mapping for these DataFormats.
  case DataFormat::Hex8:
  case DataFormat::Hex16:
  case DataFormat::Hex32:
  case DataFormat::Hex64:
  case DataFormat::UInt64:
  case DataFormat::Int64:
  case DataFormat::Float16:
  case DataFormat::Float64:
  case DataFormat::Bool:
    return llvm::createStringError(std::errc::invalid_argument,
                                   "DataFormat %d has no Format equivalent.",
                                   static_cast<int>(Format));
  }
  return llvm::createStringError(std::errc::invalid_argument,
                                 "No Format for DataFormat %d with %d "
                                 "channel(s).",
                                 static_cast<int>(Format), Channels);
}

// Validates that a TextureCreateDesc is consistent with the CPUBuffer it was
// derived from. Call this after building a TextureCreateDesc from a CPUBuffer
// to catch mismatches between the two description systems.
inline llvm::Error
validateTextureDescMatchesCPUBuffer(const TextureCreateDesc &Desc,
                                    const CPUBuffer &Buf) {
  auto ExpectedFmt = toFormat(Buf.Format, Buf.Channels);
  if (!ExpectedFmt)
    return ExpectedFmt.takeError();
  if (Desc.Fmt != *ExpectedFmt)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TextureCreateDesc format '%s' does not match CPUBuffer format "
        "(DataFormat %d, %d channels -> '%s').",
        getFormatName(Desc.Fmt).data(), static_cast<int>(Buf.Format),
        Buf.Channels, getFormatName(*ExpectedFmt).data());
  if (Desc.Width != static_cast<uint32_t>(Buf.OutputProps.Width))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TextureCreateDesc width %u does not match CPUBuffer width %d.",
        Desc.Width, Buf.OutputProps.Width);
  if (Desc.Height != static_cast<uint32_t>(Buf.OutputProps.Height))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TextureCreateDesc height %u does not match CPUBuffer height %d.",
        Desc.Height, Buf.OutputProps.Height);
  if (Desc.MipLevels != static_cast<uint32_t>(Buf.OutputProps.MipLevels))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TextureCreateDesc mip levels %u does not match CPUBuffer mip "
        "levels %d.",
        Desc.MipLevels, Buf.OutputProps.MipLevels);
  const uint32_t TexelSize = getFormatSizeInBytes(Desc.Fmt);
  if (Buf.Stride > 0 && static_cast<uint32_t>(Buf.Stride) != TexelSize)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "CPUBuffer stride %d does not match texture format element size %u.",
        Buf.Stride, TexelSize);
  const uint64_t ExpectedSize =
      static_cast<uint64_t>(Desc.Width) * Desc.Height * TexelSize;
  if (static_cast<uint64_t>(Buf.size()) != ExpectedSize)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "CPUBuffer size %u does not match expected size %llu "
        "(width %u * height %u * element size %u).",
        Buf.size(), ExpectedSize, Desc.Width, Desc.Height, TexelSize);
  return llvm::Error::success();
}

} // namespace offloadtest

#endif // OFFLOADTEST_API_FORMATCONVERSION_H
