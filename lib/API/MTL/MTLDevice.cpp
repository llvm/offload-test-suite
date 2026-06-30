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
#include "API/Util.h"
#include "MTLDescriptorHeap.h"
#include "MTLResources.h"
#include "MTLTopLevelArgumentBuffer.h"
#include "ResidencyTracker.h"
#include "Support/Pipeline.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include "../Support/OffloadMigration.h"

#include <algorithm>
#include <memory>

using namespace offloadtest;

static constexpr MTL::RenderStages AllRenderStages =
    MTL::RenderStageVertex | MTL::RenderStageFragment | MTL::RenderStageTile |
    MTL::RenderStageObject | MTL::RenderStageMesh;

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
    return IRShaderStageRayGeneration;
  case Stages::Miss:
    return IRShaderStageMiss;
  case Stages::ClosestHit:
    return IRShaderStageClosestHit;
  case Stages::AnyHit:
    return IRShaderStageAnyHit;
  case Stages::Intersection:
    return IRShaderStageIntersection;
  case Stages::Callable:
    return IRShaderStageCallable;
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

struct MetalDescriptorSet {
  IRDescriptorTableEntry *CSUHandle;
  MTLGPUDescriptorHandle CSUHandleGPU;
};

class MetalDescriptorPool : public DescriptorPool {
public:
  std::unique_ptr<MTLDescriptorHeap> CSUHeap;
  std::atomic<uint32_t> CSUAllocator = 0;

  MetalDescriptorPool(std::unique_ptr<MTLDescriptorHeap> CSUHeap)
      : DescriptorPool(GPUAPI::Metal), CSUHeap(std::move(CSUHeap)) {}

  static llvm::Expected<std::unique_ptr<DescriptorPool>>
  create(MTL::Device *Dev,
         const std::shared_ptr<MetalResidencyTracker> &ResidencyTracker) {
    const uint32_t DescriptorCount = 4096;

    const MTLDescriptorHeapDesc HeapDesc = {MTLDescriptorHeapType::CBV_SRV_UAV,
                                            DescriptorCount};

    auto DescHeapOrErr =
        MTLDescriptorHeap::create(Dev, HeapDesc, ResidencyTracker);
    if (!DescHeapOrErr)
      return DescHeapOrErr.takeError();

    return std::make_unique<MetalDescriptorPool>(std::move(*DescHeapOrErr));
  }

  void allocateDescriptors(uint32_t Count, IRDescriptorTableEntry *&CPU,
                           MTLGPUDescriptorHandle &GPU) {
    const uint32_t Offset = CSUAllocator.fetch_add(Count);
    CPU = CSUHeap->getEntryHandle(Offset);
    GPU = CSUHeap->getGPUDescriptorHandleForHeapStart().addOffset(Offset);
  }

  void reset() override { CSUAllocator.store(0); }

  static bool classof(const DescriptorPool *P) {
    return P->getAPI() == GPUAPI::Metal;
  }
};

class MetalDescriptorSets : public DescriptorSets {
public:
  llvm::SmallVector<MetalDescriptorSet> Sets;
  MetalDescriptorSets(llvm::SmallVector<MetalDescriptorSet> Sets)
      : DescriptorSets(GPUAPI::Metal), Sets(std::move(Sets)) {}

  static bool classof(const DescriptorSets *S) {
    return S->getAPI() == GPUAPI::Metal;
  }
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
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;

  // Batches of command buffers submitted to the GPU that may still be
  // in-flight.  Each batch records the fence value it signals so we can
  // non-blockingly query progress and release completed batches.
  struct InFlightBatch {
    uint64_t FenceValue;
    llvm::SmallVector<std::unique_ptr<offloadtest::CommandBuffer>> CBs;
  };
  llvm::SmallVector<InFlightBatch> InFlightBatches;

  MTLQueue(MTL::CommandQueue *Queue, std::unique_ptr<MTLFence> SubmitFence,
           std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : Queue(Queue), SubmitFence(std::move(SubmitFence)),
        ResidencyTracker(std::move(ResidencyTracker)) {
    Queue->addResidencySet(this->ResidencyTracker->ResidencySet);
  }
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

struct RootSignatureLayout {
  uint32_t IsSampler : 1;
  uint32_t Count : 31;

  RootSignatureLayout(bool IsSampler, uint32_t Count)
      : IsSampler(IsSampler), Count(Count) {}
  RootSignatureLayout() = delete;
};

struct DescriptorCountPair {
  uint32_t DescriptorCount;
  uint32_t SamplerCount;
};

struct DescriptorSetsLayout {
  llvm::SmallVector<RootSignatureLayout> RSigLayout;
  llvm::SmallVector<DescriptorCountPair> Sets;
};

class MTLPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  IRRootSignaturePtr RootSig;
  std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
  DescriptorSetsLayout Layout;
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

  // True for pipelines created via createPipelineRT; mirrors the VK / DX
  // backends' IsRayTracing flag so classof can downcast safely.
  bool IsRayTracing = false;

  MTLPipelineState(llvm::StringRef Name, IRRootSignaturePtr RootSig,
                   std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer,
                   DescriptorSetsLayout Layout,
                   MTL::ComputePipelineState *ComputePipeline,
                   MTL::Size ThreadsPerGroup)
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        RootSig(std::move(RootSig)), ArgBuffer(std::move(ArgBuffer)),
        Layout(std::move(Layout)), ComputePipeline(ComputePipeline),
        ThreadsPerGroup(ThreadsPerGroup) {}

  MTLPipelineState(llvm::StringRef Name, IRRootSignaturePtr RootSig,
                   std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer,
                   DescriptorSetsLayout Layout,
                   MTL::RenderPipelineState *RenderPipeline,
                   MTL::DepthStencilState *DepthStencilState,
                   MTL::CullMode CullMode,
                   MTL::Size MeshThreadsPerThreadgroup = {1, 1, 1},
                   MTL::Size ObjectThreadsPerThreadgroup = {1, 1, 1})
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        RootSig(std::move(RootSig)), ArgBuffer(std::move(ArgBuffer)),
        Layout(std::move(Layout)), RenderPipeline(RenderPipeline),
        DepthStencilState(DepthStencilState), CullMode(CullMode),
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

protected:
  // RT subclass constructor — keeps Compute/RenderPipeline null while sharing
  // the rest of the layout (Name, root signature, argument buffer).
  MTLPipelineState(llvm::StringRef Name, IRRootSignaturePtr RootSig,
                   std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer,
                   DescriptorSetsLayout Layout, bool IsRT)
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        RootSig(std::move(RootSig)), ArgBuffer(std::move(ArgBuffer)),
        Layout(std::move(Layout)), IsRayTracing(IsRT) {}
};

/// Ray tracing pipeline state. Layered on top of MTLPipelineState so the
/// existing argument-buffer / root-signature plumbing keeps working; adds the
/// raygen compute pipeline (held in ComputePipeline) plus the
/// IRShaderIdentifier records the SBT builder needs to populate per-record
/// entries.
///
/// The Metal RT path goes through `metal_irconverter`:
///   • each entry point is compiled to a Metal IR function;
///   • raygen is compiled as a kernel (IRRayGenerationCompilationKernel) so
///     it becomes the compute function of the pipeline;
///   • miss / closest-hit / any-hit / intersection / callable are compiled as
///     visible functions, attached to the pipeline via LinkedFunctions, and
///     looked up by name in a MTLVisibleFunctionTable;
///   • the SBT records IRShaderIdentifier values whose `shaderHandle` is the
///     slot in that visible function table.
class MTLRayTracingPipelineState : public MTLPipelineState {
public:
  // ResourceID-based callable tables wired into IRDispatchRaysArgument.
  MTL::VisibleFunctionTable *VFT = nullptr;
  MTL::IntersectionFunctionTable *IFT = nullptr;
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;

  // Per shader entry / hit-group: pre-built IRShaderIdentifier the SBT
  // builder memcpys into each record. Keys are EntryPoint strings for
  // raygen / miss / callable shaders and HitGroup.Name for hit groups.
  llvm::StringMap<IRShaderIdentifier> ShaderIdentifiers;

  // Keep the per-stage Metal libraries / functions alive as long as the
  // pipeline owns the visible-function-table indices that reference them.
  llvm::SmallVector<MTL::Library *> Libraries;
  llvm::SmallVector<MTL::Function *> Functions;

  MTLRayTracingPipelineState(
      llvm::StringRef Name, IRRootSignaturePtr RootSig,
      std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuf,
      DescriptorSetsLayout Layout,
      std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : MTLPipelineState(Name, std::move(RootSig), std::move(ArgBuf),
                         std::move(Layout),
                         /*IsRT=*/true),
        ResidencyTracker(std::move(ResidencyTracker)) {}

  ~MTLRayTracingPipelineState() override {
    ResidencyTracker->withLock([&](MTL::ResidencySet *RS) {
      if (VFT)
        RS->removeAllocation(VFT);
      if (IFT)
        RS->removeAllocation(IFT);
    });

    if (VFT)
      VFT->release();
    if (IFT)
      IFT->release();
    for (MTL::Function *F : Functions)
      if (F)
        F->release();
    for (MTL::Library *L : Libraries)
      if (L)
        L->release();
  }

  static bool classof(const offloadtest::PipelineState *B) {
    if (B->getAPI() != GPUAPI::Metal)
      return false;
    return static_cast<const MTLPipelineState *>(B)->IsRayTracing;
  }
};

/// Metal-side shader binding table. There is no `MTLShaderBindingTable` in
/// the Metal API — the irconverter runtime expects the four SBT regions to be
/// laid out as `IRShaderIdentifier` records in a single buffer whose ranges
/// are referenced from an `IRDispatchRaysArgument` struct at dispatch time.
class MTLShaderBindingTable : public offloadtest::ShaderBindingTable {
public:
  MTL::Buffer *Buffer = nullptr;
  IRVirtualAddressRange RayGenRegion{};
  IRVirtualAddressRangeAndStride MissRegion{};
  IRVirtualAddressRangeAndStride HitGroupRegion{};
  IRVirtualAddressRangeAndStride CallableRegion{};
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;

