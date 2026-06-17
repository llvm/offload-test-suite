//===- MTL/MTLDevice.cpp - Metal Device -----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

#define IR_RUNTIME_METALCPP
#define IR_PRIVATE_IMPLEMENTATION
#include "metal_irconverter.h"
#include "metal_irconverter_runtime.h"
// ir_raytracing.h depends on types defined in metal_irconverter_runtime.h —
// keep this include in its own block so clang-format won't sort it above.
#include "ir_raytracing.h"

#include "API/Device.h"
#include "API/Encoder.h"
#include "API/FormatConversion.h"
#include "MTLDescriptorHeap.h"
#include "MTLResources.h"
#include "MTLTopLevelArgumentBuffer.h"
#include "Support/Pipeline.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include "../Util.h"

#include <algorithm>
#include <memory>

using namespace offloadtest;

static llvm::Error toError(NS::Error *Err) {
  if (!Err)
    return llvm::Error::success();
  const std::error_code EC =
      std::error_code(static_cast<int>(Err->code()), std::system_category());
  llvm::SmallString<256> ErrMsg;
  llvm::raw_svector_ostream OS(ErrMsg);
  OS << Err->localizedDescription()->utf8String() << ": ";
  OS << Err->localizedFailureReason()->utf8String();
  return llvm::createStringError(EC, ErrMsg);
}

static llvm::Error toError(const IRError *Err, llvm::StringRef Context) {
  if (!Err)
    return llvm::Error::success();

  const uint32_t Code = IRErrorGetCode(Err);
  if (IRErrorCodeNoError == Code)
    return llvm::Error::success();

  const std::error_code EC =
      std::error_code(static_cast<int>(Code), std::generic_category());
  llvm::SmallString<64> ErrMsg;
  llvm::raw_svector_ostream OS(ErrMsg);
  OS << Context << ": ";

  switch (Code) {
#define IR_ERR(x)                                                              \
  case x:                                                                      \
    OS << #x;                                                                  \
    break;

    IR_ERR(IRErrorCodeShaderRequiresRootSignature);
    IR_ERR(IRErrorCodeUnrecognizedRootSignatureDescriptor);
    IR_ERR(IRErrorCodeUnrecognizedParameterTypeInRootSignature);
    IR_ERR(IRErrorCodeResourceNotReferencedByRootSignature);
    IR_ERR(IRErrorCodeShaderIncompatibleWithDualSourceBlending);
    IR_ERR(IRErrorCodeUnsupportedWaveSize);
    IR_ERR(IRErrorCodeUnsupportedInstruction);
    IR_ERR(IRErrorCodeCompilationError);
    IR_ERR(IRErrorCodeFailedToSynthesizeStageInFunction);
    IR_ERR(IRErrorCodeFailedToSynthesizeStreamOutFunction);
    IR_ERR(IRErrorCodeFailedToSynthesizeIndirectIntersectionFunction);
    IR_ERR(IRErrorCodeUnableToVerifyModule);
    IR_ERR(IRErrorCodeUnableToLinkModule);
    IR_ERR(IRErrorCodeUnrecognizedDXILHeader);
    IR_ERR(IRErrorCodeInvalidRaytracingAttribute);
    IR_ERR(IRErrorCodeNullHullShaderInputOutputMismatch);
    IR_ERR(IRErrorCodeInvalidRaytracingUserAttributeSize);
    IR_ERR(IRErrorCodeIncorrectHitgroupType);
    IR_ERR(IRErrorCodeFP64Usage);
    IR_ERR(IRErrorCodeUnknown);
  default:
    break;
#undef IR_ERR
  }
  return llvm::createStringError(EC, ErrMsg);
}

#define MTLFormats(FMT)                                                        \
  if (Channels == 1)                                                           \
    return MTL::PixelFormatR##FMT;                                             \
  if (Channels == 2)                                                           \
    return MTL::PixelFormatRG##FMT;                                            \
  if (Channels == 4)                                                           \
    return MTL::PixelFormatRGBA##FMT;

static MTL::PixelFormat getMTLFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Int32:
    MTLFormats(32Sint) break;
  case DataFormat::Float32:
    MTLFormats(32Float) break;
  case DataFormat::UInt64:
  case DataFormat::Int64:
    if (Channels == 1)
      return MTL::PixelFormatRG32Uint;
    if (Channels == 2)
      return MTL::PixelFormatRGBA32Uint;
    llvm_unreachable("Unsupported channel count for 64-bit format");

  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return MTL::PixelFormatInvalid;
}

static IRShaderStage getShaderStage(Stages Stage) {
  switch (Stage) {
  case Stages::Compute:
    return IRShaderStageCompute;
  case Stages::Vertex:
    return IRShaderStageVertex;
  case Stages::Hull:
    llvm_unreachable("Hull shaders are not supported on Metal.");
  case Stages::Domain:
    llvm_unreachable("Domain shaders are not supported on Metal.");
  case Stages::Geometry:
    llvm_unreachable("Geometry shaders are not supported on Metal.");
  case Stages::Pixel:
    return IRShaderStageFragment;
  case Stages::Amplification:
    return IRShaderStageAmplification;
  case Stages::Mesh:
    return IRShaderStageMesh;
  case Stages::RayGeneration:
  case Stages::Miss:
  case Stages::ClosestHit:
  case Stages::AnyHit:
  case Stages::Intersection:
  case Stages::Callable:
    llvm_unreachable("RayTracing shaders take a different path on Metal.");
  }
  llvm_unreachable("All cases handled");
}

namespace {
struct MTLDeleter {
  template <typename T> void operator()(T *Arg) const {
    if (Arg)
      Arg->release();
  }
};

template <typename T> using MTLPtr = std::unique_ptr<T, MTLDeleter>;

template <auto Fn> struct IRDeleter {
  template <typename T> constexpr void operator()(T *Arg) const { Fn(Arg); }
};

using IRCompilerPtr = std::unique_ptr<IRCompiler, IRDeleter<IRCompilerDestroy>>;
using IRObjectPtr = std::unique_ptr<IRObject, IRDeleter<IRObjectDestroy>>;
using IRRootSignaturePtr =
    std::unique_ptr<IRRootSignature, IRDeleter<IRRootSignatureDestroy>>;
using IRMetalLibBinaryPtr =
    std::unique_ptr<IRMetalLibBinary, IRDeleter<IRMetalLibBinaryDestroy>>;
using IRShaderReflectionPtr =
    std::unique_ptr<IRShaderReflection, IRDeleter<IRShaderReflectionDestroy>>;
using IRErrorPtr = std::unique_ptr<IRError, IRDeleter<IRErrorDestroy>>;

struct MetalIR {
  IRMetalLibBinaryPtr Binary;
  IRShaderReflectionPtr Reflection;
};

class MTLFence : public offloadtest::Fence {
public:
  MTLFence(MTL::SharedEvent *Event, llvm::StringRef Name)
      : Name(Name), Event(Event) {}
  std::string Name;
  MTL::SharedEvent *Event;

  static llvm::Expected<std::unique_ptr<MTLFence>>
  create(MTL::Device *Device, llvm::StringRef Name) {
    MTL::SharedEvent *Event = Device->newSharedEvent();
    if (!Event)
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create shared event.");
    return std::make_unique<MTLFence>(Event, Name);
  }

  ~MTLFence() {
    if (Event)
      Event->release();
  }

  uint64_t getFenceValue() override { return Event->signaledValue(); }

  llvm::Error waitForCompletion(uint64_t SignalValue) override {
    if (!Event->waitUntilSignaledValue(SignalValue, UINT64_MAX))
      return llvm::createStringError(std::errc::timed_out,
                                     "Timed out waiting on shared event.");
    return llvm::Error::success();
  }
};

class MTLQueue : public offloadtest::Queue {
public:
  using Queue::submit;

  MTL::CommandQueue *Queue;
  std::unique_ptr<MTLFence> SubmitFence;
  uint64_t FenceCounter = 0;

  // Batches of command buffers submitted to the GPU that may still be
  // in-flight.  Each batch records the fence value it signals so we can
  // non-blockingly query progress and release completed batches.
  struct InFlightBatch {
    uint64_t FenceValue;
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs;
  };
  llvm::SmallVector<InFlightBatch> InFlightBatches;

  MTLQueue(MTL::CommandQueue *Queue, std::unique_ptr<MTLFence> SubmitFence)
      : Queue(Queue), SubmitFence(std::move(SubmitFence)) {}
  ~MTLQueue() override {
    if (Queue)
      Queue->release();
  }

  llvm::Expected<offloadtest::SubmitResult>
  submit(llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs)
      override;

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Buffer & /*Resource*/,
                     llvm::ArrayRef<TileMapping> /*Mappings*/) override {
    return llvm::createStringError(
        std::errc::not_supported,
        "Metal backend does not yet support tile mappings.");
  }

  llvm::Expected<offloadtest::SubmitResult>
  updateTileMappings(offloadtest::Texture & /*Resource*/,
                     llvm::ArrayRef<TileMapping> /*Mappings*/) override {
    return llvm::createStringError(
        std::errc::not_supported,
        "Metal backend does not yet support tile mappings.");
  }
};

class MTLPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  IRRootSignaturePtr RootSig;
  std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
  MTL::ComputePipelineState *ComputePipeline = nullptr;
  MTL::RenderPipelineState *RenderPipeline = nullptr;

  // Compute pipeline only state. Threadgroup size comes from numthreads() in
  // the HLSL source and is captured from shader reflection at pipeline
  // creation, so dispatch() doesn't need to re-query reflection each time.
  MTL::Size ThreadsPerGroup = MTL::Size(1, 1, 1);

  // Rasterization pipeline only state.
  // These are part of the pipeline in DX and VK, but dynamic state in Metal.
  // To have a shared API we store these here and set the state when the
  // pipeline is used.
  MTL::DepthStencilState *DepthStencilState = nullptr;
  MTL::CullMode CullMode = MTL::CullModeNone;

  MTL::Size MeshThreadsPerThreadgroup{1, 1, 1};
  MTL::Size ObjectThreadsPerThreadgroup{1, 1, 1};

  MTLPipelineState(llvm::StringRef Name, IRRootSignaturePtr RootSig,
                   std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer,
                   MTL::ComputePipelineState *ComputePipeline,
                   MTL::Size ThreadsPerGroup)
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        RootSig(std::move(RootSig)), ArgBuffer(std::move(ArgBuffer)),
        ComputePipeline(ComputePipeline), ThreadsPerGroup(ThreadsPerGroup) {}

  MTLPipelineState(llvm::StringRef Name, IRRootSignaturePtr RootSig,
                   std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer,
                   MTL::RenderPipelineState *RenderPipeline,
                   MTL::DepthStencilState *DepthStencilState,
                   MTL::CullMode CullMode,
                   MTL::Size MeshThreadsPerThreadgroup = {1, 1, 1},
                   MTL::Size ObjectThreadsPerThreadgroup = {1, 1, 1})
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        RootSig(std::move(RootSig)), ArgBuffer(std::move(ArgBuffer)),
        RenderPipeline(RenderPipeline), DepthStencilState(DepthStencilState),
        CullMode(CullMode),
        MeshThreadsPerThreadgroup(MeshThreadsPerThreadgroup),
        ObjectThreadsPerThreadgroup(ObjectThreadsPerThreadgroup) {}

  ~MTLPipelineState() override {
    if (ComputePipeline)
      ComputePipeline->release();
    if (RenderPipeline)
      RenderPipeline->release();
    if (DepthStencilState)
      DepthStencilState->release();
  }

  static bool classof(const offloadtest::PipelineState *B) {
    return B->getAPI() == GPUAPI::Metal;
  }
};

class MTLBuffer : public offloadtest::Buffer {
public:
  MTL::Buffer *Buf;
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;

  MTLBuffer(MTL::Buffer *Buf, llvm::StringRef Name, BufferCreateDesc Desc,
            size_t SizeInBytes)
      : offloadtest::Buffer(GPUAPI::Metal), Buf(Buf), Name(Name), Desc(Desc),
        SizeInBytes(SizeInBytes) {}
  MTLBuffer(const MTLBuffer &) = delete;
  MTLBuffer(MTLBuffer &&) = delete;
  MTLBuffer &operator=(const MTLBuffer &) = delete;
  MTLBuffer &operator=(MTLBuffer &&) = delete;

  size_t getSizeInBytes() const override { return SizeInBytes; }

  size_t querySparseTileSizeInBytes(const Device &Dev) const override;

  llvm::Expected<void *> map() override {
    if (Desc.Location == MemoryLocation::GpuOnly)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Cannot map a GpuOnly buffer.");
    return Buf->contents();
  }

  void unmap() override {
    // Managed storage (CpuToGpu) requires an explicit didModifyRange to
    // propagate CPU-side writes to the GPU. Shared storage (GpuToCpu) is
    // coherent and needs no action.
    if (Desc.Location == MemoryLocation::CpuToGpu)
      Buf->didModifyRange(NS::Range::Make(0, SizeInBytes));
  }

  ~MTLBuffer() override {
    if (Buf)
      Buf->release();
  }

  const BufferCreateDesc &getDesc() const override { return Desc; }

  static bool classof(const offloadtest::Buffer *B) {
    return B->getAPI() == GPUAPI::Metal;
  }
};

class MTLTexture : public offloadtest::Texture {
public:
  MTL::Texture *Tex;
  std::string Name;
  TextureCreateDesc Desc;

  MTLTexture(MTL::Texture *Tex, llvm::StringRef Name, TextureCreateDesc Desc)
      : offloadtest::Texture(GPUAPI::Metal), Tex(Tex), Name(Name), Desc(Desc) {}

  ~MTLTexture() override {
    if (Tex)
      Tex->release();
  }

  TileShape querySparseTileShape(const Device &Dev) const override;

  const TextureCreateDesc &getDesc() const override { return Desc; }

  static bool classof(const offloadtest::Texture *T) {
    return T->getAPI() == GPUAPI::Metal;
  }
};

