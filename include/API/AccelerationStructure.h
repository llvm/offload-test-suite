//===- AccelerationStructure.h - RT Acceleration Structure Types ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef OFFLOADTEST_API_ACCELERATIONSTRUCTURE_H
#define OFFLOADTEST_API_ACCELERATIONSTRUCTURE_H

#include "API/API.h"
#include "API/Buffer.h"
#include "API/Resources.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Error.h"

#include <cstdint>
#include <variant>

namespace offloadtest {

struct AccelerationStructureSizes {
  uint64_t ResultDataMaxSizeInBytes = 0;
  uint64_t ScratchDataSizeInBytes = 0;
  uint64_t UpdateScratchDataSizeInBytes = 0;
};

struct TriangleGeometryDesc {
  Buffer *VertexBuffer = nullptr;
  uint64_t VertexBufferOffset = 0;
  uint32_t VertexCount = 0;
  uint32_t VertexStride = 0;
  Format VertexFormat = Format::RGB32Float;
  Buffer *IndexBuffer = nullptr;
  uint64_t IndexBufferOffset = 0;
  uint32_t IndexCount = 0;
  IndexFormat IdxFormat = IndexFormat::Uint32;
  bool Opaque = true;
};

struct AABBGeometryDesc {
  Buffer *AABBBuffer = nullptr;
  uint64_t AABBBufferOffset = 0;
  uint32_t AABBCount = 0;
  uint32_t AABBStride = 24;
  bool Opaque = true;
};

class AccelerationStructure;

struct AccelerationStructureInstance {
  float Transform[3][4] = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}};
  uint32_t InstanceID = 0;
  uint8_t InstanceMask = 0xFF;
  AccelerationStructure *BLAS = nullptr;
};

struct BLASBuildRequest {
  // Target AS this build writes into.
  AccelerationStructure *AS = nullptr;
  // DXR / Vulkan / Metal all forbid mixing triangle and AABB geometry in a
  // single BLAS, so the geometry list is held as a variant — the invalid
  // mixed-geometry state is unrepresentable.
  std::variant<llvm::SmallVector<TriangleGeometryDesc>,
               llvm::SmallVector<AABBGeometryDesc>>
      Geometry;
};

struct TLASBuildRequest {
  // Target AS this build writes into.
  AccelerationStructure *AS = nullptr;
  llvm::SmallVector<AccelerationStructureInstance> Instances;
};

inline llvm::Error validateGeometryDesc(const TriangleGeometryDesc &D) {
  if (!D.VertexBuffer)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: VertexBuffer is null.");
  if (!isPositionCompatible(D.VertexFormat))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: VertexFormat '%s' is not position-compatible.",
        getFormatName(D.VertexFormat).data());
  if (D.VertexStride < getFormatSizeInBytes(D.VertexFormat))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: VertexStride (%u) must be >= format size (%u).",
        D.VertexStride, getFormatSizeInBytes(D.VertexFormat));
  if (D.VertexCount == 0)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "TriangleGeometryDesc: VertexCount is 0.");
  if (D.IndexBuffer && D.IndexCount == 0)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: IndexBuffer is set but IndexCount is 0.");
  if (!D.IndexBuffer && D.IndexCount != 0)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: IndexCount is set but IndexBuffer is null.");
  if (D.IndexBuffer && D.IndexCount % 3 != 0)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: IndexCount (%u) must be a multiple of 3.",
        D.IndexCount);
  if (!D.IndexBuffer && D.VertexCount % 3 != 0)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TriangleGeometryDesc: VertexCount (%u) must be a multiple of 3 when "
        "no index buffer is provided.",
        D.VertexCount);
  return llvm::Error::success();
}

inline llvm::Error validateGeometryDesc(const AABBGeometryDesc &D) {
  if (!D.AABBBuffer)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "AABBGeometryDesc: AABBBuffer is null.");
  if (D.AABBCount == 0)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "AABBGeometryDesc: AABBCount is 0.");
  if (D.AABBStride < 24)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "AABBGeometryDesc: AABBStride (%u) must be >= 24.", D.AABBStride);
  return llvm::Error::success();
}

template <typename T>
inline llvm::Error validateBLASGeometry(llvm::ArrayRef<T> Geoms) {
  if (Geoms.empty())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "BLASBuildRequest must have at least one geometry descriptor.");
  for (const auto &G : Geoms)
    if (auto Err = validateGeometryDesc(G))
      return Err;
  return llvm::Error::success();
}

inline llvm::Error validateTLASBuildRequest(const TLASBuildRequest &Req) {
  if (Req.Instances.empty())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "TLASBuildRequest: Must have at least one instance.");
  for (size_t I = 0; I < Req.Instances.size(); ++I)
    if (!Req.Instances[I].BLAS)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "TLASBuildRequest: Instance %zu has a null BLAS pointer.", I);
  return llvm::Error::success();
}

class AccelerationStructure {
  GPUAPI API;
  AccelerationStructureSizes Sizes;

public:
  virtual ~AccelerationStructure();
  AccelerationStructure(const AccelerationStructure &) = delete;
  AccelerationStructure &operator=(const AccelerationStructure &) = delete;

  GPUAPI getAPI() const { return API; }
  // Result/scratch sizes the AS was allocated for. Available for the GPU
  // build path so it can size the scratch buffer without re-querying.
  const AccelerationStructureSizes &getSizes() const { return Sizes; }

protected:
  explicit AccelerationStructure(GPUAPI API,
                                 const AccelerationStructureSizes &Sizes)
      : API(API), Sizes(Sizes) {}
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ACCELERATIONSTRUCTURE_H