  MTLShaderBindingTable(MTL::Buffer *Buf, IRVirtualAddressRange RG,
                        IRVirtualAddressRangeAndStride MS,
                        IRVirtualAddressRangeAndStride HG,
                        IRVirtualAddressRangeAndStride CL,
                        std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : offloadtest::ShaderBindingTable(GPUAPI::Metal), Buffer(Buf),
        RayGenRegion(RG), MissRegion(MS), HitGroupRegion(HG),
        CallableRegion(CL), ResidencyTracker(std::move(ResidencyTracker)) {

    this->ResidencyTracker->withLock(
        [&](MTL::ResidencySet *RS) { RS->addAllocation(Buffer); });
  }

  ~MTLShaderBindingTable() override {
    if (Buffer) {
      ResidencyTracker->withLock(
          [&](MTL::ResidencySet *RS) { RS->removeAllocation(Buffer); });
      Buffer->release();
    }
  }

  static bool classof(const offloadtest::ShaderBindingTable *S) {
    return S->getAPI() == GPUAPI::Metal;
  }
};

class MTLBuffer : public offloadtest::Buffer {
public:
  MTL::Resource
      *Resource; // MTL::Texture* for typed buffer, otherwise MTL::Buffer*
  std::string Name;
  BufferCreateDesc Desc;
  size_t SizeInBytes;
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;

  MTLBuffer(MTL::Resource *Resource, llvm::StringRef Name,
            BufferCreateDesc Desc, size_t SizeInBytes,
            std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : offloadtest::Buffer(GPUAPI::Metal), Resource(Resource), Name(Name),
        Desc(Desc), SizeInBytes(SizeInBytes),
        ResidencyTracker(std::move(ResidencyTracker)) {
    this->ResidencyTracker->withLock(
        [&](MTL::ResidencySet *RS) { RS->addAllocation(Resource); });
  }
  MTLBuffer(const MTLBuffer &) = delete;
  MTLBuffer(MTLBuffer &&) = delete;
  MTLBuffer &operator=(const MTLBuffer &) = delete;
  MTLBuffer &operator=(MTLBuffer &&) = delete;

  size_t getSizeInBytes() const override { return SizeInBytes; }

  size_t querySparseTileSizeInBytes(const Device &Dev) const override;

  // Only valid for non-typed buffers
  MTL::Buffer *getBufferPtr() const {
    assert(Desc.AccessType != BufferShaderAccessType::Typed);
    return static_cast<MTL::Buffer *>(Resource);
  }

  // Only valid for typed buffers
  MTL::Texture *getTexturePtr() const {
    assert(Desc.AccessType == BufferShaderAccessType::Typed);
    return static_cast<MTL::Texture *>(Resource);
  }

  llvm::Expected<void *> map() override {
    if (Desc.Location == MemoryLocation::GpuOnly)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "Cannot map a GpuOnly buffer.");
    if (Desc.AccessType == BufferShaderAccessType::Typed)
      return llvm::createStringError(
          std::errc::not_supported,
          "Metal does not support mapping typed buffers.");

    return getBufferPtr()->contents();
  }

  void unmap() override {
    // Managed storage (CpuToGpu) requires an explicit didModifyRange to
    // propagate CPU-side writes to the GPU. Shared storage (GpuToCpu) is
    // coherent and needs no action.
    if (Desc.Location == MemoryLocation::CpuToGpu)
      getBufferPtr()->didModifyRange(NS::Range::Make(0, SizeInBytes));
  }