/// Metal has no standalone render-pass object: render pass info lives on
/// MTLRenderPassDescriptor and is consumed when a render command encoder
/// is created. We therefore just stash the descriptor for the encoder to
/// translate later.
class MTLRenderPass final : public offloadtest::RenderPass {
public:
  offloadtest::RenderPassDesc Desc;

  explicit MTLRenderPass(offloadtest::RenderPassDesc Desc)
      : RenderPass(GPUAPI::Metal), Desc(std::move(Desc)) {}

  static bool classof(const offloadtest::RenderPass *RP) {
    return RP->getAPI() == GPUAPI::Metal;
  }
};

class MTLDevice; // forward decl — defined below in this same anon ns

class MTLCommandBuffer : public offloadtest::CommandBuffer {
public:
  MTL::CommandBuffer *CmdBuffer = nullptr;
  /// Back-pointer to the owning device; used by encoders that need to
  /// allocate scratch / instance buffers for AS builds.
  MTLDevice *Dev = nullptr;
  /// Buffers that must outlive command-buffer submission (e.g. AS scratch
  /// and TLAS instance buffers used during builds).
  llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> KeepAliveOwned;

  static llvm::Expected<std::unique_ptr<MTLCommandBuffer>>
  create(MTL::CommandQueue *Queue) {
    auto CB = std::unique_ptr<MTLCommandBuffer>(new MTLCommandBuffer());
    CB->CmdBuffer = Queue->commandBuffer();
    if (!CB->CmdBuffer)
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create Metal command buffer.");
    return CB;
  }

  ~MTLCommandBuffer() override = default;

  static bool classof(const CommandBuffer *CB) {
    return CB->getKind() == GPUAPI::Metal;
  }

  llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
  createComputeEncoder() override;

  llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
  createRenderEncoder(const offloadtest::RenderPassBeginDesc &Desc) override;

private:
  MTLCommandBuffer() : CommandBuffer(GPUAPI::Metal) {}
};

class MetalAccelerationStructure : public offloadtest::AccelerationStructure {
public:
  MTL::AccelerationStructure *AccelStruct;

  MetalAccelerationStructure(MTL::AccelerationStructure *AccelStruct,
                             const AccelerationStructureSizes &Sizes)
      : offloadtest::AccelerationStructure(GPUAPI::Metal, Sizes),
        AccelStruct(AccelStruct) {}

  ~MetalAccelerationStructure() override {
    if (AccelStruct)
      AccelStruct->release();
  }

  static bool classof(const offloadtest::AccelerationStructure *AS) {
    return AS->getAPI() == GPUAPI::Metal;
  }
};

llvm::Expected<offloadtest::SubmitResult> MTLQueue::submit(
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs) {
  // Non-blocking: query how far the GPU has progressed and release
  // command buffers from completed submissions.
  {
    const uint64_t Completed = SubmitFence->getFenceValue();
    llvm::erase_if(InFlightBatches, [Completed](const InFlightBatch &B) {
      return B.FenceValue <= Completed;
    });
  }

  // Metal serial queues guarantee that command buffers execute in commit order,
  // so no explicit wait on prior work is needed here.
  const uint64_t SignalValue = ++FenceCounter;

  for (size_t I = 0; I < CBs.size(); ++I) {
    auto &MCB = llvm::cast<MTLCommandBuffer>(*CBs[I].get());
    // Signal the submit fence when the last command buffer completes.
    if (I == CBs.size() - 1)
      MCB.CmdBuffer->encodeSignalEvent(SubmitFence->Event, SignalValue);
    MCB.CmdBuffer->commit();
  }

  // Keep submitted command buffers alive until the GPU is done with them.
  InFlightBatches.push_back({SignalValue, std::move(CBs)});

  return offloadtest::SubmitResult{SubmitFence.get(), SignalValue};
}

class MTLComputeEncoder : public offloadtest::ComputeEncoder {
  MTLCommandBuffer *CB = nullptr;
  MTL::CommandBuffer *CmdBuffer;
  MTL::ComputeCommandEncoder *ComputeEnc = nullptr;
  MTL::BlitCommandEncoder *BlitEnc = nullptr;
  /// Lazy AS encoder, created when batchBuildAS() is called and torn down at
  /// the next encoder transition (via endEncodingImpl).
  MTL::AccelerationStructureCommandEncoder *ASEnc = nullptr;

  /// Accumulated barrier scope from commands recorded since the last barrier.
  MTL::BarrierScope PendingScope = MTL::BarrierScope(0);

  /// Record that a command touched the given resource types.  The accumulated
  /// scope is flushed as a memoryBarrier before the next command.
  void addBarrierScope(MTL::BarrierScope Scope) { PendingScope |= Scope; }

  void flushBarrier() {
    if (ComputeEnc && PendingScope != MTL::BarrierScope(0)) {
      ComputeEnc->memoryBarrier(PendingScope);
      PendingScope = MTL::BarrierScope(0);
    }
  }

  /// End the blit encoder if active, lazily (re-)create the compute encoder.
  /// Metal requires a dedicated BlitCommandEncoder for copy operations. Metal 4
  /// moves blit operations onto the compute encoder, removing this separation.
  llvm::Error ensureComputeEncoder() {
    if (ComputeEnc)
      return llvm::Error::success();
    endEncodingImpl();
    ComputeEnc = CmdBuffer->computeCommandEncoder();
    if (!ComputeEnc)
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create Metal compute encoder.");
    ComputeEnc->pushDebugGroup(
        NS::String::string("ComputeEncoder", NS::UTF8StringEncoding));
    return llvm::Error::success();
  }

  /// End the compute encoder if active, lazily create the blit encoder.
  llvm::Error ensureBlitEncoder() {
    if (BlitEnc)
      return llvm::Error::success();
    endEncodingImpl();
    BlitEnc = CmdBuffer->blitCommandEncoder();
    if (!BlitEnc)
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create Metal blit encoder.");
    return llvm::Error::success();
  }

public:
  MTLComputeEncoder(MTLCommandBuffer *CB, MTL::CommandBuffer *CmdBuffer,
                    MTL::ComputeCommandEncoder *Encoder)
      : ComputeEncoder(GPUAPI::Metal), CB(CB), CmdBuffer(CmdBuffer),
        ComputeEnc(Encoder) {}

  ~MTLComputeEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::Metal;
  }

  MTL::ComputeCommandEncoder *getNative() const { return ComputeEnc; }

  MTL::CommandEncoder *getActiveEncoder() const {
    if (ComputeEnc)
      return ComputeEnc;
    return BlitEnc;
  }

  void pushDebugGroup(llvm::StringRef Label) override {
    if (auto *Enc = getActiveEncoder())
      Enc->pushDebugGroup(
          NS::String::string(Label.data(), NS::UTF8StringEncoding));
  }

  void popDebugGroup() override {
    if (auto *Enc = getActiveEncoder())
      Enc->popDebugGroup();
  }

  void insertDebugSignpost(llvm::StringRef Label) override {
    if (auto *Enc = getActiveEncoder())
      Enc->insertDebugSignpost(
          NS::String::string(Label.data(), NS::UTF8StringEncoding));
  }

  llvm::Error dispatch(const offloadtest::PipelineState &PSO,
                       uint32_t GroupCountX, uint32_t GroupCountY,
                       uint32_t GroupCountZ) override {
    const auto &MTLPSO = llvm::cast<MTLPipelineState>(PSO);
    if (!MTLPSO.ComputePipeline)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "PipelineState bound to dispatch() is not a compute pipeline.");
    if (auto Err = ensureComputeEncoder())
      return Err;
    flushBarrier();
    insertDebugSignpost(llvm::formatv("Dispatch [{0},{1},{2}]", GroupCountX,
                                      GroupCountY, GroupCountZ)
                            .str());
    ComputeEnc->setComputePipelineState(MTLPSO.ComputePipeline);

    const MTL::Size GridSize(MTLPSO.ThreadsPerGroup.width * GroupCountX,
                             MTLPSO.ThreadsPerGroup.height * GroupCountY,
                             MTLPSO.ThreadsPerGroup.depth * GroupCountZ);
    ComputeEnc->dispatchThreads(GridSize, MTLPSO.ThreadsPerGroup);
    addBarrierScope(MTL::BarrierScopeBuffers | MTL::BarrierScopeTextures);
    return llvm::Error::success();
  }

  llvm::Error copyBufferToBuffer(offloadtest::Buffer &Src, size_t SrcOffset,
                                 offloadtest::Buffer &Dst, size_t DstOffset,
                                 size_t Size) override {
    if (auto Err = ensureBlitEncoder())
      return Err;
    auto &MTLSrc = static_cast<MTLBuffer &>(Src);
    auto &MTLDst = static_cast<MTLBuffer &>(Dst);
    insertDebugSignpost(llvm::formatv("CopyBuffer {0}B", Size).str());
    BlitEnc->copyFromBuffer(MTLSrc.Buf, SrcOffset, MTLDst.Buf, DstOffset, Size);
    addBarrierScope(MTL::BarrierScopeBuffers);
    return llvm::Error::success();
  }

  llvm::Error copyBufferToTexture(offloadtest::Buffer &Src,
                                  offloadtest::Texture &Dst) override {
    if (auto Err = ensureBlitEncoder())
      return Err;
    auto &MTLSrc = static_cast<MTLBuffer &>(Src);
    auto &MTLDst = static_cast<MTLTexture &>(Dst);

    // The upload buffer is laid out with a tightly packed row stride matching
    // getTextureUploadRowStrideInBytes(), so the source bytes-per-row is the
    // texture width times the element size.
    const size_t ElemSize = getFormatSizeInBytes(MTLDst.Desc.Fmt);
    const size_t RowBytes = MTLDst.Desc.Width * ElemSize;
    const size_t ImageBytes = RowBytes * MTLDst.Desc.Height;
    const MTL::Size CopySize(MTLDst.Desc.Width, MTLDst.Desc.Height, 1);

    insertDebugSignpost(llvm::formatv("copyBufferToTexture {0} -> {1}",
                                      MTLSrc.Name, MTLDst.Name)
                            .str());
    BlitEnc->copyFromBuffer(MTLSrc.Buf, /*sourceOffset=*/0, RowBytes,
                            ImageBytes, CopySize, MTLDst.Tex,
                            /*destinationSlice=*/0, /*destinationLevel=*/0,
                            MTL::Origin(0, 0, 0));
    addBarrierScope(MTL::BarrierScopeTextures);

    return llvm::Error::success();
  }

  llvm::Error copyCounterToBuffer(offloadtest::Buffer &,
                                  offloadtest::Buffer &) override {
    return llvm::createStringError(
        std::errc::not_supported,
        "Counter buffers are not supported on the Metal backend.");
  }

  llvm::Error copyTextureToBuffer(offloadtest::Texture &Src,
                                  offloadtest::Buffer &Dst) override {
    if (auto Err = ensureBlitEncoder())
      return Err;
    auto &MTLSrc = static_cast<MTLTexture &>(Src);
    auto &MTLDst = static_cast<MTLBuffer &>(Dst);

    // The readback buffer is linear with a tightly packed row stride, so the
    // destination bytes-per-row is the texture width times the element size.
    const size_t ElemSize = getFormatSizeInBytes(MTLSrc.Desc.Fmt);
    const size_t RowBytes = MTLSrc.Desc.Width * ElemSize;
    const size_t ImageBytes = RowBytes * MTLSrc.Desc.Height;
    const MTL::Size CopySize(MTLSrc.Desc.Width, MTLSrc.Desc.Height, 1);

    insertDebugSignpost(llvm::formatv("copyTextureToBuffer {0} -> {1}",
                                      MTLSrc.Name, MTLDst.Name)
                            .str());
    BlitEnc->copyFromTexture(MTLSrc.Tex, /*sourceSlice=*/0, /*sourceLevel=*/0,
                             MTL::Origin(0, 0, 0), CopySize, MTLDst.Buf,
                             /*destinationOffset=*/0, RowBytes, ImageBytes);
    addBarrierScope(MTL::BarrierScopeBuffers);
    return llvm::Error::success();
  }

  // Defined out-of-line below — needs MTLDevice's full type for access to the
  // MTL::Device handle (used to allocate scratch and instance buffers).
  llvm::Error batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) override;

  /// Lazily transition into an AccelerationStructureCommandEncoder; mirrors
  /// the existing compute↔blit lazy switch.
  llvm::Error ensureASEncoder() {
    if (ASEnc)
      return llvm::Error::success();
    endEncodingImpl();
    ASEnc = CmdBuffer->accelerationStructureCommandEncoder();
    if (!ASEnc)
      return llvm::createStringError(
          std::errc::device_or_resource_busy,
          "Failed to create Metal acceleration-structure encoder.");
    return llvm::Error::success();
  }

  void endEncodingImpl() override {
    if (ComputeEnc) {
      flushBarrier();
      ComputeEnc->popDebugGroup();
      ComputeEnc->endEncoding();
      ComputeEnc = nullptr;
    }
    if (BlitEnc) {
      BlitEnc->endEncoding();
      BlitEnc = nullptr;
    }
    if (ASEnc) {
      ASEnc->endEncoding();
      ASEnc = nullptr;
    }
  }
};

llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
MTLCommandBuffer::createComputeEncoder() {
  MTL::ComputeCommandEncoder *NativeEncoder =
      CmdBuffer->computeCommandEncoder();
  if (!NativeEncoder)
    return llvm::createStringError(
        std::errc::device_or_resource_busy,
        "Failed to create Metal compute command encoder.");
  NativeEncoder->pushDebugGroup(
      NS::String::string("ComputeEncoder", NS::UTF8StringEncoding));
  return std::make_unique<MTLComputeEncoder>(this, CmdBuffer, NativeEncoder);
}

static MTL::LoadAction getMTLLoadAction(offloadtest::LoadAction Action) {
  switch (Action) {
  case offloadtest::LoadAction::Load:
    return MTL::LoadActionLoad;
  case offloadtest::LoadAction::Clear:
    return MTL::LoadActionClear;
  case offloadtest::LoadAction::DontCare:
    return MTL::LoadActionDontCare;
  }
  llvm_unreachable("All LoadAction cases handled");
}

static MTL::StoreAction getMTLStoreAction(offloadtest::StoreAction Action) {
  switch (Action) {
  case offloadtest::StoreAction::Store:
    return MTL::StoreActionStore;
  case offloadtest::StoreAction::DontCare:
    return MTL::StoreActionDontCare;
  }
  llvm_unreachable("All StoreAction cases handled");
}

class MTLRenderEncoder : public offloadtest::RenderEncoder {
  MTL::RenderCommandEncoder *RenderEnc = nullptr;

  // Encoder contract: viewport and scissor must both be set before
  // drawInstanced().
  bool ViewportSet = false;
  bool ScissorSet = false;

public:
  MTLRenderEncoder(MTL::RenderCommandEncoder *Enc)
      : RenderEncoder(GPUAPI::Metal), RenderEnc(Enc) {}
  MTLRenderEncoder(const MTLRenderEncoder &CB) = delete;
  MTLRenderEncoder(MTLRenderEncoder &&CB) = delete;
  MTLRenderEncoder &operator=(MTLRenderEncoder &CB) = delete;
  MTLRenderEncoder &operator=(const MTLRenderEncoder &&CB) = delete;

  ~MTLRenderEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::Metal;
  }

  /// Access the underlying Metal encoder for state that the abstract
  /// RenderEncoder API does not yet cover.
  /// Returns nullptr after endEncoding().
  MTL::RenderCommandEncoder *getNative() const { return RenderEnc; }

  void pushDebugGroup(llvm::StringRef Label) override {
    assert(RenderEnc);
    RenderEnc->pushDebugGroup(
        NS::String::string(Label.data(), NS::UTF8StringEncoding));
  }

  void popDebugGroup() override {
    assert(RenderEnc);
    RenderEnc->popDebugGroup();
  }

  void insertDebugSignpost(llvm::StringRef Label) override {
    assert(RenderEnc);
    RenderEnc->insertDebugSignpost(
        NS::String::string(Label.data(), NS::UTF8StringEncoding));
  }

  void setViewport(const offloadtest::Viewport &VP) override {
    RenderEnc->setViewport(MTL::Viewport{
        static_cast<double>(VP.X), static_cast<double>(VP.Y),
        static_cast<double>(VP.Width), static_cast<double>(VP.Height),
        static_cast<double>(VP.MinDepth), static_cast<double>(VP.MaxDepth)});
    ViewportSet = true;
  }

  void setScissor(const offloadtest::ScissorRect &Rect) override {
    MTL::ScissorRect MTLRect;
    MTLRect.x = static_cast<NS::UInteger>(Rect.X);
    MTLRect.y = static_cast<NS::UInteger>(Rect.Y);
    MTLRect.width = Rect.Width;
    MTLRect.height = Rect.Height;
    RenderEnc->setScissorRect(MTLRect);
    ScissorSet = true;
  }

  void setVertexBuffer(uint32_t Slot, offloadtest::Buffer *VB, size_t Offset,
                       uint32_t /*Stride*/) override {
    // Stride is needed in DX12 at binding time, ignore parameter here.
    // Metal Shader Converter reserves low buffer indices for its own tables;
    // vertex buffers start at kIRVertexBufferBindPoint. See
    // https://developer.apple.com/metal/shader-converter/ ("Metal vertex
    // fetch").
    const NS::UInteger BufIdx = kIRVertexBufferBindPoint + Slot;
    assert(Slot <
               sizeof(IRRuntimeVertexBuffers) / sizeof(IRRuntimeVertexBuffer) &&
           "Vertex buffer slot exceeds Metal Shader Converter limit");
    assert(Slot == 0 && "Pipeline vertex descriptor only describes slot 0");
    if (VB) {
      auto &MTLVB = llvm::cast<MTLBuffer>(*VB);
      RenderEnc->setVertexBuffer(MTLVB.Buf, Offset, BufIdx);
    } else {
      RenderEnc->setVertexBuffer(nullptr, 0, BufIdx);
    }
  }

  llvm::Error drawInstanced(const offloadtest::PipelineState &PSO,
                            uint32_t VertexCount, uint32_t InstanceCount,
                            uint32_t FirstVertex,
                            uint32_t FirstInstance) override {
    if (!ViewportSet)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Viewport must be set before drawing.");
    if (!ScissorSet)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Scissor must be set before drawing.");

    const auto &MTLPSO = llvm::cast<MTLPipelineState>(PSO);
    if (!MTLPSO.RenderPipeline)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "PipelineState bound to drawInstanced() is not a render pipeline.");
    RenderEnc->setRenderPipelineState(MTLPSO.RenderPipeline);
    if (MTLPSO.DepthStencilState)
      RenderEnc->setDepthStencilState(MTLPSO.DepthStencilState);
    RenderEnc->setCullMode(MTLPSO.CullMode);
    // Match the DX/VK convention (CCW = front) hardcoded in those backends.
    RenderEnc->setFrontFacingWinding(MTL::WindingCounterClockwise);

    // IRRuntimeDrawPrimitives also sets the DrawParams / DrawInfo argument
    // buffers that metal-irconverter consults for SV_VertexID and friends.
    IRRuntimeDrawPrimitives(RenderEnc, MTL::PrimitiveTypeTriangle,
                            static_cast<NS::UInteger>(FirstVertex),
                            static_cast<NS::UInteger>(VertexCount),
                            static_cast<NS::UInteger>(InstanceCount),
                            static_cast<NS::UInteger>(FirstInstance));

    return llvm::Error::success();
  }

  llvm::Error dispatchMesh(const offloadtest::PipelineState &PSO,
                           uint32_t GroupCountX, uint32_t GroupCountY,
                           uint32_t GroupCountZ) override {
    if (!ViewportSet)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Viewport must be set before drawing.");
    if (!ScissorSet)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Scissor must be set before drawing.");

    const auto &MTLPSO = llvm::cast<MTLPipelineState>(PSO);
    if (!MTLPSO.RenderPipeline)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "PipelineState bound to dispatchMesh() is not a render pipeline.");
    RenderEnc->setRenderPipelineState(MTLPSO.RenderPipeline);
    if (MTLPSO.DepthStencilState)
      RenderEnc->setDepthStencilState(MTLPSO.DepthStencilState);
    RenderEnc->setCullMode(MTLPSO.CullMode);
    // Match the DX/VK convention (CCW = front) hardcoded in those backends.
    RenderEnc->setFrontFacingWinding(MTL::WindingCounterClockwise);

    RenderEnc->drawMeshThreadgroups(
        MTL::Size(GroupCountX, GroupCountY, GroupCountZ),
        MTLPSO.ObjectThreadsPerThreadgroup, MTLPSO.MeshThreadsPerThreadgroup);

    return llvm::Error::success();
  }

  void endEncodingImpl() override {
    assert(RenderEnc);
    RenderEnc->popDebugGroup();
    RenderEnc->endEncoding();
    RenderEnc = nullptr;
  }
};

llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
MTLCommandBuffer::createRenderEncoder(
    const offloadtest::RenderPassBeginDesc &Desc) {
  if (!Desc.Pass)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc is missing its RenderPass.");
  auto &Pass = llvm::cast<MTLRenderPass>(*Desc.Pass);
  const offloadtest::RenderPassDesc &PassDesc = Pass.Desc;

  if (Desc.ColorAttachments.size() != PassDesc.ColorAttachments.size())
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RenderPassBeginDesc color attachment count does not match its "
        "RenderPass.");
  if (PassDesc.DepthStencil.has_value() != (Desc.DepthStencil != nullptr))
    return llvm::createStringError(std::errc::invalid_argument,
                                   "RenderPassBeginDesc depth-stencil "
                                   "presence does not match its RenderPass.");

  uint32_t Width = 0, Height = 0;
  if (auto Err = findAndValidateRenderPassTextureSize(Desc, &Width, &Height))
    return Err;

  MTL::RenderPassDescriptor *MTLDesc =
      MTL::RenderPassDescriptor::alloc()->init();
  auto DescScope = llvm::scope_exit([&] { MTLDesc->release(); });

  for (size_t I = 0; I < Desc.ColorAttachments.size(); ++I) {
    if (!Desc.ColorAttachments[I])
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RenderPassBeginDesc has a null color attachment texture.");
    auto &Tex = llvm::cast<MTLTexture>(*Desc.ColorAttachments[I]);
    const offloadtest::ColorAttachmentFormatDesc &Color =
        PassDesc.ColorAttachments[I];

    auto *CADesc = MTL::RenderPassColorAttachmentDescriptor::alloc()->init();
    CADesc->setTexture(Tex.Tex);
    CADesc->setLoadAction(getMTLLoadAction(Color.Load));
    CADesc->setStoreAction(getMTLStoreAction(Color.Store));
    if (Color.Load == offloadtest::LoadAction::Clear) {
      if (!Tex.getDesc().OptimizedClearValue) {
        CADesc->release();
        return llvm::createStringError(
            std::errc::invalid_argument,
            "LoadAction::Clear requires the render target to have been "
            "created with an OptimizedClearValue.");
      }
      const auto *CV =
          std::get_if<ClearColor>(&*Tex.getDesc().OptimizedClearValue);
      assert(CV && "RenderTarget OptimizedClearValue must be a ClearColor");
      CADesc->setClearColor(MTL::ClearColor(CV->R, CV->G, CV->B, CV->A));
    }
    MTLDesc->colorAttachments()->setObject(CADesc, I);
    CADesc->release();
  }

  if (Desc.DepthStencil) {
    auto &Tex = llvm::cast<MTLTexture>(*Desc.DepthStencil);
    const offloadtest::DepthStencilAttachmentFormatDesc &DS =
        *PassDesc.DepthStencil;

    auto *DADesc = MTLDesc->depthAttachment();
    DADesc->setTexture(Tex.Tex);
    DADesc->setLoadAction(getMTLLoadAction(DS.DepthLoad));
    DADesc->setStoreAction(getMTLStoreAction(DS.DepthStore));

    auto *SADesc = MTLDesc->stencilAttachment();
    SADesc->setTexture(Tex.Tex);
    SADesc->setLoadAction(getMTLLoadAction(DS.StencilLoad));
    SADesc->setStoreAction(getMTLStoreAction(DS.StencilStore));

    if (DS.DepthLoad == offloadtest::LoadAction::Clear ||
        DS.StencilLoad == offloadtest::LoadAction::Clear) {
      if (!Tex.getDesc().OptimizedClearValue)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "LoadAction::Clear requires the depth-stencil texture to have "
            "been created with an OptimizedClearValue.");
      const auto *CV =
          std::get_if<ClearDepthStencil>(&*Tex.getDesc().OptimizedClearValue);
      assert(CV &&
             "DepthStencil OptimizedClearValue must be a ClearDepthStencil");
      if (DS.DepthLoad == offloadtest::LoadAction::Clear)
        DADesc->setClearDepth(CV->Depth);
      if (DS.StencilLoad == offloadtest::LoadAction::Clear)
        SADesc->setClearStencil(CV->Stencil);
    }
  }

  MTLDesc->setRenderTargetWidth(Width);
  MTLDesc->setRenderTargetHeight(Height);

  MTL::RenderCommandEncoder *NativeEncoder =
      CmdBuffer->renderCommandEncoder(MTLDesc);
  if (!NativeEncoder)
    return llvm::createStringError(
        std::errc::device_or_resource_busy,
        "Failed to create Metal render command encoder.");
  NativeEncoder->pushDebugGroup(
      NS::String::string("RenderEncoder", NS::UTF8StringEncoding));
  return std::make_unique<MTLRenderEncoder>(NativeEncoder);
}

class MTLDevice : public offloadtest::Device {
  // MTLComputeEncoder needs access to the MTL::Device handle for AS scratch
  // and instance buffer allocation.
  friend class MTLComputeEncoder;

  Capabilities Caps;
  MTL::Device *Device;
  MTLQueue GraphicsQueue;

  struct ResourceSet {
    MTLPtr<MTL::Resource> Resource;
    // AS-only; mutually exclusive with Resource above.
    MetalAccelerationStructure *AS = nullptr;
    explicit ResourceSet(MTL::Resource *Resource) : Resource(Resource) {}
    explicit ResourceSet(MetalAccelerationStructure *AS) : AS(AS) {}
  };

  // ResourceBundle will contain one ResourceSet for a singular resource
  // or multiple ResourceSets for resource array.
  using ResourceBundle = llvm::SmallVector<ResourceSet>;
  using ResourcePair = std::pair<offloadtest::Resource *, ResourceBundle>;

  struct DescriptorTable {
    llvm::SmallVector<ResourcePair> Resources;
  };

  struct InvocationState {
    InvocationState() { Pool = NS::AutoreleasePool::alloc()->init(); }
    ~InvocationState() { Pool->release(); }

    NS::AutoreleasePool *Pool = nullptr;
    std::unique_ptr<MTLDescriptorHeap> DescHeap;
    std::unique_ptr<offloadtest::Buffer> VB;
    std::unique_ptr<offloadtest::Texture> RenderTarget;
    std::unique_ptr<offloadtest::Buffer> FrameBufferReadback;
    std::unique_ptr<offloadtest::Texture> DepthStencil;
    std::unique_ptr<MTLCommandBuffer> CB;
    std::unique_ptr<PipelineState> Pipeline;
    std::unique_ptr<offloadtest::RenderPass> RenderPass;

    llvm::SmallVector<DescriptorTable> DescTables;
    // TODO: Support RootResources?