  ~MTLBuffer() override {
    if (Resource) {
      ResidencyTracker->withLock(
          [&](MTL::ResidencySet *RS) { RS->removeAllocation(Resource); });
      Resource->release();
    }
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
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;

  MTLTexture(MTL::Texture *Tex, llvm::StringRef Name, TextureCreateDesc Desc,
             std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : offloadtest::Texture(GPUAPI::Metal), Tex(Tex), Name(Name), Desc(Desc),
        ResidencyTracker(std::move(ResidencyTracker)) {
    this->ResidencyTracker->withLock(
        [&](MTL::ResidencySet *RS) { RS->addAllocation(Tex); });
  }

  ~MTLTexture() override {
    if (Tex) {
      ResidencyTracker->withLock(
          [&](MTL::ResidencySet *RS) { RS->removeAllocation(Tex); });
      Tex->release();
    }
  }

  TileShape querySparseTileShape(const Device &Dev) const override;

  const TextureCreateDesc &getDesc() const override { return Desc; }

  static bool classof(const offloadtest::Texture *T) {
    return T->getAPI() == GPUAPI::Metal;
  }
};

class MTLSampler : public offloadtest::Sampler {
public:
  SamplerCreateDesc Desc;

  const SamplerCreateDesc &getDesc() const override { return Desc; }

  static bool classof(const offloadtest::Sampler *S) {
    return S->getAPI() == GPUAPI::Metal;
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
  MTL::Fence *Fence = nullptr;
  /// Back-pointer to the owning device; used by encoders that need to
  /// allocate scratch / instance buffers for AS builds.
  MTLDevice *Dev = nullptr;
  /// Buffers that must outlive command-buffer submission (e.g. AS scratch
  /// and TLAS instance buffers used during builds).
  llvm::SmallVector<std::unique_ptr<offloadtest::Buffer>> KeepAliveOwned;

  MTL::Buffer *BoundCSUHeapBuffer = nullptr;

  static llvm::Expected<std::unique_ptr<MTLCommandBuffer>>
  create(MTL::CommandQueue *Queue) {
    auto CB = std::unique_ptr<MTLCommandBuffer>(new MTLCommandBuffer());
    CB->CmdBuffer = Queue->commandBuffer();
    if (!CB->CmdBuffer)
      return llvm::createStringError(std::errc::device_or_resource_busy,
                                     "Failed to create Metal command buffer.");
    return CB;
  }

  ~MTLCommandBuffer() override {
    if (Fence)
      Fence->release();
  }

  MTL::Fence *ensureFence();

  static bool classof(const CommandBuffer *CB) {
    return CB->getKind() == GPUAPI::Metal;
  }

  void bindPool(const DescriptorPool &Pool) override {
    const MetalDescriptorPool &PoolMTL = llvm::cast<MetalDescriptorPool>(Pool);
    BoundCSUHeapBuffer = PoolMTL.CSUHeap->Buffer;
  }

  llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
  createComputeEncoder() override;

  llvm::Expected<std::unique_ptr<offloadtest::RenderEncoder>>
  createRenderEncoder(const offloadtest::RenderPassBeginDesc &Desc) override;

  MTLCommandBuffer() : CommandBuffer(GPUAPI::Metal) {}
};

class MetalAccelerationStructure : public offloadtest::AccelerationStructure {
public:
  MTL::AccelerationStructure *AccelStruct;
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;
  std::unique_ptr<offloadtest::Buffer> HeaderBuffer;  // TLAS Only
  std::unique_ptr<offloadtest::Buffer> ContribBuffer; // TLAS Only

  // BLAS
  MetalAccelerationStructure(
      MTL::AccelerationStructure *AccelStruct,
      const AccelerationStructureSizes &Sizes,
      std::shared_ptr<MetalResidencyTracker> ResidencyTracker)
      : offloadtest::AccelerationStructure(GPUAPI::Metal, Sizes),
        AccelStruct(AccelStruct),
        ResidencyTracker(std::move(ResidencyTracker)) {
    this->ResidencyTracker->withLock(
        [&](MTL::ResidencySet *RS) { RS->addAllocation(AccelStruct); });
  }

  // TLAS
  MetalAccelerationStructure(
      MTL::AccelerationStructure *AccelStruct,
      const AccelerationStructureSizes &Sizes,
      std::shared_ptr<MetalResidencyTracker> ResidencyTracker,
      std::unique_ptr<offloadtest::Buffer> HeaderBuffer,
      std::unique_ptr<offloadtest::Buffer> ContribBuffer)
      : offloadtest::AccelerationStructure(GPUAPI::Metal, Sizes),
        AccelStruct(AccelStruct), ResidencyTracker(std::move(ResidencyTracker)),
        HeaderBuffer(std::move(HeaderBuffer)),
        ContribBuffer(std::move(ContribBuffer)) {
    this->ResidencyTracker->withLock(
        [&](MTL::ResidencySet *RS) { RS->addAllocation(AccelStruct); });
  }

  ~MetalAccelerationStructure() override {
    if (AccelStruct) {
      ResidencyTracker->withLock(
          [&](MTL::ResidencySet *RS) { RS->removeAllocation(AccelStruct); });
      AccelStruct->release();
    }
  }

  static bool classof(const offloadtest::AccelerationStructure *AS) {
    return AS->getAPI() == GPUAPI::Metal;
  }
};

class MetalDescriptorSetsBuilder : public DescriptorSetsBuilder {
public:
  llvm::SmallVector<MetalDescriptorSet> Sets;
  llvm::SmallVector<IRDescriptorTableEntry *> SetStates;

  MetalDescriptorSetsBuilder(llvm::SmallVector<MetalDescriptorSet> Sets)
      : DescriptorSetsBuilder(GPUAPI::Metal), Sets(std::move(Sets)) {
    SetStates.reserve(this->Sets.size());
    for (const auto &Set : this->Sets) {
      SetStates.push_back(Set.CSUHandle);
    }
  }

  DescriptorSetsBuilder &bindBuffers(uint32_t SetIndex,
                                     llvm::ArrayRef<const Buffer *> B) {
    IRDescriptorTableEntry *EntryPtr = SetStates[SetIndex];
    SetStates[SetIndex] += B.size();

    for (size_t I = 0, N = B.size(); I < N; ++I) {
      const MTLBuffer &BufferMTL = llvm::cast<MTLBuffer>(*B[I]);
      if (BufferMTL.Desc.AccessType != BufferShaderAccessType::Typed) {
        IRBufferView View = {};
        View.buffer = BufferMTL.getBufferPtr();
        View.bufferSize = BufferMTL.SizeInBytes;
        IRDescriptorTableSetBufferView(EntryPtr + I, &View);
      } else {
        IRDescriptorTableSetTexture(EntryPtr + I, BufferMTL.getTexturePtr(), 0,
                                    0);
      }
    }

    return *this;
  }

  DescriptorSetsBuilder &bindTextures(uint32_t SetIndex,
                                      llvm::ArrayRef<const Texture *> T,
                                      llvm::ArrayRef<const Sampler *> S) {
    assert((S.empty() || S.size() == T.size()) &&
           "Sampler list must either be empty or match "
           "texture list when binding descriptors.");

    IRDescriptorTableEntry *EntryPtr = SetStates[SetIndex];
    SetStates[SetIndex] += T.size();

    for (size_t I = 0, N = T.size(); I < N; ++I) {
      assert((S.empty() || S[I] == nullptr) &&
             "Metal does not support combined image samplers.");

      const MTLTexture &TextureMTL = llvm::cast<MTLTexture>(*T[I]);
      IRDescriptorTableSetTexture(EntryPtr + I, TextureMTL.Tex, 0, 0);
    }

    return *this;
  }

  DescriptorSetsBuilder &constant(uint32_t SetIndex,
                                  llvm::ArrayRef<const Buffer *> B,
                                  VKBind) override {
    return bindBuffers(SetIndex, B);
  }

  DescriptorSetsBuilder &
  read(uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind) override {
    return bindBuffers(SetIndex, B);
  }
  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const Texture *> T,
                              llvm::ArrayRef<const Sampler *> S,
                              VKBind) override {
    return bindTextures(SetIndex, T, S);
  }
  DescriptorSetsBuilder &read(uint32_t SetIndex,
                              llvm::ArrayRef<const AccelerationStructure *> A,
                              VKBind) override {
    IRDescriptorTableEntry *EntryPtr = SetStates[SetIndex];
    SetStates[SetIndex] += A.size();

    for (size_t I = 0, N = A.size(); I < N; ++I) {
      const MetalAccelerationStructure &AccelStructMTL =
          llvm::cast<MetalAccelerationStructure>(*A[I]);
      const MTLBuffer &HeaderBufferMTL =
          llvm::cast<MTLBuffer>(*AccelStructMTL.HeaderBuffer.get());
      IRDescriptorTableSetAccelerationStructure(
          EntryPtr + I, HeaderBufferMTL.getBufferPtr()->gpuAddress());
    }

    return *this;
  }

  DescriptorSetsBuilder &
  write(uint32_t SetIndex, llvm::ArrayRef<const Buffer *> B, VKBind) override {
    return bindBuffers(SetIndex, B);
  }
  DescriptorSetsBuilder &
  write(uint32_t SetIndex, llvm::ArrayRef<const Texture *> T, VKBind) override {
    return bindTextures(SetIndex, T, {});
  }

  DescriptorSetsBuilder &sampler(uint32_t SetIndex,
                                 llvm::ArrayRef<const Sampler *> S,
                                 VKBind) override {
    assert(false && "Samplers are not implemented on Metal.");
    return *this;
  }

  std::unique_ptr<DescriptorSets> build() override {
    return std::make_unique<MetalDescriptorSets>(std::move(Sets));
  }

  static bool classof(const DescriptorSetsBuilder *B) {
    return B->getAPI() == GPUAPI::Metal;
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

  // Update residency.
  // From a correctness perspective this is the least complex way
  // From a performance perspective this is not ideal, because requestResidency
  // may take some time to execute. Preferably this would be done earlier in the
  // frame.
  ResidencyTracker->withLock([&](MTL::ResidencySet *RS) {
    RS->commit();
    RS->requestResidency();
  });

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

  // Commands that touch resources via descriptors require manual memory
  // bariers. Use addBarrierScope on resource access, call flushBarrier before
  // executing a command that accesses resources via descriptors.
  // Note: This is only valid on ComputeEnc. BlitEnc and ASEnc track hazards
  // automatically.
  void addBarrierScope(MTL::BarrierScope Scope) { PendingScope |= Scope; }
  void flushBarrier() {
    assert(ComputeEnc &&
           "flushBarrier must be called after ensureComputeEncoder");
    if (PendingScope != MTL::BarrierScope(0)) {
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
    if (CB->Fence)
      ComputeEnc->waitForFence(CB->Fence);
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
    if (CB->Fence)
      BlitEnc->waitForFence(CB->Fence);
    return llvm::Error::success();
  }

public:
  MTLComputeEncoder(MTLCommandBuffer *CB, MTL::CommandBuffer *CmdBuffer)
      : ComputeEncoder(GPUAPI::Metal), CB(CB), CmdBuffer(CmdBuffer) {}

  ~MTLComputeEncoder() override { endEncoding(); }

  static bool classof(const CommandEncoder *E) {
    return E->getAPI() == GPUAPI::Metal;
  }

  llvm::Expected<MTL::ComputeCommandEncoder *> getNative() {
    if (auto Err = ensureComputeEncoder())
      return Err;
    return ComputeEnc;
  }

  MTL::CommandEncoder *getActiveEncoder() const {
    if (ComputeEnc)
      return ComputeEnc;
    if (BlitEnc)
      return BlitEnc;
    return ASEnc;
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

  void bindDescriptorSets(const PipelineState &PSO,
                          const DescriptorSets &DSets) {
    const auto &MTLPSO = llvm::cast<MTLPipelineState>(PSO);
    const MetalDescriptorSets &DSetsMTL =
        llvm::cast<MetalDescriptorSets>(DSets);

    if (auto Err = ensureComputeEncoder())
      llvm::report_fatal_error(std::move(Err));

    ComputeEnc->setBuffer(CB->BoundCSUHeapBuffer, 0,
                          kIRDescriptorHeapBindPoint);

    // NOTE(manon): This is a problem when we dispatch the same pipeline
    // multiple times. This buffer should not be living in the PSO.
    for (uint32_t I = 0, N = DSetsMTL.Sets.size(); I < N; ++I) {
      assert(!MTLPSO.Layout.RSigLayout[I].IsSampler &&
             "Descriptor layout doesn't match root signature.");
      MTLPSO.ArgBuffer->setRootDescriptorTable(I,
                                               DSetsMTL.Sets[I].CSUHandleGPU);
    }

    ComputeEnc->setBuffer(MTLPSO.ArgBuffer->Buffer, 0,
                          kIRArgumentBufferBindPoint);
  }

  void bindComputeDescriptorSets(const PipelineState &PSO,
                                 const DescriptorSets &DSets) override {
    return bindDescriptorSets(PSO, DSets);
  }

  void bindRayTracingDescriptorSets(const PipelineState &PSO,
                                    const DescriptorSets &DSets) override {
    return bindDescriptorSets(PSO, DSets);
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

    if (MTLSrc.Desc.AccessType == BufferShaderAccessType::Typed) {
      if (MTLDst.Desc.AccessType == BufferShaderAccessType::Typed) {
        const uint32_t SrcElementSize =
            getFormatSizeInBytes(MTLSrc.Desc.AccessTypeParams.Fmt);
        const uint32_t DstElementSize =
            getFormatSizeInBytes(MTLDst.Desc.AccessTypeParams.Fmt);
        BlitEnc->copyFromTexture(
            MTLSrc.getTexturePtr(), 0 /*sourceSlice (unused)*/,
            0 /*sourceLevel (unused)*/,
            MTL::Origin(SrcOffset / SrcElementSize, 0, 0),
            MTL::Size(Size / SrcElementSize, 1, 1), MTLDst.getTexturePtr(),
            0 /*destinationSlice (unused)*/, 0 /*destinationLevel (unused)*/,
            MTL::Origin(DstOffset / DstElementSize, 0, 0));
      } else {
        const uint32_t ElementSize =
            getFormatSizeInBytes(MTLSrc.Desc.AccessTypeParams.Fmt);
        BlitEnc->copyFromTexture(
            MTLSrc.getTexturePtr(), 0 /*sourceSlice (unused)*/,
            0 /*sourceLevel (unused)*/,
            MTL::Origin(SrcOffset / ElementSize, 0, 0),
            MTL::Size(Size / ElementSize, 1, 1), MTLDst.getBufferPtr(),
            DstOffset, 0 /*destinationBytesPerRow (unused)*/,
            0 /*destinationBytesPerImage (unused)*/);
      }
    } else if (MTLDst.Desc.AccessType == BufferShaderAccessType::Typed) {
      const uint32_t ElementSize =
          getFormatSizeInBytes(MTLDst.Desc.AccessTypeParams.Fmt);
      BlitEnc->copyFromBuffer(
          MTLSrc.getBufferPtr(), SrcOffset, 0 /*sourceBytesPerRow (unused)*/,
          0 /*sourceBytesPerImage (unused)*/,
          MTL::Size(Size / ElementSize, 1, 1), MTLDst.getTexturePtr(),
          0 /*destinationSlice (unused)*/, 0 /*destinationLevel (unused)*/,
          MTL::Origin(DstOffset / ElementSize, 0, 0));
    } else {
      BlitEnc->copyFromBuffer(MTLSrc.getBufferPtr(), SrcOffset,
                              MTLDst.getBufferPtr(), DstOffset, Size);
    }
    return llvm::Error::success();
  }

  llvm::Error copyBufferToTexture(offloadtest::Buffer &Src,
                                  offloadtest::Texture &Dst) override {
    if (auto Err = ensureBlitEncoder())
      return Err;
    auto &MTLSrc = static_cast<MTLBuffer &>(Src);
    auto &MTLDst = static_cast<MTLTexture &>(Dst);
    assert(MTLSrc.Desc.AccessType != BufferShaderAccessType::Typed &&
           "TODO(manon): Support typed buffer copies.");

    // The upload buffer holds tightly packed texel data for every mip level
    // (see createTextureWithData): each mip's rows are contiguous with no
    // padding, and the mips follow one another. Copy one mip at a time, with
    // the source bytes-per-row being that mip's width times the element size.
    const size_t ElemSize = getFormatSizeInBytes(MTLDst.Desc.Fmt);

    insertDebugSignpost(llvm::formatv("copyBufferToTexture {0} -> {1}",
                                      MTLSrc.Name, MTLDst.Name)
                            .str());
    size_t CurrentOffset = 0;
    for (uint32_t I = 0; I < MTLDst.Desc.MipLevels; ++I) {
      const uint32_t MipWidth = std::max(1u, MTLDst.Desc.Width >> I);
      const uint32_t MipHeight = std::max(1u, MTLDst.Desc.Height >> I);
      const size_t RowBytes = MipWidth * ElemSize;
      const size_t ImageBytes = RowBytes * MipHeight;
      BlitEnc->copyFromBuffer(
          MTLSrc.getBufferPtr(), CurrentOffset, RowBytes, ImageBytes,
          MTL::Size(MipWidth, MipHeight, 1), MTLDst.Tex,
          /*destinationSlice=*/0, /*destinationLevel=*/I, MTL::Origin(0, 0, 0));
      CurrentOffset += ImageBytes;
    }
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
    assert(MTLDst.Desc.AccessType != BufferShaderAccessType::Typed &&
           "TODO(manon): Support typed buffer copies.");

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
                             MTL::Origin(0, 0, 0), CopySize,
                             MTLDst.getBufferPtr(),
                             /*destinationOffset=*/0, RowBytes, ImageBytes);
    return llvm::Error::success();
  }

  // Defined out-of-line below — needs MTLDevice's full type for access to the
  // MTL::Device handle (used to allocate scratch and instance buffers).
  llvm::Error batchBuildAS(llvm::ArrayRef<ASBuildItem> Items) override;

  // Dispatch threads using a raygen compute kernel synthesized by the
  // irconverter. All bindings (descriptor heap, top-level argument buffer,
  // IRDispatchRaysArgument at slot 3, visible/intersection function tables,
  // and the SBT buffer) must already be set on the active compute encoder by
  // the caller — this method only binds the pipeline state and issues the
  // dispatch.
  llvm::Error dispatchRays(const PipelineState &PSO,
                           const ShaderBindingTable &SBT, uint32_t Width,
                           uint32_t Height, uint32_t Depth) override;
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
    if (CB->Fence)
      ASEnc->waitForFence(CB->Fence);
    return llvm::Error::success();
  }

  void endEncodingImpl() override {
    if (ComputeEnc) {
      // Clear PendingScope, no actual barrier needed.
      PendingScope = MTL::BarrierScope(0);

      ComputeEnc->popDebugGroup();
      ComputeEnc->updateFence(CB->ensureFence());
      ComputeEnc->endEncoding();
      ComputeEnc = nullptr;
    }
    if (BlitEnc) {
      BlitEnc->updateFence(CB->ensureFence());
      BlitEnc->endEncoding();
      BlitEnc = nullptr;
    }
    if (ASEnc) {
      ASEnc->updateFence(CB->ensureFence());
      ASEnc->endEncoding();
      ASEnc = nullptr;
    }
  }
};

llvm::Expected<std::unique_ptr<offloadtest::ComputeEncoder>>
MTLCommandBuffer::createComputeEncoder() {
  return std::make_unique<MTLComputeEncoder>(this, CmdBuffer);
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
  MTLCommandBuffer *CB = nullptr;

  // Encoder contract: viewport and scissor must both be set before
  // drawInstanced().
  bool ViewportSet = false;
  bool ScissorSet = false;

public:
  MTLRenderEncoder(MTL::RenderCommandEncoder *Enc, MTLCommandBuffer *CB)
      : RenderEncoder(GPUAPI::Metal), RenderEnc(Enc), CB(CB) {}
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
      RenderEnc->setVertexBuffer(MTLVB.getBufferPtr(), Offset, BufIdx);
    } else {
      RenderEnc->setVertexBuffer(nullptr, 0, BufIdx);
    }
  }

  void bindDescriptorSets(const PipelineState &PSO,
                          const DescriptorSets &DSets) override {
    const auto &MTLPSO = llvm::cast<MTLPipelineState>(PSO);
    const MetalDescriptorSets &DSetsMTL =
        llvm::cast<MetalDescriptorSets>(DSets);

    RenderEnc->setVertexBuffer(CB->BoundCSUHeapBuffer, 0,
                               kIRDescriptorHeapBindPoint);
    RenderEnc->setFragmentBuffer(CB->BoundCSUHeapBuffer, 0,
                                 kIRDescriptorHeapBindPoint);

    // NOTE(manon): This is a problem when we dispatch the same pipeline
    // multiple times. This buffer should not be living in the PSO.
    for (uint32_t I = 0, N = DSetsMTL.Sets.size(); I < N; ++I) {
      assert(!MTLPSO.Layout.RSigLayout[I].IsSampler &&
             "Descriptor layout doesn't match root signature.");
      MTLPSO.ArgBuffer->setRootDescriptorTable(I,
                                               DSetsMTL.Sets[I].CSUHandleGPU);
    }

    RenderEnc->setVertexBuffer(MTLPSO.ArgBuffer->Buffer, 0,
                               kIRArgumentBufferBindPoint);
    RenderEnc->setFragmentBuffer(MTLPSO.ArgBuffer->Buffer, 0,
                                 kIRArgumentBufferBindPoint);
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
    // Perform worst-case sync, something that could be optimized in the future.
    // Probably minimal perf impact.
    RenderEnc->updateFence(CB->ensureFence(), AllRenderStages);
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

  // Perform worst-case sync, something that could be optimized in the future.
  // Probably minimal perf impact.
  if (Fence)
    NativeEncoder->waitForFence(Fence, AllRenderStages);

  NativeEncoder->pushDebugGroup(
      NS::String::string("RenderEncoder", NS::UTF8StringEncoding));
  return std::make_unique<MTLRenderEncoder>(NativeEncoder, this);
}

class MTLDevice : public offloadtest::Device {
public:
  Capabilities Caps;
  MTL::Device *Device;
  std::shared_ptr<MetalResidencyTracker> ResidencyTracker;
  MTLQueue GraphicsQueue;

  llvm::Error
  createRootSignature(const BindingsDesc &BindingsDesc, bool IsGraphics,
                      IRRootSignaturePtr &OutRootSig,
                      std::unique_ptr<MTLTopLevelArgumentBuffer> &OutArgBuffer,
                      DescriptorSetsLayout &Layout) {
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

      Layout.RSigLayout.push_back(RootSignatureLayout(false, DescriptorIdx));

      Layout.Sets.push_back({
          DescriptorIdx, // DescriptorCount
          0              // SamplerCount
      });
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

    auto ArgBufferOrErr = MTLTopLevelArgumentBuffer::create(
        Device, OutRootSig.get(), ResidencyTracker);
    if (!ArgBufferOrErr)
      return ArgBufferOrErr.takeError();

    OutArgBuffer = std::move(*ArgBufferOrErr);
    return llvm::Error::success();
  }

  llvm::Expected<MetalIR> convertToMetalIR(Stages Stage, bool IsGraphics,
                                           IRRootSignature *RootSig,
                                           const ShaderContainer &SC) {
    const IRCompilerPtr Compiler(IRCompilerCreate());
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
    const IRObjectPtr ResultIR(
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

  // Compile a single ray-tracing entry point out of a DXIL library to a Metal
  // library + reflection. The compiler is configured with the global root
  // signature and a IRRayTracingPipelineConfiguration that mirrors the
  // pipeline's RTConfig — raygen is forced to kernel-mode compilation so it
  // becomes the compute function on the pipeline state, while every other
  // RT stage is emitted as a visible function callable from the raygen kernel
  // through a MTLVisibleFunctionTable.
  llvm::Expected<MetalIR>
  convertRTShaderToMetalIR(Stages Stage, IRRootSignature *RootSig,
                           const IRRayTracingPipelineConfiguration *RTConfig,
                           llvm::StringRef Entry,
                           const llvm::MemoryBuffer &Library) {
    IRCompilerPtr Compiler(IRCompilerCreate());
    if (!Compiler)
      return llvm::createStringError(std::errc::not_supported,
                                     "Failed to create IR compiler instance.");
    if (!RootSig)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Root signature must be created before converting to Metal IR.");

    IRCompilerSetEntryPointName(Compiler.get(), std::string(Entry).c_str());
    IRCompilerSetGlobalRootSignature(Compiler.get(), RootSig);
    IRCompilerSetRayTracingPipelineConfiguration(Compiler.get(), RTConfig);

    const llvm::StringRef Bytes = Library.getBuffer();
    IRObjectPtr DXIL(
        IRObjectCreateFromDXIL(reinterpret_cast<const uint8_t *>(Bytes.data()),
                               Bytes.size(), IRBytecodeOwnershipNone));

    IRError *Err = nullptr;
    IRObjectPtr ResultIR(IRCompilerAllocCompileAndLink(
        Compiler.get(), std::string(Entry).c_str(), DXIL.get(), &Err));
    if (Err)
      return toError(IRErrorPtr(Err).get(),
                     "Failed to compile RT shader to Metal IR");

    const IRShaderStage ShaderStage = getShaderStage(Stage);
    auto MetalLib = IRMetalLibBinaryPtr(IRMetalLibBinaryCreate());
    if (!IRObjectGetMetalLibBinary(ResultIR.get(), ShaderStage, MetalLib.get()))
      return llvm::createStringError(
          std::errc::not_supported,
          "Failed to retrieve Metal library binary for RT entry '%s'",
          std::string(Entry).c_str());

    auto Reflection = IRShaderReflectionPtr(IRShaderReflectionCreate());
    if (!IRObjectGetReflection(ResultIR.get(), ShaderStage, Reflection.get()))
      return llvm::createStringError(
          std::errc::not_supported,
          "Failed to retrieve RT shader reflection for entry '%s'",
          std::string(Entry).c_str());

    return MetalIR{std::move(MetalLib), std::move(Reflection)};
  }

  llvm::Error createComputeCommands(Pipeline &P, SharedInvocationState &IS,
                                    DescriptorPool &Pool) {
    IS.CB->bindPool(Pool);

    auto DescSetsOrErr =
        buildDescriptorSets(*this, Pool, *IS.Pipeline, IS.DescTables);
    if (!DescSetsOrErr)
      return DescSetsOrErr.takeError();
    auto DescSets = std::move(*DescSetsOrErr);

    auto EncoderOrErr = IS.CB->createComputeEncoder();
    if (!EncoderOrErr)
      return EncoderOrErr.takeError();
    auto Encoder = std::move(*EncoderOrErr);

    Encoder->bindComputeDescriptorSets(*IS.Pipeline, *DescSets);

    if (auto Err = Encoder->dispatch(
            *IS.Pipeline.get(), P.DispatchParameters.DispatchGroupCount[0],
            P.DispatchParameters.DispatchGroupCount[1],
            P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
    Encoder->endEncoding();
    return llvm::Error::success();
  }

  llvm::Error createRayTracingCommands(Pipeline &P, SharedInvocationState &IS,
                                       DescriptorPool &Pool) {
    IS.CB->bindPool(Pool);

    auto DescSetsOrErr =
        buildDescriptorSets(*this, Pool, *IS.Pipeline, IS.DescTables);
    if (!DescSetsOrErr)
      return DescSetsOrErr.takeError();
    auto DescSets = std::move(*DescSetsOrErr);

    auto EncoderOrErr = IS.CB->createComputeEncoder();
    if (!EncoderOrErr)
      return EncoderOrErr.takeError();
    auto Encoder = std::move(*EncoderOrErr);

    Encoder->bindRayTracingDescriptorSets(*IS.Pipeline, *DescSets);

    if (auto Err =
            Encoder->dispatchRays(*IS.Pipeline.get(), *IS.SBT.get(),
                                  P.DispatchParameters.DispatchGroupCount[0],
                                  P.DispatchParameters.DispatchGroupCount[1],
                                  P.DispatchParameters.DispatchGroupCount[2]))
      return Err;
    Encoder->endEncoding();
    return llvm::Error::success();
  }

  llvm::Error createGraphicsCommands(Pipeline &P, SharedInvocationState &IS,
                                     DescriptorPool &Pool) {
    IS.CB->bindPool(Pool);

    auto DescSetsOrErr =
        buildDescriptorSets(*this, Pool, *IS.Pipeline, IS.DescTables);
    if (!DescSetsOrErr)
      return DescSetsOrErr.takeError();
    auto DescSets = std::move(*DescSetsOrErr);

    const uint64_t Width = IS.RenderTarget->getDesc().Width;
    const uint64_t Height = IS.RenderTarget->getDesc().Height;

    RenderPassBeginDesc BeginDesc = {};
    BeginDesc.Pass = IS.RenderPass.get();
    BeginDesc.ColorAttachments.push_back(IS.RenderTarget.get());
    BeginDesc.DepthStencil = IS.DepthStencil.get();

    auto EncOrErr = IS.CB->createRenderEncoder(BeginDesc);
    if (!EncOrErr)
      return EncOrErr.takeError();
    auto Encoder = std::move(*EncOrErr);

    Encoder->bindDescriptorSets(*IS.Pipeline, *DescSets);

    Viewport VP;
    VP.Width = static_cast<float>(Width);
    VP.Height = static_cast<float>(Height);
    Encoder->setViewport(VP);

    ScissorRect Scissor;
    Scissor.Width = static_cast<uint32_t>(Width);
    Scissor.Height = static_cast<uint32_t>(Height);
    Encoder->setScissor(Scissor);

    if (P.isTraditionalRaster()) {
      if (IS.VB)
        Encoder->setVertexBuffer(0, IS.VB.get(), 0,
                                 P.Bindings.getVertexStride());

      if (auto Err =
              Encoder->drawInstanced(*IS.Pipeline.get(), P.getVertexCount(),
                                     /*InstanceCount=*/1))
        return Err;
    } else {
      if (auto Err = Encoder->dispatchMesh(
              *IS.Pipeline.get(), P.DispatchParameters.DispatchGroupCount[0],
              P.DispatchParameters.DispatchGroupCount[1],
              P.DispatchParameters.DispatchGroupCount[2]))
        return Err;
    }

    Encoder->endEncoding();

    return llvm::Error::success();
  }

public:
  MTLDevice(MTL::Device *D, MTL::CommandQueue *Q,
            std::unique_ptr<MTLFence> SubmitFence)
      : Device(D),
        ResidencyTracker(llvm::cantFail(MetalResidencyTracker::create(D))),
        GraphicsQueue(Q, std::move(SubmitFence), ResidencyTracker) {
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

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineRT(llvm::StringRef Name, const BindingsDesc &BD,
                   const RayTracingPipelineCreateDesc &Desc) override {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this Metal device.");
    if (!Desc.Library)
      return llvm::createStringError(std::errc::invalid_argument,
                                     "RayTracingPipelineCreateDesc.Library is "
                                     "null — backend needs a DXIL blob.");

    IRRootSignaturePtr RootSig;
    std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
    DescriptorSetsLayout Layout;
    if (auto Err = createRootSignature(BD, /*IsGraphics=*/false, RootSig,
                                       ArgBuffer, Layout))
      return Err;

    // Configure the irconverter ray tracing pipeline. Raygen is compiled as a
    // kernel so it can be dispatched directly; miss / closest-hit / any-hit /
    // intersection / callable shaders are compiled as visible functions and
    // looked up via a MTLVisibleFunctionTable at runtime.
    auto RTConfig =
        std::unique_ptr<IRRayTracingPipelineConfiguration,
                        IRDeleter<IRRayTracingPipelineConfigurationDestroy>>(
            IRRayTracingPipelineConfigurationCreate());
    if (!RTConfig)
      return llvm::createStringError(
          std::errc::not_supported,
          "Failed to create IRRayTracingPipelineConfiguration.");
    IRRayTracingPipelineConfigurationSetMaxAttributeSizeInBytes(
        RTConfig.get(), Desc.Config.MaxAttributeSizeInBytes);
    IRRayTracingPipelineConfigurationSetMaxRecursiveDepth(
        RTConfig.get(), static_cast<int>(Desc.Config.MaxTraceRecursionDepth));
    IRRayTracingPipelineConfigurationSetRayGenerationCompilationMode(
        RTConfig.get(), IRRayGenerationCompilationKernel);
    IRRayTracingPipelineConfigurationSetIntersectionFunctionCompilationMode(
        RTConfig.get(), IRIntersectionFunctionCompilationVisibleFunction);

    auto State = std::make_unique<MTLRayTracingPipelineState>(
        Name, std::move(RootSig), std::move(ArgBuffer), std::move(Layout),
        ResidencyTracker);

    // Compile each entry point. Raygen lands in `RaygenFn` (becomes the
    // compute function); everything else gets linked in via LinkedFunctions
    // and indexed by visible-function-table slot.
    MTLPtr<MTL::Function> RaygenFn;
    std::string RaygenEntry;
    llvm::SmallVector<MTL::Function *, 4> VisibleFns;
    llvm::StringMap<uint32_t> EntryToVFTIndex;
    MTL::Size RaygenThreadsPerGroup(1, 1, 1);
    for (const auto &Sh : Desc.Shaders) {
      auto IROrErr = convertRTShaderToMetalIR(Sh.Stage, State->RootSig.get(),
                                              RTConfig.get(), Sh.EntryPoint,
                                              *Desc.Library);
      if (!IROrErr)
        return IROrErr.takeError();

      dispatch_data_t Data = IRMetalLibGetBytecodeData(IROrErr->Binary.get());
      NS::Error *Error = nullptr;
      MTL::Library *Lib = Device->newLibrary(Data, &Error);
      if (Error)
        return toError(Error);
      State->Libraries.push_back(Lib);

      MTL::Function *Fn = Lib->newFunction(
          NS::String::string(Sh.EntryPoint.c_str(), NS::UTF8StringEncoding));
      if (!Fn)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Failed to find RT entry point '%s' in compiled Metal library.",
            Sh.EntryPoint.c_str());

      if (Sh.Stage == Stages::RayGeneration) {
        RaygenFn.reset(Fn);
        RaygenEntry = Sh.EntryPoint;
        IRVersionedCSInfo Info;
        if (IRShaderReflectionCopyComputeInfo(IROrErr->Reflection.get(),
                                              IRReflectionVersion_1_0, &Info)) {
          RaygenThreadsPerGroup =
              MTL::Size(Info.info_1_0.tg_size[0], Info.info_1_0.tg_size[1],
                        Info.info_1_0.tg_size[2]);
          IRShaderReflectionReleaseComputeInfo(&Info);
        }
      } else {
        const uint32_t Slot = static_cast<uint32_t>(VisibleFns.size());
        VisibleFns.push_back(Fn);
        EntryToVFTIndex[Sh.EntryPoint] = Slot;
        State->Functions.push_back(Fn);
      }
    }
    if (!RaygenFn)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "RayTracing pipeline requires at least one RayGeneration shader.");

    // Pre-build IRShaderIdentifier records for every name the SBT can
    // reference. Raygen records carry no shader handle (the kernel is
    // dispatched directly); miss / closest-hit / callable carry their
    // visible-function-table index; hit groups reuse the closest-hit
    // index since this PR1 bring-up only supports HitGroupType::Triangles
    // without AnyHit/Intersection.
    IRShaderIdentifier RaygenIdent{};
    IRShaderIdentifierInit(&RaygenIdent, /*shaderHandle=*/0);
    State->ShaderIdentifiers[RaygenEntry] = RaygenIdent;

    for (const auto &Sh : Desc.Shaders) {
      if (Sh.Stage == Stages::RayGeneration)
        continue;
      auto It = EntryToVFTIndex.find(Sh.EntryPoint);
      assert(It != EntryToVFTIndex.end() && "missing visible-function index");
      IRShaderIdentifier Ident{};
      IRShaderIdentifierInit(&Ident, It->second);
      State->ShaderIdentifiers[Sh.EntryPoint] = Ident;
    }

    for (const auto &HG : Desc.HitGroups) {
      if (HG.AnyHit || HG.Intersection)
        return llvm::createStringError(
            std::errc::not_supported,
            "Metal RT bring-up only supports Triangle hit groups with a "
            "ClosestHit shader; AnyHit/Intersection support is not "
            "implemented yet.");
      auto It = EntryToVFTIndex.find(HG.ClosestHit);
      if (It == EntryToVFTIndex.end())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Hit group '%s' references unknown ClosestHit shader '%s'.",
            HG.Name.c_str(), HG.ClosestHit.c_str());
      IRShaderIdentifier Ident{};
      IRShaderIdentifierInit(&Ident, It->second);
      State->ShaderIdentifiers[HG.Name] = Ident;
    }

    // Pipeline descriptor: raygen as the compute function, everything else as
    // linked functions reachable from the visible function table.
    MTLPtr<MTL::ComputePipelineDescriptor> Desc2(
        MTL::ComputePipelineDescriptor::alloc()->init());
    Desc2->setComputeFunction(RaygenFn.get());
    Desc2->setLabel(
        NS::String::string(std::string(Name).c_str(), NS::UTF8StringEncoding));
    // setMaxCallStackDepth defaults to 1, allowing the raygen kernel exactly
    // one level of visible-function call. Nested TraceRay (depth ≥ 2) needs
    // the call stack to nest at least that deep — match what the YAML
    // RTConfig declares so recursive RT pipelines work.
    Desc2->setMaxCallStackDepth(Desc.Config.MaxTraceRecursionDepth);
    if (!VisibleFns.empty()) {
      MTLPtr<MTL::LinkedFunctions> Linked(
          MTL::LinkedFunctions::alloc()->init());
      NS::Array *FnArr = NS::Array::array(
          reinterpret_cast<NS::Object *const *>(VisibleFns.data()),
          VisibleFns.size());
      Linked->setFunctions(FnArr);
      Desc2->setLinkedFunctions(Linked.get());
    }

    NS::Error *Error = nullptr;
    MTL::ComputePipelineState *PSO = Device->newComputePipelineState(
        Desc2.get(), MTL::PipelineOptionNone, /*reflection=*/nullptr, &Error);
    if (Error)
      return toError(Error);

    // Populate the visible function table from function handles obtained on
    // the freshly-created pipeline.
    if (!VisibleFns.empty()) {
      MTLPtr<MTL::VisibleFunctionTableDescriptor> VFTDesc(
          MTL::VisibleFunctionTableDescriptor::alloc()->init());
      VFTDesc->setFunctionCount(VisibleFns.size());
      MTL::VisibleFunctionTable *VFT =
          PSO->newVisibleFunctionTable(VFTDesc.get());
      if (!VFT) {
        PSO->release();
        return llvm::createStringError(
            std::errc::device_or_resource_busy,
            "Failed to create MTL::VisibleFunctionTable for RT pipeline.");
      }
      for (uint32_t I = 0; I < VisibleFns.size(); ++I) {
        MTL::FunctionHandle *H = PSO->functionHandle(VisibleFns[I]);
        if (!H) {
          VFT->release();
          PSO->release();
          return llvm::createStringError(
              std::errc::not_supported,
              "Pipeline has no FunctionHandle for linked function index %u.",
              I);
        }
        VFT->setFunction(H, I);
      }
      State->VFT = VFT;
    }

    ResidencyTracker->withLock([&](MTL::ResidencySet *RS) {
      if (State->VFT)
        RS->addAllocation(State->VFT);
      if (State->IFT)
        RS->addAllocation(State->IFT);
    });

    State->ComputePipeline = PSO;
    State->ThreadsPerGroup = RaygenThreadsPerGroup;
    return State;
  }

  llvm::Expected<std::unique_ptr<ShaderBindingTable>>
  createShaderBindingTable(const PipelineState &PSO,
                           const ShaderBindingTableDesc &Desc) override {
    if (!llvm::isa<MTLRayTracingPipelineState>(&PSO))
      return llvm::createStringError(
          std::errc::invalid_argument,
          "createShaderBindingTable requires a RayTracing PipelineState.");
    const auto &RTPSO = llvm::cast<MTLRayTracingPipelineState>(PSO);

    // Layout: four concatenated regions of IRShaderIdentifier-sized records.
    // computeSBTLayout aligns record stride/region size to the values we
    // pass. Metal does not expose explicit record/table alignment knobs the
    // way D3D12 does — pick natural alignment (16 bytes) so the irconverter
    // runtime's pointer reads stay aligned.
    constexpr uint32_t IdSize =
        static_cast<uint32_t>(sizeof(IRShaderIdentifier));
    constexpr uint32_t RecordAlign = 16;
    constexpr uint32_t BaseAlign = 16;
    const SBTLayout Layout =
        computeSBTLayout(IdSize, RecordAlign, BaseAlign, Desc);
    const uint32_t TotalSize = Layout.TotalSize;
    const llvm::ArrayRef<SBTEntry> RGEntries(&Desc.RayGen, 1);

    MTL::Buffer *Buffer =
        Device->newBuffer(TotalSize, MTL::ResourceStorageModeShared);
    if (!Buffer)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Failed to allocate Metal SBT buffer.");
    auto *Mapped = static_cast<uint8_t *>(Buffer->contents());
    std::memset(Mapped, 0, TotalSize);

    auto WriteEntries = [&](uint8_t *Region, llvm::ArrayRef<SBTEntry> Entries,
                            uint32_t Stride) -> llvm::Error {
      for (size_t I = 0; I < Entries.size(); ++I) {
        const auto &E = Entries[I];
        auto It = RTPSO.ShaderIdentifiers.find(E.ShaderName);
        if (It == RTPSO.ShaderIdentifiers.end()) {
          Buffer->release();
          return llvm::createStringError(
              std::errc::invalid_argument,
              "SBT references unknown shader/hit-group name: '%s'",
              E.ShaderName.c_str());
        }
        uint8_t *Dst = Region + I * Stride;
        std::memcpy(Dst, &It->second, sizeof(IRShaderIdentifier));
        if (!E.LocalRootData.empty())
          std::memcpy(Dst + sizeof(IRShaderIdentifier), E.LocalRootData.data(),
                      E.LocalRootData.size());
      }
      return llvm::Error::success();
    };

    if (auto Err = WriteEntries(Mapped + Layout.RayGen.Offset, RGEntries,
                                Layout.RayGen.Stride))
      return Err;
    if (auto Err = WriteEntries(Mapped + Layout.Miss.Offset, Desc.Miss,
                                Layout.Miss.Stride))
      return Err;
    if (auto Err = WriteEntries(Mapped + Layout.HitGroup.Offset, Desc.HitGroup,
                                Layout.HitGroup.Stride))
      return Err;
    if (auto Err = WriteEntries(Mapped + Layout.Callable.Offset, Desc.Callable,
                                Layout.Callable.Stride))
      return Err;

    const uint64_t Base = Buffer->gpuAddress();
    auto MakeRange = [&](const SBTRegionLayout &R) {
      IRVirtualAddressRange V{};
      V.StartAddress = R.Size ? Base + R.Offset : 0;
      V.SizeInBytes = R.Size;
      return V;
    };
    auto MakeRangeAndStride = [&](const SBTRegionLayout &R) {
      IRVirtualAddressRangeAndStride V{};
      V.StartAddress = R.Size ? Base + R.Offset : 0;
      V.SizeInBytes = R.Size;
      V.StrideInBytes = R.Stride;
      return V;
    };
    return std::make_unique<MTLShaderBindingTable>(
        Buffer, MakeRange(Layout.RayGen), MakeRangeAndStride(Layout.Miss),
        MakeRangeAndStride(Layout.HitGroup),
        MakeRangeAndStride(Layout.Callable), ResidencyTracker);
  }

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

    MTL::Resource *Res = nullptr;
    if (Desc.AccessType == BufferShaderAccessType::Typed) {
      MTL::TextureDescriptor *TDesc =
          MTL::TextureDescriptor::textureBufferDescriptor(
              getMetalPixelFormat(Desc.AccessTypeParams.Fmt),
              SizeInBytes / getFormatSizeInBytes(Desc.AccessTypeParams.Fmt),
              getMetalBufferResourceOptions(Desc.Location),
              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);

      Res = Device->newTexture(TDesc);
      if (!Res)
        return llvm::createStringError(
            std::errc::not_enough_memory,
            "Failed to create Metal typed buffer (texture).");
    } else {
      Res = Device->newBuffer(SizeInBytes,
                              getMetalBufferResourceOptions(Desc.Location));
      if (!Res)
        return llvm::createStringError(std::errc::not_enough_memory,
                                       "Failed to create Metal buffer.");
    }

    return std::make_unique<MTLBuffer>(Res, Name, Desc, SizeInBytes,
                                       ResidencyTracker);
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
    return std::make_unique<MTLTexture>(Tex, Name, Desc, ResidencyTracker);
  }

  llvm::Expected<std::unique_ptr<Sampler>>
  createSampler(std::string, const SamplerCreateDesc &) override {
    return llvm::createStringError("createSampler is unimplemented on Metal.");
  }

  uint32_t getTextureUploadRowStrideInBytes(
      const TextureCreateDesc &Desc) const override {
    return Desc.Width * getFormatSizeInBytes(Desc.Fmt);
  }

  TextureUploadLayout
  getTextureUploadLayout(const TextureCreateDesc &Desc) const override {
    // copyBufferToTexture consumes a tightly-packed staging buffer.
    return computeTightTextureUploadLayout(Desc);
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
    DescriptorSetsLayout Layout;
    if (auto Err = createRootSignature(BindingsDesc, /*IsGraphics=*/false,
                                       RootSig, ArgBuffer, Layout))
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
        Name, std::move(RootSig), std::move(ArgBuffer), std::move(Layout), PSO,
        ThreadsPerGroup);
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
    DescriptorSetsLayout Layout;
    if (auto Err = createRootSignature(BindingsDesc, /*IsGraphics=*/true,
                                       RootSig, ArgBuffer, Layout))
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

    return std::make_unique<MTLPipelineState>(
        Name, std::move(RootSig), std::move(ArgBuffer), std::move(Layout), PSO,
        DSState, MTL::CullModeNone);
  }

  llvm::Expected<std::unique_ptr<PipelineState>> createMeshShaderRasterPipeline(
      llvm::StringRef Name, const BindingsDesc &BindingsDesc,
      const MeshShaderRasterPipelineCreateDesc &Desc) override {
    IRRootSignaturePtr RootSig;
    std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
    DescriptorSetsLayout Layout;
    if (auto Err = createRootSignature(BindingsDesc, /*IsGraphics=*/true,
                                       RootSig, ArgBuffer, Layout))
      return Err;

    NS::Error *Error = nullptr;
    auto CompileStage = [&](Stages Stage, const ShaderContainer &SC,
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
            CompileStage(Stages::Mesh, Desc.MS, "mesh", MSIR, MSLib, MSFn))
      return Err;

    MetalIR ASIR;
    MTLPtr<MTL::Library> ASLib;
    MTLPtr<MTL::Function> ASFn;
    if (Desc.AS) {
      if (auto Err = CompileStage(Stages::Amplification, *Desc.AS,
                                  "amplification", ASIR, ASLib, ASFn))
        return Err;
    }

    MetalIR PSIR;
    MTLPtr<MTL::Library> PSLib;
    MTLPtr<MTL::Function> PSFn;
    if (Desc.PS) {
      if (auto Err = CompileStage(Stages::Pixel, *Desc.PS, "fragment", PSIR,
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
        Name, std::move(RootSig), std::move(ArgBuffer), std::move(Layout), PSO,
        DSState, MTL::CullModeNone, MeshTGSize, ObjectTGSize);
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
      TD->setVertexBuffer(VB->getBufferPtr());
      TD->setVertexBufferOffset(T.VertexBufferOffset);
      TD->setVertexStride(T.VertexStride);
      TD->setVertexFormat(getMetalPositionFormat(T.VertexFormat));
      TD->setTriangleCount(T.IndexBuffer ? T.IndexCount / 3
                                         : T.VertexCount / 3);
      if (T.IndexBuffer) {
        auto *IB = llvm::cast<MTLBuffer>(T.IndexBuffer);
        TD->setIndexBuffer(IB->getBufferPtr());
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
      AD->setBoundingBoxBuffer(BB->getBufferPtr());
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

  AccelerationStructureSizes queryBLASPrebuildSize(
      llvm::ArrayRef<MTL::AccelerationStructureGeometryDescriptor *> Descs) {
    NS::Array *GeomDescs = NS::Array::array(
        reinterpret_cast<NS::Object *const *>(Descs.data()), Descs.size());

    auto *Descriptor =
        MTL::PrimitiveAccelerationStructureDescriptor::alloc()->init();
    Descriptor->setGeometryDescriptors(GeomDescs);

    const MTL::AccelerationStructureSizes Sizes =
        Device->accelerationStructureSizes(Descriptor);

    Descriptor->release();

    return {Sizes.accelerationStructureSize, Sizes.buildScratchBufferSize,
            Sizes.refitScratchBufferSize};
  }

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

    const MTL::AccelerationStructureSizes Sizes =
        Device->accelerationStructureSizes(Descriptor);

    Descriptor->release();

    return AccelerationStructureSizes{Sizes.accelerationStructureSize,
                                      Sizes.buildScratchBufferSize,
                                      Sizes.refitScratchBufferSize};
  }

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createBLAS(const AccelerationStructureSizes &Sizes) override {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this device.");

    MTL::AccelerationStructure *AS =
        Device->newAccelerationStructure(Sizes.ResultDataMaxSizeInBytes);
    if (!AS)
      return llvm::createStringError(
          std::make_error_code(std::errc::not_enough_memory),
          "Failed to create Metal BLAS.");

    return std::make_unique<MetalAccelerationStructure>(AS, Sizes,
                                                        ResidencyTracker);
  }

  llvm::Expected<std::unique_ptr<offloadtest::AccelerationStructure>>
  createTLAS(const AccelerationStructureSizes &Sizes,
             uint32_t InstanceCount) override {
    if (!Device->supportsRaytracing())
      return llvm::createStringError(
          std::errc::not_supported,
          "Ray tracing is not supported on this device.");

    // TODO(manon): We would prefer these to live in GPUOnly memory in the
    // future.
    const BufferCreateDesc ContribBufferDesc =
        BufferCreateDesc::gpuOnlyStorage();
    auto ContribBufferOrErr =
        createBuffer("AS-Contributions", ContribBufferDesc,
                     InstanceCount * sizeof(uint32_t));
    if (!ContribBufferOrErr)
      return ContribBufferOrErr.takeError();
    auto ContribBuffer = std::move(*ContribBufferOrErr);

    const MTLBuffer &ContribBufferMTL =
        llvm::cast<MTLBuffer>(*ContribBuffer.get());

    MTL::AccelerationStructure *AS =
        Device->newAccelerationStructure(Sizes.ResultDataMaxSizeInBytes);
    if (!AS)
      return llvm::createStringError(
          std::make_error_code(std::errc::not_enough_memory),
          "Failed to create Metal TLAS.");

    IRRaytracingAccelerationStructureGPUHeader Header = {};
    Header.accelerationStructureID = AS->gpuResourceID()._impl;
    Header.addressOfInstanceContributions =
        ContribBufferMTL.getBufferPtr()->gpuAddress();

    const BufferCreateDesc HeaderBufferDesc = BufferCreateDesc::uploadBuffer();
    auto HeaderBufOrErr =
        createBufferWithData(*this, "AS-Header", HeaderBufferDesc, &Header,
                             sizeof(Header), nullptr, nullptr);
    if (!HeaderBufOrErr) {
      AS->release();
      return HeaderBufOrErr.takeError();
    }
    auto HeaderBuffer = std::move(*HeaderBufOrErr);

    return std::make_unique<MetalAccelerationStructure>(
        AS, Sizes, ResidencyTracker, std::move(HeaderBuffer),
        std::move(ContribBuffer));
  }

  llvm::Expected<std::unique_ptr<DescriptorPool>>
  createDescriptorPool() override {
    return MetalDescriptorPool::create(Device, ResidencyTracker);
  }

  llvm::Expected<std::unique_ptr<DescriptorSetsBuilder>>
  createDescriptorSetsBuilder(DescriptorPool &Pool,
                              const PipelineState &Pipeline) override {
    MetalDescriptorPool &PoolMTL = llvm::cast<MetalDescriptorPool>(Pool);
    const MTLPipelineState &PipelineMTL =
        llvm::cast<MTLPipelineState>(Pipeline);

    llvm::SmallVector<MetalDescriptorSet> Sets;
    for (const auto &Counts : PipelineMTL.Layout.Sets) {
      MetalDescriptorSet Set = {};
      if (Counts.DescriptorCount > 0)
        PoolMTL.allocateDescriptors(Counts.DescriptorCount, Set.CSUHandle,
                                    Set.CSUHandleGPU);
      Sets.push_back(Set);
    }
    return std::make_unique<MetalDescriptorSetsBuilder>(std::move(Sets));
  }

  llvm::Error executeProgram(Pipeline &P) override {
    auto DescriptorPoolOrErr = createDescriptorPool();
    if (!DescriptorPoolOrErr)
      return DescriptorPoolOrErr.takeError();
    auto DescriptorPool = std::move(*DescriptorPoolOrErr);

    SharedInvocationState IS;

    NS::AutoreleasePool *Pool = NS::AutoreleasePool::alloc()->init();
    auto PoolScope = llvm::scope_exit([&] { Pool->release(); });

    auto CBOrErr = createCommandBuffer();
    if (!CBOrErr)
      return CBOrErr.takeError();
    IS.CB = std::move(*CBOrErr);

    if (auto Err = createResources(*this, P, IS))
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

      if (auto Err = createComputeCommands(P, IS, *DescriptorPool))
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

      if (auto Err = createGraphicsCommands(P, IS, *DescriptorPool))
        return Err;
    } else if (P.isRayTracing()) {
      if (P.Shaders.empty() || !P.SBT || !P.RTConfig)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "RayTracing pipeline requires Shaders, "
            "ShaderBindingTable, and RayTracingPipelineConfig.");

      RayTracingPipelineCreateDesc RTDesc{};
      // All RT shader entries share the single DXIL library blob fanned out
      // by the offloader CLI for RT pipelines.
      RTDesc.Library = P.Shaders.front().Shader.get();
      RTDesc.HitGroups = P.HitGroups;
      RTDesc.Config = *P.RTConfig;
      RTDesc.Shaders.reserve(P.Shaders.size());
      for (const auto &Sh : P.Shaders)
        RTDesc.Shaders.push_back({Sh.Stage, Sh.Entry});

      auto PSOOrErr =
          createPipelineRT("RayTracing Pipeline State", Bindings, RTDesc);
      if (!PSOOrErr)
        return PSOOrErr.takeError();
      IS.Pipeline = std::move(*PSOOrErr);
      llvm::outs() << "RayTracing Pipeline created.\n";

      auto SBTOrErr = createShaderBindingTable(*IS.Pipeline, *P.SBT);
      if (!SBTOrErr)
        return SBTOrErr.takeError();
      IS.SBT = std::move(*SBTOrErr);
      llvm::outs() << "Shader Binding Table created.\n";

      if (auto Err = createRayTracingCommands(P, IS, *DescriptorPool))
        return Err;
    }

    auto EncoderOrErr = IS.CB->createComputeEncoder();
    if (!EncoderOrErr)
      return EncoderOrErr.takeError();
    auto ReadbackEncoder = std::move(*EncoderOrErr);

    if (IS.RenderTarget) {
      if (auto Err = ReadbackEncoder->copyTextureToBuffer(*IS.RenderTarget,
                                                          *IS.RTReadback))
        return Err;
    }

    for (auto &Table : IS.DescTables)
      for (auto &R : Table.Resources)
        if (auto Err = copyBackResource(*ReadbackEncoder, R))
          return Err;

    for (auto &R : IS.RootResources)
      if (auto Err = copyBackResource(*ReadbackEncoder, R))
        return Err;

    ReadbackEncoder->endEncoding();

    auto SubmitResult = GraphicsQueue.submit(std::move(IS.CB));
    if (!SubmitResult)
      return SubmitResult.takeError();

    if (auto Err = SubmitResult->waitForCompletion())
      return Err;

    if (auto Err = readBack(*this, P, IS))
      return Err;
    llvm::outs() << "Read data back.\n";
    return llvm::Error::success();
  }

  virtual ~MTLDevice() {};

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
          TD->setVertexBuffer(VB->getBufferPtr());
          TD->setVertexBufferOffset(T.VertexBufferOffset);
          TD->setVertexStride(T.VertexStride);
          TD->setVertexFormat(getMetalPositionFormat(T.VertexFormat));
          TD->setTriangleCount(T.IndexBuffer ? T.IndexCount / 3
                                             : T.VertexCount / 3);
          if (T.IndexBuffer) {
            auto *IB = llvm::cast<MTLBuffer>(T.IndexBuffer);
            TD->setIndexBuffer(IB->getBufferPtr());
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
          AD->setBoundingBoxBuffer(BB->getBufferPtr());
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
        auto *It = std::find(UniqueBLASes.begin(), UniqueBLASes.end(),
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

      const BufferCreateDesc UploadDesc = BufferCreateDesc::uploadBuffer();
      const uint32_t ContribBufferSize = AS->ContribBuffer->getSizeInBytes();
      auto ContribUploadBufferOrErr = CB->Dev->createBuffer(
          "Contrib Upload Buffer", UploadDesc, ContribBufferSize);
      if (!ContribUploadBufferOrErr)
        return ContribUploadBufferOrErr.takeError();
      auto ContribUploadBuffer = std::move(*ContribUploadBufferOrErr);

      auto ContribPtrOrErr = ContribUploadBuffer->map();
      if (!ContribPtrOrErr)
        return ContribPtrOrErr.takeError();
      uint32_t *ContribPtr = static_cast<uint32_t *>(*ContribPtrOrErr);

      // Pack instance descriptors. Layout differs from VK/DX12: 32-byte
      // entries with an index instead of a GPU address.
      llvm::SmallVector<MTL::AccelerationStructureUserIDInstanceDescriptor>
          Native;
      llvm::SmallVector<uint32_t> HitContributions;
      Native.reserve(TLAS->Instances.size());
      HitContributions.reserve(TLAS->Instances.size());
      for (size_t I = 0; I < TLAS->Instances.size(); ++I) {
        const auto &Src = TLAS->Instances[I];
        MTL::AccelerationStructureUserIDInstanceDescriptor D = {};
        // Metal stores transform as packed 4x3 column-major; our high-level
        // Transform[3][4] is row-major. Transpose into Metal's layout.
        for (int Row = 0; Row < 3; ++Row)
          for (int Col = 0; Col < 4; ++Col)
            D.transformationMatrix.columns[Col][Row] = Src.Transform[Row][Col];
        // Bits in AccelerationStructureInstanceFlags match
        // MTLAccelerationStructureInstanceOptions by design.
        D.options =
            static_cast<MTL::AccelerationStructureInstanceOptions>(Src.Flags);
        D.mask = Src.InstanceMask;
        D.intersectionFunctionTableOffset = 0;
        D.accelerationStructureIndex = InstanceASIdx[I];
        D.userID = Src.InstanceID;
        Native.push_back(D);
        ContribPtr[I] = Src.InstanceContributionToHitGroupIndex &
                        0xffffff; // cut-off to 24-bit to match dx12 and vulkan.
      }
      ContribUploadBuffer->unmap();

      if (auto Err = this->copyBufferToBuffer(*ContribUploadBuffer.get(), 0,
                                              *AS->ContribBuffer.get(), 0,
                                              ContribBufferSize))
        return Err;

      CB->KeepAliveOwned.push_back(std::move(ContribUploadBuffer));

      const size_t InstByteSize =
          Native.size() *
          sizeof(MTL::AccelerationStructureUserIDInstanceDescriptor);

      auto InstBufOrErr = offloadtest::createBufferWithData(
          *CB->Dev, "TLAS-Instances", UploadDesc, Native.data(), InstByteSize,
          nullptr, nullptr);
      if (!InstBufOrErr)
        return InstBufOrErr.takeError();
      const MTLBuffer &MTLInstBuf = llvm::cast<MTLBuffer>(*InstBufOrErr->get());

      auto *ID = MTL::InstanceAccelerationStructureDescriptor::alloc()->init();
      ID->setInstanceDescriptorBuffer(MTLInstBuf.getBufferPtr());
      ID->setInstanceCount(TLAS->Instances.size());
      ID->setInstanceDescriptorType(
          MTL::AccelerationStructureInstanceDescriptorTypeUserID);
      NS::Array *BLASArr = NS::Array::array(
          reinterpret_cast<NS::Object *const *>(UniqueBLASes.data()),
          UniqueBLASes.size());
      ID->setInstancedAccelerationStructures(BLASArr);
      Desc = ID;
      ScratchSize = TLAS->AS->getSizes().ScratchDataSizeInBytes;

      CB->KeepAliveOwned.push_back(std::move(*InstBufOrErr));
    }

    if (auto Err = ensureASEncoder())
      return Err;

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
    ASEnc->buildAccelerationStructure(AS->AccelStruct, Desc,
                                      MTLScratch->getBufferPtr(), 0);
    Desc->release();
  }

  return llvm::Error::success();
}

MTL::Fence *MTLCommandBuffer::ensureFence() {
  if (Fence)
    return Fence;

  Fence = Dev->Device->newFence();
  if (!Fence)
    llvm::report_fatal_error("Failed to create Metal fence.");

  return Fence;
}

llvm::Error MTLComputeEncoder::dispatchRays(const PipelineState &PSO,
                                            const ShaderBindingTable &SBT,
                                            uint32_t Width, uint32_t Height,
                                            uint32_t Depth) {
  if (!llvm::isa<MTLRayTracingPipelineState>(&PSO))
    return llvm::createStringError(
        std::errc::invalid_argument,
        "dispatchRays requires a RayTracing PipelineState.");
  const auto &RTPSO = llvm::cast<MTLRayTracingPipelineState>(PSO);
  const auto &MTLSBT = llvm::cast<MTLShaderBindingTable>(SBT);
  if (!RTPSO.ComputePipeline)
    return llvm::createStringError(
        std::errc::invalid_argument,
        "RayTracing PipelineState has no compute pipeline state.");
  if (auto Err = ensureComputeEncoder())
    return Err;
  flushBarrier();
  insertDebugSignpost(
      llvm::formatv("DispatchRays [{0},{1},{2}]", Width, Height, Depth).str());

  // Per-dispatch ray arguments, consumed at kIRRayDispatchArgumentsBindPoint.
  IRDispatchRaysArgument Args{};
  Args.DispatchRaysDesc.RayGenerationShaderRecord = MTLSBT.RayGenRegion;
  Args.DispatchRaysDesc.MissShaderTable = MTLSBT.MissRegion;
  Args.DispatchRaysDesc.HitGroupTable = MTLSBT.HitGroupRegion;
  Args.DispatchRaysDesc.CallableShaderTable = MTLSBT.CallableRegion;
  Args.DispatchRaysDesc.Width = Width;
  Args.DispatchRaysDesc.Height = Height;
  Args.DispatchRaysDesc.Depth = Depth;
  Args.GRS = RTPSO.ArgBuffer->getGPUAddress();
  Args.ResDescHeap =
      CB->BoundCSUHeapBuffer ? CB->BoundCSUHeapBuffer->gpuAddress() : 0;
  Args.SmpDescHeap = 0;
  Args.VisibleFunctionTable =
      RTPSO.VFT ? RTPSO.VFT->gpuResourceID() : MTL::ResourceID{0};
  Args.IntersectionFunctionTable =
      RTPSO.IFT ? RTPSO.IFT->gpuResourceID() : MTL::ResourceID{0};
  Args.IntersectionFunctionTables = 0;

  const BufferCreateDesc ArgsBufDesc = BufferCreateDesc::uploadBuffer();
  auto ArgsBufOrErr = offloadtest::createBufferWithData(
      *CB->Dev, "MTL Dispatch Rays Arguments", ArgsBufDesc, &Args,
      sizeof(IRDispatchRaysArgument), nullptr, nullptr);
  if (!ArgsBufOrErr)
    return ArgsBufOrErr.takeError();
  auto *MTLArgsBuf = llvm::cast<MTLBuffer>(ArgsBufOrErr->get());
  CB->KeepAliveOwned.push_back(std::move(*ArgsBufOrErr));

  ComputeEnc->setBuffer(MTLArgsBuf->getBufferPtr(), 0,
                        kIRRayDispatchArgumentsBindPoint);

  ComputeEnc->setComputePipelineState(RTPSO.ComputePipeline);

  // dispatchThreads automatically takes care of bounds checking.
  const MTL::Size GridSize(Width, Height, Depth);
  ComputeEnc->dispatchThreads(GridSize, RTPSO.ThreadsPerGroup);
  addBarrierScope(MTL::BarrierScopeBuffers | MTL::BarrierScopeTextures);
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