    // Parallel-indexed to `P.AccelStructs.BLAS`.
    llvm::SmallVector<std::unique_ptr<offloadtest::AccelerationStructure>>
        BLASes;
    // Keyed by `TLASDesc::Name`.
    llvm::StringMap<std::unique_ptr<offloadtest::AccelerationStructure>> TLASes;
    // Vertex/index buffers consumed during AS builds; must outlive submission.
    llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> ASInputBuffers;
    // Per-AS header + contributions buffers; resident at dispatch.
    llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> ASDescriptorBuffers;
  };

  llvm::Error createRootSignature(
      const BindingsDesc &BindingsDesc, bool IsGraphics,
      IRRootSignaturePtr &OutRootSig,
      std::unique_ptr<MTLTopLevelArgumentBuffer> &OutArgBuffer) {
    uint32_t DescriptorCount = 0;
    for (auto &D : BindingsDesc.DescriptorSetDescs)
      DescriptorCount += D.ResourceBindings.size();

    std::vector<IRRootParameter1> RootParams;
    const std::unique_ptr<IRDescriptorRange1[]> Ranges =
        std::unique_ptr<IRDescriptorRange1[]>(
            new IRDescriptorRange1[DescriptorCount]);

    uint32_t RangeIdx = 0;
    for (const auto &Set : BindingsDesc.DescriptorSetDescs) {
      uint32_t DescriptorIdx = 0;
      const uint32_t StartRangeIdx = RangeIdx;
      for (const auto &Binding : Set.ResourceBindings) {
        auto &Range = Ranges.get()[RangeIdx];
        switch (getDescriptorKind(Binding.Kind)) {
        case DescriptorKind::SRV:
          Range.RangeType = IRDescriptorRangeTypeSRV;
          break;
        case DescriptorKind::UAV:
          Range.RangeType = IRDescriptorRangeTypeUAV;
          break;
        case DescriptorKind::CBV:
          Range.RangeType = IRDescriptorRangeTypeCBV;
          break;
        case DescriptorKind::SAMPLER:
          llvm_unreachable("Not implemented yet."); // Requires a separate heap
        }
        Range.NumDescriptors = Binding.DescriptorCount;
        Range.BaseShaderRegister = Binding.DXBinding.Register;
        Range.RegisterSpace = Binding.DXBinding.Space;
        Range.OffsetInDescriptorsFromTableStart = DescriptorIdx;
        llvm::outs() << "DescriptorRange[" << RangeIdx << "] {"
                     << " Type=" << static_cast<uint32_t>(Range.RangeType)
                     << ","
                     << " NumDescriptors=" << Range.NumDescriptors << ","
                     << " BaseShaderRegister=" << Range.BaseShaderRegister
                     << ","
                     << " RegisterSpace=" << Range.RegisterSpace << ","
                     << " OffsetInDescriptorsFromTableStart="
                     << Range.OffsetInDescriptorsFromTableStart << " }\n";
        RangeIdx++;
        DescriptorIdx += Binding.DescriptorCount;
      }

      auto &Param = RootParams.emplace_back();
      Param.ParameterType = IRRootParameterTypeDescriptorTable;
      Param.DescriptorTable.NumDescriptorRanges =
          static_cast<uint32_t>(Set.ResourceBindings.size());
      Param.DescriptorTable.pDescriptorRanges = &Ranges.get()[StartRangeIdx];
      Param.ShaderVisibility = IRShaderVisibilityAll;
    }

    // NOTE: Attempting to create a RS with version 1.0 seems to fail
    // with IRErrorCodeUnrecognizedRootSignatureDescriptor, creating with 1.1
    // instead
    IRVersionedRootSignatureDescriptor VersionedDesc = {};
    VersionedDesc.version = IRRootSignatureVersion_1_1;
    auto &Desc = VersionedDesc.desc_1_1;
    Desc.NumParameters = static_cast<uint32_t>(RootParams.size());
    Desc.pParameters = RootParams.data();
    Desc.NumStaticSamplers = 0;
    Desc.pStaticSamplers = nullptr;
    Desc.Flags = IsGraphics ? IRRootSignatureFlagAllowInputAssemblerInputLayout
                            : IRRootSignatureFlagNone;

    IRError *Err = nullptr;
    IRRootSignaturePtr RootSig(
        IRRootSignatureCreateFromDescriptor(&VersionedDesc, &Err));
    if (!RootSig)
      return toError(IRErrorPtr(Err).get(), "Failed to create root signature");

    OutRootSig = std::move(RootSig);

    auto ArgBufferOrErr =
        MTLTopLevelArgumentBuffer::create(Device, OutRootSig.get());
    if (!ArgBufferOrErr)
      return ArgBufferOrErr.takeError();

    OutArgBuffer = std::move(*ArgBufferOrErr);
    return llvm::Error::success();
  }

  llvm::Error createDescriptorHeap(Pipeline &P, InvocationState &State) {
    if (P.getDescriptorCount() == 0) {
      llvm::outs()
          << "No descriptors found, skipping descriptor heap creation.\n";
      return llvm::Error::success();
    }
    const uint32_t DescriptorCount = P.getDescriptorCountWithFlattenedArrays();
    const MTLDescriptorHeapDesc HeapDesc = {MTLDescriptorHeapType::CBV_SRV_UAV,
                                            DescriptorCount};

    auto DescHeapOrErr = MTLDescriptorHeap::create(Device, HeapDesc);
    if (!DescHeapOrErr)
      return DescHeapOrErr.takeError();

    State.DescHeap = std::move(*DescHeapOrErr);
    llvm::outs() << "Descriptor heap created with " << DescriptorCount
                 << " descriptors.\n";
    return llvm::Error::success();
  }

  llvm::Expected<MetalIR> convertToMetalIR(Stages Stage, bool IsGraphics,
                                           IRRootSignature *RootSig,
                                           const ShaderContainer &SC) {
    IRCompilerPtr Compiler(IRCompilerCreate());
    if (!Compiler)
      return llvm::createStringError(std::errc::not_supported,
                                     "Failed to create IR compiler instance.");

    if (!RootSig)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Root signature must be created before converting to Metal IR.");

    // Configure IR compiler settings
    IRCompilerSetEntryPointName(Compiler.get(), SC.EntryPoint.c_str());
    IRCompilerSetGlobalRootSignature(Compiler.get(), RootSig);
    if (IsGraphics) {
      // Matches DX::Device backend:
      // PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      IRCompilerSetInputTopology(Compiler.get(), IRInputTopologyTriangle);
    }

    const llvm::StringRef Program = SC.Shader->getBuffer();
    IRObject *DXIL = IRObjectCreateFromDXIL(
        reinterpret_cast<const uint8_t *>(Program.data()), Program.size(),
        IRBytecodeOwnershipNone);

    // Compile DXIL to Metal IR
    IRError *Err = nullptr;
    IRObjectPtr ResultIR(
        IRCompilerAllocCompileAndLink(Compiler.get(), nullptr, DXIL, &Err));
    if (Err)
      return toError(IRErrorPtr(Err).get(),
                     "Failed to compile and link DXIL to Metal IR");

    // Retrieve Metallib and shader reflection from the compiled IR object
    const IRShaderStage ShaderStage = getShaderStage(Stage);
    auto MetalLib = IRMetalLibBinaryPtr(IRMetalLibBinaryCreate());
    if (!IRObjectGetMetalLibBinary(ResultIR.get(), ShaderStage,
                                   MetalLib.get())) {
      return llvm::createStringError(
          std::errc::not_supported,
          "Failed to retrieve Metal library binary from "
          "IR object.");
    }

    auto Reflection = IRShaderReflectionPtr(IRShaderReflectionCreate());
    if (!IRObjectGetReflection(ResultIR.get(), ShaderStage, Reflection.get())) {
      return llvm::createStringError(
          std::errc::not_supported,
          "Failed to retrieve shader reflection from IR object.");
    }

    return MetalIR{std::move(MetalLib), std::move(Reflection)};
  }

  // Creates a Metal resource (buffer or texture) for the given Resource at the
  // specified array index.
  llvm::Expected<MTL::Resource *>
  createResource(Resource &R, size_t ResourceArrayIndex = 0) {
    const offloadtest::CPUBuffer &B = *R.BufferPtr;

    if (R.isRaw()) {
      MTL::Buffer *Buf =
          Device->newBuffer(B.Data[ResourceArrayIndex].get(), R.size(),
                            MTL::ResourceStorageModeManaged);
      Buf->didModifyRange(NS::Range::Make(0, Buf->length()));
      return Buf;
    } else {
      const uint64_t Width =
          R.isTexture() ? B.OutputProps.Width : R.size() / R.getElementSize();
      const uint64_t Height = R.isTexture() ? B.OutputProps.Height : 1;
      MTL::TextureUsage UsageFlags = MTL::ResourceUsageRead;
      if (R.isReadWrite())
        UsageFlags |= MTL::ResourceUsageWrite;
      MTL::TextureDescriptor *Desc = nullptr;
      const MTL::PixelFormat Format = getMTLFormat(B.Format, B.Channels);
      switch (R.Kind) {
      case ResourceKind::Buffer:
      case ResourceKind::RWBuffer:
        Desc = MTL::TextureDescriptor::textureBufferDescriptor(
            Format, Width, MTL::ResourceStorageModeManaged, UsageFlags);
        break;
      case ResourceKind::Texture2D:
      case ResourceKind::RWTexture2D:
        Desc = MTL::TextureDescriptor::texture2DDescriptor(Format, Width,
                                                           Height, false);
        break;
      case ResourceKind::Sampler:
        llvm_unreachable("Not implemented yet.");
      case ResourceKind::SampledTexture2D:
        llvm_unreachable("SampledTextures aren't supported in Metal.");
      case ResourceKind::StructuredBuffer:
      case ResourceKind::RWStructuredBuffer:
      case ResourceKind::ByteAddressBuffer:
      case ResourceKind::RWByteAddressBuffer:
      case ResourceKind::ConstantBuffer:
        llvm_unreachable("Raw is checked above");
      case ResourceKind::AccelerationStructure:
        llvm_unreachable("Acceleration structures use a separate path!");
      }

      MTL::Texture *NewTex = Device->newTexture(Desc);
      NewTex->replaceRegion(MTL::Region(0, 0, Width, Height), 0,
                            B.Data[ResourceArrayIndex].get(),
                            Width * R.getElementSize());
      return NewTex;
    }
  }

  llvm::Expected<ResourceBundle> createSRV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;

    for (size_t RegOffset = 0; RegOffset < R.BufferPtr->Data.size();
         ++RegOffset) {
      llvm::outs() << "Creating SRV: { Size = " << R.size() << ", Register = t"
                   << R.DXBinding.Register + RegOffset
                   << ", Space = " << R.DXBinding.Space;
      llvm::outs() << " }\n";

      auto ResourceOrErr = createResource(R, RegOffset);
      if (!ResourceOrErr)
        return ResourceOrErr.takeError();

      Bundle.emplace_back(ResourceOrErr.get());
    }
    return Bundle;
  }

  // TODO: counter buffer via IRRuntimeCreateAppendBufferView?
  llvm::Expected<ResourceBundle> createUAV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;

    for (size_t RegOffset = 0; RegOffset < R.BufferPtr->Data.size();
         ++RegOffset) {
      llvm::outs() << "Creating UAV: { Size = " << R.size() << ", Register = u"
                   << R.DXBinding.Register + RegOffset
                   << ", Space = " << R.DXBinding.Space
                   << ", HasCounter = " << R.HasCounter;
      llvm::outs() << " }\n";

      auto ResourceOrErr = createResource(R, RegOffset);
      if (!ResourceOrErr)
        return ResourceOrErr.takeError();

      Bundle.emplace_back(ResourceOrErr.get());
    }
    return Bundle;
  }

  llvm::Expected<ResourceBundle> createCBV(Resource &R, InvocationState &IS) {
    ResourceBundle Bundle;

    for (size_t RegOffset = 0; RegOffset < R.BufferPtr->Data.size();
         ++RegOffset) {
      llvm::outs() << "Creating CBV: { Size = " << R.size() << ", Register = b"
                   << R.DXBinding.Register + RegOffset
                   << ", Space = " << R.DXBinding.Space << " }\n";

      auto ResourceOrErr = createResource(R, RegOffset);
      if (!ResourceOrErr)
        return ResourceOrErr.takeError();

      Bundle.emplace_back(ResourceOrErr.get());
    }
    return Bundle;
  }

  void createDescriptor(Resource &R, MTL::Resource *Resource,
                        IRDescriptorTableEntry *Entry) {
    if (R.isRaw()) {
      IRBufferView View = {};
      View.buffer = static_cast<MTL::Buffer *>(Resource);
      View.bufferSize = R.size();
      IRDescriptorTableSetBufferView(Entry, &View);
    } else {
      MTL::Texture *Tex = static_cast<MTL::Texture *>(Resource);
      IRDescriptorTableSetTexture(Entry, Tex, 0, 0);
    }
  }

  // returns the next available HeapIdx
  uint32_t bindSRV(Resource &R, InvocationState &IS, uint32_t HeapIdx,
                   const ResourceBundle &ResBundle) {
    const uint32_t EltSize = R.getElementSize();
    const uint32_t NumElts = R.size() / EltSize;

    for (const ResourceSet &RS : ResBundle) {
      llvm::outs() << "SRV: HeapIdx = " << HeapIdx << " EltSize = " << EltSize
                   << " NumElts = " << NumElts << "\n";
      createDescriptor(R, RS.Resource.get(),
                       IS.DescHeap->getEntryHandle(HeapIdx));
      HeapIdx++;
    }
    return HeapIdx;
  }

  // returns the next available HeapIdx
  uint32_t bindUAV(Resource &R, InvocationState &IS, uint32_t HeapIdx,
                   const ResourceBundle &ResBundle) {
    const uint32_t EltSize = R.getElementSize();
    const uint32_t NumElts = R.size() / EltSize;
    for (const ResourceSet &RS : ResBundle) {
      llvm::outs() << "UAV: HeapIdx = " << HeapIdx << " EltSize = " << EltSize
                   << " NumElts = " << NumElts << "\n";
      createDescriptor(R, RS.Resource.get(),
                       IS.DescHeap->getEntryHandle(HeapIdx));
      HeapIdx++;
    }
    return HeapIdx;
  }

  // returns the next available HeapIdx
  uint32_t bindCBV(Resource &R, InvocationState &IS, uint32_t HeapIdx,
                   const ResourceBundle &ResBundle) {
    for (const ResourceSet &RS : ResBundle) {
      llvm::outs() << "CBV: HeapIdx = " << HeapIdx << " Size = " << R.size()
                   << "\n";
      createDescriptor(R, RS.Resource.get(),
                       IS.DescHeap->getEntryHandle(HeapIdx));
      HeapIdx++;
    }
    return HeapIdx;
  }

  llvm::Expected<std::unique_ptr<AccelerationStructure>> createAS(Resource &R) {
    assert(R.TLASPtr && "AS resource must be resolved to a TLAS");
    assert(R.getArraySize() == 1 && "AS arrays not yet supported");
    auto SizesOrErr =
        getTLASBuildSizes(static_cast<uint32_t>(R.TLASPtr->Instances.size()));
    if (!SizesOrErr)
      return SizesOrErr.takeError();
    return createTLAS(*SizesOrErr);
  }

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    auto CreateBuffer =
        [&IS,
         this](Resource &R,
               llvm::SmallVectorImpl<ResourcePair> &Resources) -> llvm::Error {
      if (R.isAccelerationStructure()) {
        auto ASOrErr = createAS(R);
        if (!ASOrErr)
          return ASOrErr.takeError();
        ResourceBundle Bundle;
        Bundle.emplace_back(
            llvm::cast<MetalAccelerationStructure>(ASOrErr->get()));
        auto Inserted =
            IS.TLASes.try_emplace(R.TLASPtr->Name, std::move(*ASOrErr));
        assert(Inserted.second && "TLAS bound to multiple resources NYI");
        (void)Inserted;
        Resources.emplace_back(&R, std::move(Bundle));
        return llvm::Error::success();
      }
      switch (getDescriptorKind(R.Kind)) {
      case DescriptorKind::SRV: {
        auto ExRes = createSRV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.emplace_back(&R, std::move(*ExRes));
        break;
      }
      case DescriptorKind::UAV: {
        auto ExRes = createUAV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.emplace_back(&R, std::move(*ExRes));
        break;
      }
      case DescriptorKind::CBV: {
        auto ExRes = createCBV(R, IS);
        if (!ExRes)
          return ExRes.takeError();
        Resources.emplace_back(&R, std::move(*ExRes));
        break;
      }
      case DescriptorKind::SAMPLER:
        return llvm::createStringError(
            std::errc::not_supported,
            "Samplers are not yet implemented for Metal.");
      }
      return llvm::Error::success();
    };

    for (auto &D : P.Sets) {
      IS.DescTables.emplace_back(DescriptorTable());
      DescriptorTable &Table = IS.DescTables.back();
      for (auto &R : D.Resources)
        if (auto Err = CreateBuffer(R, Table.Resources))
          return Err;
    }

    // Bind descriptors in descriptor tables.
    uint32_t HeapIndex = 0;
    for (auto &T : IS.DescTables) {
      for (auto &R : T.Resources) {
        if (MetalAccelerationStructure *MTLAS = R.second[0].AS) {
          // The Metal shader converter binds the AS indirectly through an
          // `IRRaytracingAccelerationStructureGPUHeader` buffer carrying the
          // AS's `gpuResourceID` and a pointer to an instance-contributions
          // array (one `uint32` per instance, equivalent to D3D12's
          // `InstanceContributionToHitGroupIndex`).
          const uint32_t InstCount =
              static_cast<uint32_t>(R.first->TLASPtr->Instances.size());
          llvm::SmallVector<uint32_t> Contributions;
          Contributions.reserve(InstCount);
          for (const auto &Inst : R.first->TLASPtr->Instances)
            Contributions.push_back(Inst.InstanceContributionToHitGroupIndex &
                                    0xFFFFFFu);
          const BufferCreateDesc Desc = BufferCreateDesc::uploadBuffer();
          auto ContribBufOrErr = createBufferWithData(
              *IS.CB->Dev, "AS-Contributions", Desc, Contributions.data(),
              InstCount * sizeof(uint32_t), nullptr, nullptr);
          if (!ContribBufOrErr)
            return ContribBufOrErr.takeError();
          auto *MTLContrib = llvm::cast<MTLBuffer>(ContribBufOrErr->get());
          auto HeaderBufOrErr = IS.CB->Dev->createBuffer(
              "AS-Header", Desc,
              sizeof(IRRaytracingAccelerationStructureGPUHeader));
          if (!HeaderBufOrErr)
            return HeaderBufOrErr.takeError();
          auto *MTLHeader = llvm::cast<MTLBuffer>(HeaderBufOrErr->get());
          IRRaytracingSetAccelerationStructure(
              static_cast<uint8_t *>(MTLHeader->Buf->contents()),
              MTLAS->AccelStruct->gpuResourceID(),
              static_cast<uint8_t *>(MTLContrib->Buf->contents()),
              MTLContrib->Buf->gpuAddress(), Contributions.data(), InstCount);

          IRDescriptorTableSetAccelerationStructure(
              IS.DescHeap->getEntryHandle(HeapIndex),
              MTLHeader->Buf->gpuAddress());

          // The shader dereferences the contributions buffer through the
          // header, so both must be resident at dispatch.
          IS.ASDescriptorBuffers.push_back(std::move(*HeaderBufOrErr));
          IS.ASDescriptorBuffers.push_back(std::move(*ContribBufOrErr));
          HeapIndex += R.first->getArraySize();
          continue;
        }
        switch (getDescriptorKind(R.first->Kind)) {
        case DescriptorKind::SRV:
          HeapIndex = bindSRV(*(R.first), IS, HeapIndex, R.second);
          break;
        case DescriptorKind::UAV:
          HeapIndex = bindUAV(*(R.first), IS, HeapIndex, R.second);
          break;
        case DescriptorKind::CBV:
          HeapIndex = bindCBV(*(R.first), IS, HeapIndex, R.second);
          break;
        case DescriptorKind::SAMPLER:
          llvm_unreachable("Not implemented yet.");
        }
      }
    }

    if (P.isTraditionalRaster() && P.Bindings.VertexBufferPtr) {
      const CPUBuffer *VBuffer = P.Bindings.VertexBufferPtr;

      BufferCreateDesc BufDesc = {};
      BufDesc.Location = MemoryLocation::CpuToGpu;
      BufDesc.Usage = BufferUsage::VertexBuffer;
      auto BufOrErr = createBufferWithData(*this, "VertexBuffer", BufDesc,
                                           VBuffer->Data[0].get(),
                                           VBuffer->size(), nullptr, nullptr);
      if (!BufOrErr)
        return BufOrErr.takeError();
      IS.VB = std::move(*BufOrErr);
      llvm::outs() << "Vertex buffer created.\n";
    }
    return llvm::Error::success();
  }

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    auto EncoderOrErr = IS.CB->createComputeEncoder();
    if (!EncoderOrErr)
      return EncoderOrErr.takeError();
    auto &Encoder = llvm::cast<MTLComputeEncoder>(*EncoderOrErr.get());
    MTL::ComputeCommandEncoder *NativeEncoder = Encoder.getNative();

    const auto &PS = llvm::cast<MTLPipelineState>(IS.Pipeline.get());
    MTLGPUDescriptorHandle Handle = {};
    if (IS.DescHeap) {
      IS.DescHeap->bind(NativeEncoder);
      Handle = IS.DescHeap->getGPUDescriptorHandleForHeapStart();
    }

    for (uint32_t Idx = 0u; Idx < P.Sets.size(); ++Idx) {
      PS->ArgBuffer->setRootDescriptorTable(Idx, Handle);
      Handle.addOffset(P.Sets[Idx].Resources.size());
    }

    PS->ArgBuffer->bind(NativeEncoder);
    for (const auto &Table : IS.DescTables)
      for (const auto &ResPair : Table.Resources)
        for (const auto &ResSet : ResPair.second)
          NativeEncoder->useResource(ResSet.Resource.get(),
                                     MTL::ResourceUsageRead |
                                         MTL::ResourceUsageWrite);
    auto MarkASResident =
        [&](const std::unique_ptr<AccelerationStructure> &AS) {
          auto *MTLAS = llvm::cast<MetalAccelerationStructure>(AS.get());
          NativeEncoder->useResource(MTLAS->AccelStruct,
                                     MTL::ResourceUsageRead);
        };
    for (auto &AS : IS.BLASes)
      MarkASResident(AS);
    for (auto &Entry : IS.TLASes)
      MarkASResident(Entry.second);
    for (auto &B : IS.ASDescriptorBuffers)
      NativeEncoder->useResource(llvm::cast<MTLBuffer>(B.get())->Buf,
                                 MTL::ResourceUsageRead);

    if (auto Err = Encoder.dispatch(*IS.Pipeline.get(),
                                    P.DispatchParameters.DispatchGroupCount[0],
                                    P.DispatchParameters.DispatchGroupCount[1],
                                    P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
    Encoder.endEncoding();
    return llvm::Error::success();
  }

  llvm::Error createRenderTarget(Pipeline &P, InvocationState &IS) {
    if (!P.Bindings.RTargetBufferPtr)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "No render target bound for graphics pipeline.");
    const CPUBuffer &OutBuf = *P.Bindings.RTargetBufferPtr;

    auto TexOrErr = offloadtest::createRenderTargetFromCPUBuffer(*this, OutBuf);
    if (!TexOrErr)
      return TexOrErr.takeError();

    IS.RenderTarget = std::move(*TexOrErr);

    // Create a readback buffer for copying render target data to the CPU.
    BufferCreateDesc BufDesc = BufferCreateDesc::readbackBuffer();
    auto BufOrErr = createBuffer("RTReadback", BufDesc, OutBuf.size());
    if (!BufOrErr)
      return BufOrErr.takeError();
    IS.FrameBufferReadback = std::move(*BufOrErr);

    return llvm::Error::success();
  }

  llvm::Error createDepthStencil(Pipeline &P, InvocationState &IS) {
    auto TexOrErr = offloadtest::createDefaultDepthStencilTarget(
        *this, P.Bindings.RTargetBufferPtr->OutputProps.Width,
        P.Bindings.RTargetBufferPtr->OutputProps.Height);
    if (!TexOrErr)
      return TexOrErr.takeError();
    IS.DepthStencil = std::move(*TexOrErr);
    return llvm::Error::success();
  }

  llvm::Error createGraphicsCommands(Pipeline &P, InvocationState &IS) {
    if (auto Err = createRenderTarget(P, IS))
      return Err;
    // TODO: Always created for graphics pipelines. Consider making this
    // conditional on the pipeline definition.
    if (auto Err = createDepthStencil(P, IS))
      return Err;

    const uint64_t Width = IS.RenderTarget->getDesc().Width;
    const uint64_t Height = IS.RenderTarget->getDesc().Height;

    RenderPassBeginDesc BeginDesc = {};
    BeginDesc.Pass = IS.RenderPass.get();
    BeginDesc.ColorAttachments.push_back(IS.RenderTarget.get());
    BeginDesc.DepthStencil = IS.DepthStencil.get();

    auto EncOrErr = IS.CB->createRenderEncoder(BeginDesc);
    if (!EncOrErr)
      return EncOrErr.takeError();
    auto &Encoder = *EncOrErr.get();

    {
      auto &MTLEncoder = llvm::cast<MTLRenderEncoder>(Encoder);
      const auto &PS = llvm::cast<MTLPipelineState>(IS.Pipeline.get());
      auto *CmdEncoder = MTLEncoder.getNative();
      if (IS.DescHeap) {
        IS.DescHeap->bind(CmdEncoder);
        // NOTE: This code assumes 1 descriptor set (D3D12 backend also assumes
        // this)
        PS->ArgBuffer->setRootDescriptorTable(
            0, IS.DescHeap->getGPUDescriptorHandleForHeapStart());
      }
      PS->ArgBuffer->bind(CmdEncoder);
      for (const auto &Table : IS.DescTables)
        for (const auto &ResPair : Table.Resources)
          for (const auto &ResSet : ResPair.second)
            CmdEncoder->useResource(ResSet.Resource.get(),
                                    MTL::ResourceUsageRead |
                                        MTL::ResourceUsageWrite);
    }

    Viewport VP;
    VP.Width = static_cast<float>(Width);
    VP.Height = static_cast<float>(Height);
    Encoder.setViewport(VP);

    ScissorRect Scissor;
    Scissor.Width = static_cast<uint32_t>(Width);
    Scissor.Height = static_cast<uint32_t>(Height);
    Encoder.setScissor(Scissor);

    if (P.isTraditionalRaster()) {
      if (IS.VB)
        Encoder.setVertexBuffer(0, IS.VB.get(), 0,
                                P.Bindings.getVertexStride());

      if (auto Err =
              Encoder.drawInstanced(*IS.Pipeline.get(), P.getVertexCount(),
                                    /*InstanceCount=*/1))
        return Err;
    } else {
      if (auto Err = Encoder.dispatchMesh(
              *IS.Pipeline.get(), P.DispatchParameters.DispatchGroupCount[0],
              P.DispatchParameters.DispatchGroupCount[1],
              P.DispatchParameters.DispatchGroupCount[2]))
        return Err;
    }

    Encoder.endEncoding();

    // Blit the render target into the readback buffer for CPU access.
    auto &FBTex = llvm::cast<MTLTexture>(*IS.RenderTarget);
    auto &FBReadback = llvm::cast<MTLBuffer>(*IS.FrameBufferReadback);
    MTL::BlitCommandEncoder *Blit = IS.CB->CmdBuffer->blitCommandEncoder();
    const size_t ElemSize = getFormatSizeInBytes(FBTex.Desc.Fmt);
    const size_t RowBytes = Width * ElemSize;
    Blit->copyFromTexture(FBTex.Tex, 0, 0, MTL::Origin(0, 0, 0),
                          MTL::Size(Width, Height, 1), FBReadback.Buf, 0,
                          RowBytes, 0);
    Blit->endEncoding();

    return llvm::Error::success();
  }

  llvm::Error copyBack(Pipeline &P, InvocationState &IS) {
    auto MemCpyBack = [](ResourcePair &Pair) -> llvm::Error {
      const Resource &R = *Pair.first;
      if (!R.isReadWrite())
        return llvm::Error::success();

      const CPUBuffer &B = *R.BufferPtr;
      auto *RSIt = Pair.second.begin();
      auto *DataIt = B.Data.begin();
      for (; RSIt != Pair.second.end() && DataIt != B.Data.end();
           ++RSIt, ++DataIt) {
        if (R.isRaw()) {
          MTL::Buffer *Buf = static_cast<MTL::Buffer *>(RSIt->Resource.get());
          memcpy(DataIt->get(), Buf->contents(), Buf->length());
        } else {
          MTL::Texture *Tex = static_cast<MTL::Texture *>(RSIt->Resource.get());
          const uint64_t Width = R.isTexture() ? B.OutputProps.Width
                                               : R.size() / R.getElementSize();
          const uint64_t Height = R.isTexture() ? B.OutputProps.Height : 1;
          Tex->getBytes(DataIt->get(), Width * R.getElementSize(),
                        MTL::Region(0, 0, Width, Height), 0);
        }
      }

      return llvm::Error::success();
    };

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        if (auto Err = MemCpyBack(R))
          return Err;

    if (P.isRaster()) {
      auto &FBReadback = llvm::cast<MTLBuffer>(*IS.FrameBufferReadback);
      auto *RT = P.Bindings.RTargetBufferPtr;
      RT->copyFromTexture(FBReadback.Buf->contents(), RT->getImageRowBytes());
    }
    return llvm::Error::success();
  }

public:
  MTLDevice(MTL::Device *D, MTL::CommandQueue *Q,
            std::unique_ptr<MTLFence> SubmitFence)
      : Device(D), GraphicsQueue(Q, std::move(SubmitFence)) {
    Description = Device->name()->utf8String();
  }
  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      queryCapabilities();
    return Caps;
  }

  llvm::StringRef getAPIName() const override { return "Metal"; };
  GPUAPI getAPI() const override { return GPUAPI::Metal; };

  static bool classof(const offloadtest::Device *D) {
    return D->getAPI() == GPUAPI::Metal;
  }

  Queue &getGraphicsQueue() override { return GraphicsQueue; }

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override {
    return MTLFence::create(Device, Name);
  }

  llvm::Expected<std::unique_ptr<offloadtest::MemoryHeap>>
  createMemoryHeap(std::string /*Name*/, size_t /*SizeInBytes*/) override {
    return llvm::createStringError(
        std::errc::not_supported,
        "Metal backend does not yet support memory heaps.");
  }

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, const BufferCreateDesc &Desc,
               size_t SizeInBytes) override {
    if (Desc.HasCounter)
      return llvm::createStringError(
          std::errc::not_supported,
          "Metal backend does not support buffers with a counter.");

    if (Desc.Backing == MemoryBacking::Sparse)
      return llvm::createStringError(
          std::errc::not_supported,
          "Metal backend does not support sparse memory backing.");

    MTL::Buffer *Buf = Device->newBuffer(
        SizeInBytes, getMetalBufferResourceOptions(Desc.Location));
    if (!Buf)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Failed to create Metal buffer.");
    return std::make_unique<MTLBuffer>(Device, Buf, Name, Desc, SizeInBytes);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Texture>>
  createTexture(std::string Name, const TextureCreateDesc &Desc) override {
    if (auto Err = validateTextureCreateDesc(Desc))
      return Err;

    MTL::TextureDescriptor *TDesc = MTL::TextureDescriptor::texture2DDescriptor(
        getMetalPixelFormat(Desc.Fmt), Desc.Width, Desc.Height,
        Desc.MipLevels > 1);
    TDesc->setMipmapLevelCount(Desc.MipLevels);
    TDesc->setStorageMode(getMetalTextureStorageMode(Desc.Location));
    TDesc->setUsage(getMetalTextureUsage(Desc.Usage));

    MTL::Texture *Tex = Device->newTexture(TDesc);
    if (!Tex)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Failed to create Metal texture.");
    return std::make_unique<MTLTexture>(Tex, Name, Desc);
  }

  uint32_t getTextureUploadRowStrideInBytes(
      const TextureCreateDesc &Desc) const override {
    return Desc.Width * getFormatSizeInBytes(Desc.Fmt);
  }

  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override {
    auto CBOrErr = MTLCommandBuffer::create(GraphicsQueue.Queue);
    if (!CBOrErr)
      return CBOrErr.takeError();
    (*CBOrErr)->Dev = this;
    return std::unique_ptr<offloadtest::CommandBuffer>(std::move(*CBOrErr));
  }

  llvm::Expected<std::unique_ptr<offloadtest::RenderPass>>
  createRenderPass(const offloadtest::RenderPassDesc &Desc) override {
    return std::make_unique<MTLRenderPass>(Desc);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const BindingsDesc &BindingsDesc,
                   ShaderContainer CS) override {
    IRRootSignaturePtr RootSig;
    std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
    if (auto Err = createRootSignature(BindingsDesc, /*IsGraphics=*/false,
                                       RootSig, ArgBuffer))
      return Err;

    auto MetalIR = convertToMetalIR(Stages::Compute, /*IsGraphics=*/false,
                                    RootSig.get(), CS);
    if (!MetalIR)
      return MetalIR.takeError();

    dispatch_data_t Data = IRMetalLibGetBytecodeData(MetalIR->Binary.get());
    NS::Error *Error = nullptr;
    MTL::Library *Lib = Device->newLibrary(Data, &Error);
    if (Error)
      return toError(Error);
    auto LibScope = llvm::scope_exit([&] { Lib->release(); });

    MTL::Function *Fn = Lib->newFunction(
        NS::String::string(CS.EntryPoint.c_str(), NS::UTF8StringEncoding));
    if (!Fn)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Failed to find entry point '%s' in Metal library.",
          CS.EntryPoint.c_str());
    auto FnScope = llvm::scope_exit([&] { Fn->release(); });

    MTL::ComputePipelineState *PSO =
        Device->newComputePipelineState(Fn, &Error);
    if (Error)
      return toError(Error);

    IRVersionedCSInfo Info;
    if (!IRShaderReflectionCopyComputeInfo(MetalIR->Reflection.get(),
                                           IRReflectionVersion_1_0, &Info))
      return llvm::createStringError(
          "Failed to read compute reflection for entry point '%s'; cannot "
          "determine threadgroup size from numthreads().",
          CS.EntryPoint.c_str());
    const MTL::Size ThreadsPerGroup(Info.info_1_0.tg_size[0],
                                    Info.info_1_0.tg_size[1],
                                    Info.info_1_0.tg_size[2]);
    IRShaderReflectionReleaseComputeInfo(&Info);

    return std::make_unique<MTLPipelineState>(
        Name, std::move(RootSig), std::move(ArgBuffer), PSO, ThreadsPerGroup);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createTraditionalRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const TraditionalRasterPipelineCreateDesc &Desc) override {
    if (Desc.GS)
      return llvm::createStringError(
          std::errc::not_supported,
          "Geometry shaders are not supported on this backend.");
    if (Desc.HS || Desc.DS)
      return llvm::createStringError(
          std::errc::not_supported,
          "Hull/Domain (tessellation) shaders are not supported on this "
          "backend.");
    if (Desc.Topology != PrimitiveTopology::TriangleList)
      return llvm::createStringError(
          std::errc::not_supported,
          "Only TriangleList topology is currently supported.");
    const ShaderContainer &VS = Desc.VS;
    const ShaderContainer &PS = Desc.PS;
    const llvm::ArrayRef<InputLayoutDesc> InputLayout = Desc.InputLayout;
    const llvm::ArrayRef<Format> RTFormats = Desc.RTFormats;
    const std::optional<Format> DSFormat = Desc.DSFormat;

    IRRootSignaturePtr RootSig;
    std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
    if (auto Err = createRootSignature(BindingsDesc, /*IsGraphics=*/true,
                                       RootSig, ArgBuffer))
      return Err;

    NS::Error *Error = nullptr;

    // Load vertex shader.
    auto VSMetalIR = convertToMetalIR(Stages::Vertex, /*IsGraphics=*/true,
                                      RootSig.get(), VS);
    if (!VSMetalIR)
      return VSMetalIR.takeError();

    dispatch_data_t VSData = IRMetalLibGetBytecodeData(VSMetalIR->Binary.get());
    MTL::Library *VSLib = Device->newLibrary(VSData, &Error);
    if (Error)
      return toError(Error);
    auto VSLibScope = llvm::scope_exit([&] { VSLib->release(); });

    MTL::Function *VSFn = VSLib->newFunction(
        NS::String::string(VS.EntryPoint.c_str(), NS::UTF8StringEncoding));
    if (!VSFn)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Failed to find vertex entry point '%s' in Metal library.",
          VS.EntryPoint.c_str());
    auto VSFnScope = llvm::scope_exit([&] { VSFn->release(); });

    // Load pixel/fragment shader.
    auto PSMetalIR =
        convertToMetalIR(Stages::Pixel, /*IsGraphics=*/true, RootSig.get(), PS);
    if (!PSMetalIR)
      return PSMetalIR.takeError();

    dispatch_data_t PSData = IRMetalLibGetBytecodeData(PSMetalIR->Binary.get());
    MTL::Library *PSLib = Device->newLibrary(PSData, &Error);
    if (Error)
      return toError(Error);
    auto PSLibScope = llvm::scope_exit([&] { PSLib->release(); });

    MTL::Function *PSFn = PSLib->newFunction(
        NS::String::string(PS.EntryPoint.c_str(), NS::UTF8StringEncoding));
    if (!PSFn)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Failed to find fragment entry point '%s' in Metal library.",
          PS.EntryPoint.c_str());
    auto PSFnScope = llvm::scope_exit([&] { PSFn->release(); });

    MTL::RenderPipelineDescriptor *RPDesc =
        MTL::RenderPipelineDescriptor::alloc()->init();
    auto RPDescScope = llvm::scope_exit([&] { RPDesc->release(); });
    RPDesc->setVertexFunction(VSFn);
    RPDesc->setFragmentFunction(PSFn);

    // Build vertex descriptor from InputLayout.
    if (!InputLayout.empty()) {
      NS::Array *FnAttrs = VSFn->vertexAttributes();
      // Currently we error on vertex shaders without any vertex attributes.
      // However, this is a valid use case that should be supported in the
      // future.
      if (!FnAttrs)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Vertex shader has no vertex attributes.");

      // Collect the attribute indices the shader expects so that we can map
      // the specified attributes onto the correct indices. Only active
      // attributes are exposed via stage_in; inactive entries are dropped by
      // the optimizer and have no corresponding [[attribute(N)]] slot.
      llvm::StringMap<uint32_t> ShaderAttrIndices;
      for (uint32_t I = 0; I < FnAttrs->count(); ++I) {
        auto *A = static_cast<MTL::VertexAttribute *>(FnAttrs->object(I));
        if (A && A->isActive()) {
          ShaderAttrIndices.insert(std::make_pair(
              llvm::StringRef(A->name()->utf8String()), A->attributeIndex()));
          llvm::errs() << "Shader attr: " << A->name()->utf8String()
                       << " at index " << A->attributeIndex() << "\n";
        }
      }

      if (ShaderAttrIndices.size() != InputLayout.size())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Mismatch between vertex shader active attribute count and "
            "pipeline vertex input count.");

      MTL::VertexDescriptor *VtxDesc = MTL::VertexDescriptor::alloc()->init();
      auto VtxDescScope = llvm::scope_exit([&] { VtxDesc->release(); });
      uint32_t Stride = 0;
      for (uint32_t I = 0; I < static_cast<uint32_t>(InputLayout.size()); ++I) {
        const InputLayoutDesc &Elem = InputLayout[I];
        assert(!Elem.InstanceStepRate &&
               "Instance step rate is currently not supported.");

        llvm::SmallString<32> AttrName(Elem.Name);
        llvm::transform(AttrName, AttrName.begin(), tolower);
        // Append a zero since we're only supporting one attribute per name.
        // We'll need to revisit this if we ever support indexed attributes.
        AttrName += "0";

        auto It = ShaderAttrIndices.find(AttrName);
        if (It == ShaderAttrIndices.end())
          return llvm::createStringError(
              std::errc::invalid_argument,
              "Input layout element '%s' does not match any active vertex "
              "shader attribute.",
              AttrName.c_str());

        const uint32_t ElemSize = getFormatSizeInBytes(Elem.Fmt);
        MTL::VertexAttributeDescriptor *AttrDesc =
            MTL::VertexAttributeDescriptor::alloc()->init();
        AttrDesc->setBufferIndex(kIRVertexBufferBindPoint);
        AttrDesc->setOffset(Elem.OffsetInBytes);
        AttrDesc->setFormat(getMetalVertexFormat(Elem.Fmt));
        VtxDesc->attributes()->setObject(AttrDesc, It->getValue());
        AttrDesc->release();
        Stride = std::max(Stride, Elem.OffsetInBytes + ElemSize);
      }

      MTL::VertexBufferLayoutDescriptor *LDesc =
          MTL::VertexBufferLayoutDescriptor::alloc()->init();
      LDesc->setStride(Stride);
      LDesc->setStepRate(1);
      LDesc->setStepFunction(MTL::VertexStepFunctionPerVertex);
      VtxDesc->layouts()->setObject(LDesc, kIRVertexBufferBindPoint);
      LDesc->release();

      RPDesc->setVertexDescriptor(VtxDesc);
    }

    // Configure render target color attachments.
    for (size_t I = 0; I < RTFormats.size(); ++I) {
      MTL::RenderPipelineColorAttachmentDescriptor *RPCA =
          MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init();
      RPCA->setPixelFormat(getMetalPixelFormat(RTFormats[I]));
      RPDesc->colorAttachments()->setObject(RPCA, I);
      RPCA->release();
    }

    // Configure depth/stencil attachment.
    if (DSFormat) {
      const MTL::PixelFormat DSPixelFormat = getMetalPixelFormat(*DSFormat);
      RPDesc->setDepthAttachmentPixelFormat(DSPixelFormat);
      if (isStencilFormat(*DSFormat))
        RPDesc->setStencilAttachmentPixelFormat(DSPixelFormat);
    }

    MTL::RenderPipelineState *PSO =
        Device->newRenderPipelineState(RPDesc, &Error);
    if (Error)
      return toError(Error);

    MTL::DepthStencilDescriptor *DSDesc =
        MTL::DepthStencilDescriptor::alloc()->init();
    DSDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    DSDesc->setDepthWriteEnabled(true);
    MTL::DepthStencilState *DSState = Device->newDepthStencilState(DSDesc);
    DSDesc->release();

    return std::make_unique<MTLPipelineState>(Name, std::move(RootSig),
                                              std::move(ArgBuffer), PSO,
                                              DSState, MTL::CullModeNone);
  }

  llvm::Expected<std::unique_ptr<PipelineState>> createMeshShaderRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const MeshShaderRasterPipelineCreateDesc &Desc) override {
    IRRootSignaturePtr RootSig;
    std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
    if (auto Err = createRootSignature(BindingsDesc, /*IsGraphics=*/true,
                                       RootSig, ArgBuffer))
      return Err;

    NS::Error *Error = nullptr;
    auto compileStage = [&](Stages Stage, const ShaderContainer &SC,
                            llvm::StringRef RoleName, MetalIR &OutIR,
                            MTLPtr<MTL::Library> &OutLib,
                            MTLPtr<MTL::Function> &OutFn) -> llvm::Error {
      auto IROrErr =
          convertToMetalIR(Stage, /*IsGraphics=*/true, RootSig.get(), SC);
      if (!IROrErr)
        return IROrErr.takeError();
      OutIR = std::move(*IROrErr);

      dispatch_data_t Data = IRMetalLibGetBytecodeData(OutIR.Binary.get());
      NS::Error *Err = nullptr;
      OutLib = MTLPtr<MTL::Library>(Device->newLibrary(Data, &Err));
      if (Err)
        return toError(Err);

      OutFn = MTLPtr<MTL::Function>(OutLib->newFunction(
          NS::String::string(SC.EntryPoint.c_str(), NS::UTF8StringEncoding)));
      if (!OutFn)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Failed to find %s entry point '%s' in Metal library.",
            RoleName.data(), SC.EntryPoint.c_str());
      return llvm::Error::success();
    };

    MetalIR MSIR;
    MTLPtr<MTL::Library> MSLib;
    MTLPtr<MTL::Function> MSFn;
    if (auto Err =
            compileStage(Stages::Mesh, Desc.MS, "mesh", MSIR, MSLib, MSFn))
      return Err;

    MetalIR ASIR;
    MTLPtr<MTL::Library> ASLib;
    MTLPtr<MTL::Function> ASFn;
    if (Desc.AS) {
      if (auto Err = compileStage(Stages::Amplification, *Desc.AS,
                                  "amplification", ASIR, ASLib, ASFn))
        return Err;
    }

    MetalIR PSIR;
    MTLPtr<MTL::Library> PSLib;
    MTLPtr<MTL::Function> PSFn;
    if (Desc.PS) {
      if (auto Err = compileStage(Stages::Pixel, *Desc.PS, "fragment", PSIR,
                                  PSLib, PSFn))
        return Err;
    }

    MTL::MeshRenderPipelineDescriptor *MSPDesc =
        MTL::MeshRenderPipelineDescriptor::alloc()->init();
    auto DescScope = llvm::scope_exit([&] { MSPDesc->release(); });

    MSPDesc->setMeshFunction(MSFn.get());
    if (ASFn)
      MSPDesc->setObjectFunction(ASFn.get());
    if (PSFn)
      MSPDesc->setFragmentFunction(PSFn.get());

    for (size_t I = 0; I < Desc.RTFormats.size(); ++I) {
      MTL::RenderPipelineColorAttachmentDescriptor *RPCA =
          MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init();
      RPCA->setPixelFormat(getMetalPixelFormat(Desc.RTFormats[I]));
      MSPDesc->colorAttachments()->setObject(RPCA, I);
      RPCA->release();
    }

    if (Desc.DSFormat) {
      const MTL::PixelFormat DSPixelFormat =
          getMetalPixelFormat(*Desc.DSFormat);
      MSPDesc->setDepthAttachmentPixelFormat(DSPixelFormat);
      if (isStencilFormat(*Desc.DSFormat))
        MSPDesc->setStencilAttachmentPixelFormat(DSPixelFormat);
    }

    MTL::RenderPipelineState *PSO = Device->newRenderPipelineState(
        MSPDesc, MTL::PipelineOptionNone, /*reflection=*/nullptr, &Error);
    if (Error)
      return toError(Error);

    MTL::DepthStencilDescriptor *DSDesc =
        MTL::DepthStencilDescriptor::alloc()->init();
    DSDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    DSDesc->setDepthWriteEnabled(true);
    MTL::DepthStencilState *DSState = Device->newDepthStencilState(DSDesc);
    DSDesc->release();

    // Pull threads-per-threadgroup from shader reflection.
    MTL::Size MeshTGSize(1, 1, 1);
    {
      IRVersionedMSInfo MSInfo;
      if (IRShaderReflectionCopyMeshInfo(MSIR.Reflection.get(),
                                         IRReflectionVersion_1_0, &MSInfo)) {
        MeshTGSize = MTL::Size(MSInfo.info_1_0.num_threads[0],
                               MSInfo.info_1_0.num_threads[1],
                               MSInfo.info_1_0.num_threads[2]);
      }
      IRShaderReflectionReleaseMeshInfo(&MSInfo);
    }

    MTL::Size ObjectTGSize(1, 1, 1);
    if (Desc.AS) {
      IRVersionedASInfo ASInfo;
      if (IRShaderReflectionCopyAmplificationInfo(
              ASIR.Reflection.get(), IRReflectionVersion_1_0, &ASInfo)) {
        ObjectTGSize = MTL::Size(ASInfo.info_1_0.num_threads[0],
                                 ASInfo.info_1_0.num_threads[1],
                                 ASInfo.info_1_0.num_threads[2]);
      }
      IRShaderReflectionReleaseAmplificationInfo(&ASInfo);
    }

    return std::make_unique<MTLPipelineState>(
        Name, std::move(RootSig), std::move(ArgBuffer), PSO, DSState,
        MTL::CullModeNone, MeshTGSize, ObjectTGSize);
  }

  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<TriangleGeometryDesc> Triangles) override {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this device.");

    if (auto Err = validateBLASGeometry(Triangles))
      return Err;

    llvm::SmallVector<MTL::AccelerationStructureGeometryDescriptor *> Descs;
    Descs.reserve(Triangles.size());
    for (const auto &T : Triangles) {
      auto *TD =
          MTL::AccelerationStructureTriangleGeometryDescriptor::alloc()->init();
      auto *VB = llvm::cast<MTLBuffer>(T.VertexBuffer);
      TD->setVertexBuffer(VB->Buf);
      TD->setVertexBufferOffset(T.VertexBufferOffset);
      TD->setVertexStride(T.VertexStride);
      TD->setVertexFormat(getMetalPositionFormat(T.VertexFormat));
      TD->setTriangleCount(T.IndexBuffer ? T.IndexCount / 3
                                         : T.VertexCount / 3);
      if (T.IndexBuffer) {
        auto *IB = llvm::cast<MTLBuffer>(T.IndexBuffer);
        TD->setIndexBuffer(IB->Buf);
        TD->setIndexBufferOffset(T.IndexBufferOffset);
        TD->setIndexType(getMetalIndexType(T.IdxFormat));
      }
      TD->setOpaque(T.Opaque);
      Descs.push_back(TD);
    }

    AccelerationStructureSizes Sizes = queryBLASPrebuildSize(Descs);
    for (auto *D : Descs)
      D->release();
    return Sizes;
  }

  llvm::Expected<AccelerationStructureSizes>
  getBLASBuildSizes(llvm::ArrayRef<AABBGeometryDesc> AABBs) override {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this device.");

    if (auto Err = validateBLASGeometry(AABBs))
      return Err;

    llvm::SmallVector<MTL::AccelerationStructureGeometryDescriptor *> Descs;
    Descs.reserve(AABBs.size());
    for (const auto &A : AABBs) {
      auto *AD =
          MTL::AccelerationStructureBoundingBoxGeometryDescriptor::alloc()
              ->init();
      auto *BB = llvm::cast<MTLBuffer>(A.AABBBuffer);
      AD->setBoundingBoxBuffer(BB->Buf);
      AD->setBoundingBoxBufferOffset(A.AABBBufferOffset);
      AD->setBoundingBoxStride(A.AABBStride);
      AD->setBoundingBoxCount(A.AABBCount);
      AD->setOpaque(A.Opaque);
      Descs.push_back(AD);
    }

    AccelerationStructureSizes Sizes = queryBLASPrebuildSize(Descs);
    for (auto *D : Descs)
      D->release();
    return Sizes;
  }

private:
  AccelerationStructureSizes queryBLASPrebuildSize(
      llvm::ArrayRef<MTL::AccelerationStructureGeometryDescriptor *> Descs) {
    NS::Array *GeomDescs = NS::Array::array(
        reinterpret_cast<NS::Object *const *>(Descs.data()), Descs.size());

    auto *Descriptor =
        MTL::PrimitiveAccelerationStructureDescriptor::alloc()->init();
    Descriptor->setGeometryDescriptors(GeomDescs);

    MTL::AccelerationStructureSizes Sizes =
        Device->accelerationStructureSizes(Descriptor);

    Descriptor->release();

    return {Sizes.accelerationStructureSize, Sizes.buildScratchBufferSize,
            Sizes.refitScratchBufferSize};
  }

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  allocateAS(const AccelerationStructureSizes &Sizes, const char *Kind) {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this device.");

    MTL::AccelerationStructure *AS =
        Device->newAccelerationStructure(Sizes.ResultDataMaxSizeInBytes);
    if (!AS)
      return llvm::createStringError(
          std::make_error_code(std::errc::not_enough_memory),
          "Failed to create Metal " + llvm::Twine(Kind) + ".");
    return std::make_unique<MetalAccelerationStructure>(AS, Sizes);
  }

public:
  llvm::Expected<AccelerationStructureSizes>
  getTLASBuildSizes(uint32_t InstanceCount) override {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this device.");

    auto *Descriptor =
        MTL::InstanceAccelerationStructureDescriptor::alloc()->init();
    Descriptor->setInstanceCount(InstanceCount);
    // UserID descriptor type so per-instance InstanceID survives the
    // build and is returned by HLSL CommittedInstanceID()/InstanceIndex()
    // semantics on the shader side.
    Descriptor->setInstanceDescriptorType(
        MTL::AccelerationStructureInstanceDescriptorTypeUserID);

    MTL::AccelerationStructureSizes Sizes =
        Device->accelerationStructureSizes(Descriptor);

    Descriptor->release();

    return AccelerationStructureSizes{Sizes.accelerationStructureSize,
                                      Sizes.buildScratchBufferSize,
                                      Sizes.refitScratchBufferSize};
  }

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createBLAS(const AccelerationStructureSizes &Sizes) override {
    return allocateAS(Sizes, "BLAS");
  }

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createTLAS(const AccelerationStructureSizes &Sizes) override {
    return allocateAS(Sizes, "TLAS");
  }

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState IS;

    auto CBOrErr = MTLCommandBuffer::create(GraphicsQueue.Queue);
    if (!CBOrErr)
      return CBOrErr.takeError();
    IS.CB = std::move(*CBOrErr);
    IS.CB->Dev = this;

    if (auto Err = createDescriptorHeap(P, IS))
      return Err;

    if (auto Err = createBuffers(P, IS))
      return Err;

    if (!P.AccelStructs.BLAS.empty() || !P.AccelStructs.TLAS.empty()) {
      auto EncOrErr = IS.CB->createComputeEncoder();
      if (!EncOrErr)
        return EncOrErr.takeError();
      if (auto Err = offloadtest::buildPipelineAccelerationStructures(
              *this, **EncOrErr, P, IS.BLASes, IS.TLASes, IS.ASInputBuffers))
        return Err;
      (*EncOrErr)->endEncoding();
    }

    BindingsDesc Bindings = {};
    for (auto &S : P.Sets) {
      DescriptorSetLayoutDesc Layout;
      for (auto &R : S.Resources) {
        ResourceBindingDesc ResourceBinding = {};
        ResourceBinding.Kind = R.Kind;
        ResourceBinding.DXBinding.Register = R.DXBinding.Register;
        ResourceBinding.DXBinding.Space = R.DXBinding.Space;
        ResourceBinding.VKBinding = R.VKBinding;
        ResourceBinding.DescriptorCount = R.getArraySize();
        Layout.ResourceBindings.push_back(ResourceBinding);
      }
      Bindings.DescriptorSetDescs.push_back(Layout);
    }

    if (P.isCompute()) {
      if (P.Shaders.size() != 1 || P.Shaders[0].Stage != Stages::Compute)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Compute pipeline must have exactly one compute shader.");

      ShaderContainer CS = {};
      CS.EntryPoint = P.Shaders[0].Entry;
      CS.Shader = P.Shaders[0].Shader.get();

      auto PipelineStateOrErr =
          createPipelineCs("Compute Pipeline State", Bindings, CS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      IS.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Compute Pipeline created.\n";

      if (auto Err = createComputeCommands(P, IS))
        return Err;
    } else if (P.isRaster()) {
      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);

      llvm::SmallVector<Format> RTFormats;
      if (!FormatOrErr)
        return FormatOrErr.takeError();
      RTFormats.push_back(*FormatOrErr);

      if (P.isTraditionalRaster()) {
        TraditionalRasterPipelineCreateDesc PipelineDesc = {};
        PipelineDesc.Topology = P.Bindings.Topology;
        PipelineDesc.DSFormat = Format::D32FloatS8Uint;
        PipelineDesc.RTFormats = RTFormats;
        for (auto &Shader : P.Shaders) {
          ShaderContainer SC = {};
          SC.EntryPoint = Shader.Entry;
          SC.Shader = Shader.Shader.get();
          PipelineDesc.setShader(Shader.Stage, std::move(SC));
        }

        for (auto &Attr : P.Bindings.VertexAttributes) {
          auto FormatOrErr = toFormat(Attr.Format, Attr.Channels);
          if (!FormatOrErr)
            return FormatOrErr.takeError();

          InputLayoutDesc Layout = {};
          Layout.Name = Attr.Name;
          Layout.Fmt = *FormatOrErr;
          Layout.OffsetInBytes = Attr.Offset;
          PipelineDesc.InputLayout.push_back(Layout);
        }

        auto PipelineStateOrErr = createTraditionalRasterPipeline(
            "Graphics Pipeline State", Bindings, PipelineDesc);
        if (!PipelineStateOrErr)
          return PipelineStateOrErr.takeError();
        IS.Pipeline = std::move(*PipelineStateOrErr);
      } else if (P.isMeshShaderRaster()) {
        MeshShaderRasterPipelineCreateDesc PipelineDesc = {};
        PipelineDesc.Topology = P.Bindings.Topology;
        PipelineDesc.DSFormat = Format::D32FloatS8Uint;
        PipelineDesc.RTFormats = RTFormats;
        for (auto &Shader : P.Shaders) {
          ShaderContainer SC = {};
          SC.EntryPoint = Shader.Entry;
          SC.Shader = Shader.Shader.get();
          PipelineDesc.setShader(Shader.Stage, std::move(SC));
        }

        auto PipelineStateOrErr = createMeshShaderRasterPipeline(
            "Mesh Shader Pipeline State", Bindings, PipelineDesc);
        if (!PipelineStateOrErr)
          return PipelineStateOrErr.takeError();
        IS.Pipeline = std::move(*PipelineStateOrErr);
        llvm::outs() << "Mesh Shader Pipeline created.\n";
      }

      ColorAttachmentFormatDesc ColorAttachment = {};
      ColorAttachment.Fmt = *FormatOrErr;
      ColorAttachment.Load = LoadAction::Clear;
      ColorAttachment.Store = StoreAction::Store;

      DepthStencilAttachmentFormatDesc DSAttachment = {};
      DSAttachment.Fmt = Format::D32FloatS8Uint;
      DSAttachment.DepthLoad = LoadAction::Clear;
      DSAttachment.DepthStore = StoreAction::Store;
      DSAttachment.StencilLoad = LoadAction::DontCare;
      DSAttachment.StencilStore = StoreAction::DontCare;

      RenderPassDesc PassDesc;
      PassDesc.ColorAttachments.push_back(ColorAttachment);
      PassDesc.DepthStencil = DSAttachment;

      auto RenderPassOrErr = createRenderPass(PassDesc);
      if (!RenderPassOrErr)
        return RenderPassOrErr.takeError();
      IS.RenderPass = std::move(*RenderPassOrErr);

      if (auto Err = createGraphicsCommands(P, IS))
        return Err;
    } else if (P.isRayTracing()) {
      return llvm::createStringError(
          "RayTracing pipeline not yet supported on Metal");
    }

    auto SubmitResult = GraphicsQueue.submit(std::move(IS.CB));
    if (!SubmitResult)
      return SubmitResult.takeError();

    if (auto Err = SubmitResult->waitForCompletion())
      return Err;

    if (auto Err = copyBack(P, IS))
      return Err;
    return llvm::Error::success();
  }

  virtual ~MTLDevice() {};

private:
  void queryCapabilities() {
    // GPU Family Metal3 (macOS 13+) is where mesh shaders became available.
    const bool MeshShaderSupported =
        Device->supportsFamily(MTL::GPUFamilyMetal3);
    Caps.insert(std::make_pair(
        "MeshShader", makeCapability<bool>("MeshShader", MeshShaderSupported)));
    Caps.insert(
        std::make_pair("supportsRaytracing",
                       makeCapability<bool>("supportsRaytracing",
                                            Device->supportsRaytracing())));
  }
};

size_t MTLBuffer::querySparseTileSizeInBytes(const Device &Dev) const {
  return llvm::cast<MTLDevice>(Dev).Device->sparseTileSizeInBytes();
}

TileShape MTLTexture::querySparseTileShape(const Device &Dev) const {
  // The owning device is recovered from the texture so it needn't store it.
  const MTL::Size S = llvm::cast<MTLDevice>(Dev).Device->sparseTileSize(
      Tex->textureType(), Tex->pixelFormat(), Tex->sampleCount());
  return TileShape{static_cast<uint32_t>(S.width),
                   static_cast<uint32_t>(S.height),
                   static_cast<uint32_t>(S.depth)};
}

llvm::Error MTLComputeEncoder::batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) {
  if (Items.empty())
    return llvm::Error::success();
  if (!CB || !CB->Dev)
    return llvm::createStringError(
        std::errc::not_supported,
        "Metal command buffer has no associated MTLDevice.");
  MTL::Device *MTLDev = CB->Dev->Device;
  if (!MTLDev->supportsRaytracing())
    return llvm::createStringError(
        std::errc::not_supported,
        "Ray tracing is not supported on this Metal device.");

  if (auto Err = ensureASEncoder())
    return Err;

  for (const auto &Item : Items) {
    MetalAccelerationStructure *AS = nullptr;
    MTL::AccelerationStructureDescriptor *Desc = nullptr;
    uint64_t ScratchSize = 0;

    if (const auto *BLAS = llvm::dyn_cast<const BLASBuildRequest *>(Item)) {
      AS = llvm::cast<MetalAccelerationStructure>(BLAS->AS);
      llvm::SmallVector<MTL::AccelerationStructureGeometryDescriptor *> Geoms;
      if (const auto *Tris =
              std::get_if<llvm::SmallVector<TriangleGeometryDesc>>(
                  &BLAS->Geometry)) {
        Geoms.reserve(Tris->size());
        for (const auto &T : *Tris) {
          auto *TD =
              MTL::AccelerationStructureTriangleGeometryDescriptor::alloc()
                  ->init();
          auto *VB = llvm::cast<MTLBuffer>(T.VertexBuffer);
          TD->setVertexBuffer(VB->Buf);
          TD->setVertexBufferOffset(T.VertexBufferOffset);
          TD->setVertexStride(T.VertexStride);
          TD->setVertexFormat(getMetalPositionFormat(T.VertexFormat));
          TD->setTriangleCount(T.IndexBuffer ? T.IndexCount / 3
                                             : T.VertexCount / 3);
          if (T.IndexBuffer) {
            auto *IB = llvm::cast<MTLBuffer>(T.IndexBuffer);
            TD->setIndexBuffer(IB->Buf);
            TD->setIndexBufferOffset(T.IndexBufferOffset);
            TD->setIndexType(getMetalIndexType(T.IdxFormat));
          }
          TD->setOpaque(T.Opaque);
          Geoms.push_back(TD);
        }
      } else {
        const auto &AABBs =
            std::get<llvm::SmallVector<AABBGeometryDesc>>(BLAS->Geometry);
        Geoms.reserve(AABBs.size());
        for (const auto &A : AABBs) {
          auto *AD =
              MTL::AccelerationStructureBoundingBoxGeometryDescriptor::alloc()
                  ->init();
          auto *BB = llvm::cast<MTLBuffer>(A.AABBBuffer);
          AD->setBoundingBoxBuffer(BB->Buf);
          AD->setBoundingBoxBufferOffset(A.AABBBufferOffset);
          AD->setBoundingBoxStride(A.AABBStride);
          AD->setBoundingBoxCount(A.AABBCount);
          AD->setOpaque(A.Opaque);
          Geoms.push_back(AD);
        }
      }
      auto *PD = MTL::PrimitiveAccelerationStructureDescriptor::alloc()->init();
      NS::Array *GeomArr = NS::Array::array(
          reinterpret_cast<NS::Object *const *>(Geoms.data()), Geoms.size());
      PD->setGeometryDescriptors(GeomArr);
      Desc = PD;
      ScratchSize = BLAS->AS->getSizes().ScratchDataSizeInBytes;
      for (auto *G : Geoms)
        G->release();
    } else {
      const auto *TLAS = llvm::cast<const TLASBuildRequest *>(Item);
      AS = llvm::cast<MetalAccelerationStructure>(TLAS->AS);

      // Metal's MTLAccelerationStructureInstanceDescriptor references BLASes
      // by index into a separate `instancedAccelerationStructures` array,
      // not by GPU address. Deduplicate the BLAS pointers and remember
      // their indices.
      llvm::SmallVector<MTL::AccelerationStructure *> UniqueBLASes;
      llvm::SmallVector<uint32_t> InstanceASIdx;
      InstanceASIdx.reserve(TLAS->Instances.size());
      for (const auto &Inst : TLAS->Instances) {
        auto *MTLBLAS = llvm::cast<MetalAccelerationStructure>(Inst.BLAS);
        auto It = std::find(UniqueBLASes.begin(), UniqueBLASes.end(),
                            MTLBLAS->AccelStruct);
        uint32_t Idx;
        if (It == UniqueBLASes.end()) {
          Idx = static_cast<uint32_t>(UniqueBLASes.size());
          UniqueBLASes.push_back(MTLBLAS->AccelStruct);
        } else {
          Idx = static_cast<uint32_t>(It - UniqueBLASes.begin());
        }
        InstanceASIdx.push_back(Idx);
      }

      // Pack instance descriptors. Layout differs from VK/DX12: 32-byte
      // entries with an index instead of a GPU address.
      llvm::SmallVector<MTL::AccelerationStructureUserIDInstanceDescriptor>
          Native;
      Native.reserve(TLAS->Instances.size());
      for (size_t I = 0; I < TLAS->Instances.size(); ++I) {
        const auto &Src = TLAS->Instances[I];
        MTL::AccelerationStructureUserIDInstanceDescriptor D = {};
        // Metal stores transform as packed 4x3 column-major; our high-level
        // Transform[3][4] is row-major. Transpose into Metal's layout.
        for (int Row = 0; Row < 3; ++Row)
          for (int Col = 0; Col < 4; ++Col)
            D.transformationMatrix.columns[Col][Row] = Src.Transform[Row][Col];
        D.options = MTL::AccelerationStructureInstanceOptionNone;
        D.mask = Src.InstanceMask;
        D.intersectionFunctionTableOffset = 0;
        D.accelerationStructureIndex = InstanceASIdx[I];
        D.userID = Src.InstanceID;
        Native.push_back(D);
      }
      const size_t InstByteSize =
          Native.size() *
          sizeof(MTL::AccelerationStructureUserIDInstanceDescriptor);

      const BufferCreateDesc UploadDesc = BufferCreateDesc::uploadBuffer();
      auto InstBufOrErr = offloadtest::createBufferWithData(
          *CB->Dev, "TLAS-Instances", UploadDesc, Native.data(), InstByteSize,
          nullptr, nullptr);
      if (!InstBufOrErr)
        return InstBufOrErr.takeError();
      auto *MTLInstBuf = llvm::cast<MTLBuffer>(InstBufOrErr->get());
      CB->KeepAliveOwned.push_back(std::move(*InstBufOrErr));

      auto *ID = MTL::InstanceAccelerationStructureDescriptor::alloc()->init();
      ID->setInstanceDescriptorBuffer(MTLInstBuf->Buf);
      ID->setInstanceCount(TLAS->Instances.size());
      ID->setInstanceDescriptorType(
          MTL::AccelerationStructureInstanceDescriptorTypeUserID);
      NS::Array *BLASArr = NS::Array::array(
          reinterpret_cast<NS::Object *const *>(UniqueBLASes.data()),
          UniqueBLASes.size());
      ID->setInstancedAccelerationStructures(BLASArr);
      Desc = ID;
      ScratchSize = TLAS->AS->getSizes().ScratchDataSizeInBytes;
    }

    const BufferCreateDesc ScratchDesc = BufferCreateDesc::scratchBuffer();
    auto ScratchOrErr =
        CB->Dev->createBuffer("AS-Scratch", ScratchDesc, ScratchSize);
    if (!ScratchOrErr) {
      Desc->release();
      return ScratchOrErr.takeError();
    }
    auto *MTLScratch = llvm::cast<MTLBuffer>(ScratchOrErr->get());
    CB->KeepAliveOwned.push_back(std::move(*ScratchOrErr));

    insertDebugSignpost("BuildAccelerationStructure");
    ASEnc->buildAccelerationStructure(AS->AccelStruct, Desc, MTLScratch->Buf,
                                      0);
    Desc->release();
  }

  return llvm::Error::success();
}
} // namespace

llvm::Error offloadtest::initializeMetalDevices(
    const DeviceConfig /*Config*/,
    llvm::SmallVectorImpl<std::unique_ptr<Device>> &Devices) {
  MTL::Device *MetalDevice = MTL::CreateSystemDefaultDevice();
  MTL::CommandQueue *MetalQueue = MetalDevice->newCommandQueue();

  auto SubmitFenceOrErr = MTLFence::create(MetalDevice, "QueueSubmitFence");
  if (!SubmitFenceOrErr)
    return SubmitFenceOrErr.takeError();

  auto DefaultDev = std::make_unique<MTLDevice>(MetalDevice, MetalQueue,
                                                std::move(*SubmitFenceOrErr));
  Devices.push_back(std::move(DefaultDev));

  return llvm::Error::success();
}
