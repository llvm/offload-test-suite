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

#include "API/Device.h"
#include "API/FormatConversion.h"
#include "MTLDescriptorHeap.h"
#include "MTLResources.h"
#include "MTLTopLevelArgumentBuffer.h"
#include "Support/Pipeline.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
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
  case Stages::Pixel:
    return IRShaderStageFragment;
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
};

class MTLPipelineState : public offloadtest::PipelineState {
public:
  std::string Name;
  MTL::ComputePipelineState *ComputePipeline = nullptr;
  MTL::RenderPipelineState *RenderPipeline = nullptr;

  MTLPipelineState(llvm::StringRef Name,
                   MTL::ComputePipelineState *ComputePipeline)
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        ComputePipeline(ComputePipeline) {}

  MTLPipelineState(llvm::StringRef Name,
                   MTL::RenderPipelineState *RenderPipeline)
      : offloadtest::PipelineState(GPUAPI::Metal), Name(Name),
        RenderPipeline(RenderPipeline) {}

  ~MTLPipelineState() override {
    if (ComputePipeline)
      ComputePipeline->release();
    if (RenderPipeline)
      RenderPipeline->release();
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

  ~MTLBuffer() override {
    if (Buf)
      Buf->release();
  }

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

  static bool classof(const offloadtest::Texture *T) {
    return T->getAPI() == GPUAPI::Metal;
  }
};

class MTLCommandBuffer : public offloadtest::CommandBuffer {
public:
  MTL::CommandBuffer *CmdBuffer = nullptr;

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

private:
  MTLCommandBuffer() : CommandBuffer(GPUAPI::Metal) {}
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
class MTLDevice : public offloadtest::Device {
  Capabilities Caps;
  MTL::Device *Device;
  MTLQueue GraphicsQueue;

  struct ResourceSet {
    MTLPtr<MTL::Resource> Resource;
    ResourceSet(MTL::Resource *Resource) : Resource(Resource) {}
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
    IRRootSignaturePtr RootSig;
    std::unique_ptr<MTLTopLevelArgumentBuffer> ArgBuffer;
    std::unique_ptr<MTLDescriptorHeap> DescHeap;
    MTL::Buffer *VertexBuffer;
    std::unique_ptr<offloadtest::Texture> FrameBufferTexture;
    std::unique_ptr<offloadtest::Buffer> FrameBufferReadback;
    std::unique_ptr<offloadtest::Texture> DepthStencil;
    std::unique_ptr<MTLCommandBuffer> CB;
    std::unique_ptr<PipelineState> Pipeline;
    IRShaderReflectionPtr ComputeReflection;

    llvm::SmallVector<DescriptorTable> DescTables;
    // TODO: Support RootResources?
  };

  llvm::Error createRootSignature(const Pipeline &P, InvocationState &State) {
    std::vector<IRRootParameter1> RootParams;
    const uint32_t DescriptorCount = P.getDescriptorCount();
    const std::unique_ptr<IRDescriptorRange1[]> Ranges =
        std::unique_ptr<IRDescriptorRange1[]>(
            new IRDescriptorRange1[DescriptorCount]);

    uint32_t RangeIdx = 0;
    for (const auto &D : P.Sets) {
      uint32_t DescriptorIdx = 0;
      const uint32_t StartRangeIdx = RangeIdx;
      for (const auto &R : D.Resources) {
        auto &Range = Ranges.get()[RangeIdx];
        switch (getDescriptorKind(R.Kind)) {
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
          llvm_unreachable("Not implemented yet.");
        }
        Range.NumDescriptors = R.getArraySize();
        Range.BaseShaderRegister = R.DXBinding.Register;
        Range.RegisterSpace = R.DXBinding.Space;
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
        DescriptorIdx += R.getArraySize();
      }

      auto &Param = RootParams.emplace_back();
      Param.ParameterType = IRRootParameterTypeDescriptorTable;
      Param.DescriptorTable.NumDescriptorRanges =
          static_cast<uint32_t>(D.Resources.size());
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
    Desc.Flags = P.isGraphics()
                     ? IRRootSignatureFlagAllowInputAssemblerInputLayout
                     : IRRootSignatureFlagNone;

    IRError *Err = nullptr;
    IRRootSignaturePtr RootSig(
        IRRootSignatureCreateFromDescriptor(&VersionedDesc, &Err));
    if (!RootSig)
      return toError(IRErrorPtr(Err).get(), "Failed to create root signature");

    State.RootSig = std::move(RootSig);

    auto ArgBufferOrErr =
        MTLTopLevelArgumentBuffer::create(Device, State.RootSig.get());
    if (!ArgBufferOrErr)
      return ArgBufferOrErr.takeError();

    State.ArgBuffer = std::move(*ArgBufferOrErr);
    return llvm::Error::success();
  }

  llvm::Error createDescriptorHeap(Pipeline &P, InvocationState &State) {
    if (P.getDescriptorCount() == 0) {
      llvm::outs()
          << "No descriptors found, skipping descriptor heap creation.\n";
      return llvm::Error::success();
    }
    const METAL_DESCRIPTOR_HEAP_DESC HeapDesc = {
        METAL_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        P.getDescriptorCountWithFlattenedArrays()};

    auto DescHeapOrErr = MTLDescriptorHeap::create(Device, HeapDesc);
    if (!DescHeapOrErr)
      return DescHeapOrErr.takeError();

    State.DescHeap = std::move(*DescHeapOrErr);
    llvm::outs() << "Descriptor heap created with "
                 << P.getDescriptorCountWithFlattenedArrays()
                 << " descriptors.\n";
    return llvm::Error::success();
  }

  llvm::Expected<MetalIR> convertToMetalIR(const Pipeline &P, Stages Stage,
                                           const ShaderContainer &SC,
                                           const InvocationState &State) {
    IRCompilerPtr Compiler(IRCompilerCreate());
    if (!Compiler)
      return llvm::createStringError(std::errc::not_supported,
                                     "Failed to create IR compiler instance.");

    if (!State.RootSig)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Root signature must be created before converting to Metal IR.");

    // Configure IR compiler settings
    IRCompilerSetEntryPointName(Compiler.get(), SC.Entry.c_str());
    IRCompilerSetGlobalRootSignature(Compiler.get(), State.RootSig.get());
    if (P.isGraphics()) {
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

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    auto CreateBuffer =
        [&IS,
         this](Resource &R,
               llvm::SmallVectorImpl<ResourcePair> &Resources) -> llvm::Error {
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

    if (P.isGraphics()) {
      // Create and mark the vertex buffer as modified.
      IS.VertexBuffer = Device->newBuffer(
          P.Bindings.VertexBufferPtr->Data.back().get(),
          P.Bindings.VertexBufferPtr->size(), MTL::ResourceStorageModeManaged);
      IS.VertexBuffer->didModifyRange(
          NS::Range::Make(0, IS.VertexBuffer->length()));
    }
    return llvm::Error::success();
  }

  llvm::Error createComputeCommands(Pipeline &P, InvocationState &IS) {
    MTL::ComputeCommandEncoder *CmdEncoder =
        IS.CB->CmdBuffer->computeCommandEncoder();

    auto CloseCommandEncoder =
        llvm::scope_exit([&]() { CmdEncoder->endEncoding(); });

    const auto &PS = llvm::cast<MTLPipelineState>(IS.Pipeline.get());
    CmdEncoder->setComputePipelineState(PS->ComputePipeline);
    METAL_GPU_DESCRIPTOR_HANDLE Handle = {};
    if (IS.DescHeap) {
      IS.DescHeap->bind(CmdEncoder);
      Handle = IS.DescHeap->getGPUDescriptorHandleForHeapStart();
    }

    for (uint32_t Idx = 0u; Idx < P.Sets.size(); ++Idx) {
      IS.ArgBuffer->setRootDescriptorTable(Idx, Handle);
      Handle.Offset(P.Sets[Idx].Resources.size());
    }

    IS.ArgBuffer->bind(CmdEncoder);
    for (const auto &Table : IS.DescTables)
      for (const auto &ResPair : Table.Resources)
        for (const auto &ResSet : ResPair.second)
          CmdEncoder->useResource(ResSet.Resource.get(),
                                  MTL::ResourceUsageRead |
                                      MTL::ResourceUsageWrite);

    NS::UInteger TGS[3] = {PS->ComputePipeline->maxTotalThreadsPerThreadgroup(),
                           1, 1};
    if (IS.ComputeReflection) {
      IRVersionedCSInfo Info;
      if (IRShaderReflectionCopyComputeInfo(IS.ComputeReflection.get(),
                                            IRReflectionVersion_1_0, &Info)) {
        TGS[0] = Info.info_1_0.tg_size[0];
        TGS[1] = Info.info_1_0.tg_size[1];
        TGS[2] = Info.info_1_0.tg_size[2];
      }
      IRShaderReflectionReleaseComputeInfo(&Info);
    }

    const llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
    const MTL::Size GridSize =
        MTL::Size(TGS[0] * DispatchSize[0], TGS[1] * DispatchSize[1],
                  TGS[2] * DispatchSize[2]);
    const MTL::Size GroupSize(TGS[0], TGS[1], TGS[2]);
    CmdEncoder->dispatchThreads(GridSize, GroupSize);
    CmdEncoder->memoryBarrier(MTL::BarrierScopeBuffers);

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

    IS.FrameBufferTexture = std::move(*TexOrErr);

    // Create a readback buffer for copying render target data to the CPU.
    BufferCreateDesc BufDesc = {};
    BufDesc.Location = MemoryLocation::GpuToCpu;
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

    auto &FBTex = llvm::cast<MTLTexture>(*IS.FrameBufferTexture);
    auto &DS = llvm::cast<MTLTexture>(*IS.DepthStencil);
    auto &FBReadback = llvm::cast<MTLBuffer>(*IS.FrameBufferReadback);

    MTL::RenderPassDescriptor *Desc =
        MTL::RenderPassDescriptor::alloc()->init();

    const uint64_t Width = FBTex.Desc.Width;
    const uint64_t Height = FBTex.Desc.Height;

    // Color attachment.
    auto *CADesc = MTL::RenderPassColorAttachmentDescriptor::alloc()->init();
    CADesc->setTexture(FBTex.Tex);
    CADesc->setLoadAction(MTL::LoadActionClear);
    const auto *ColorCV =
        std::get_if<ClearColor>(&*FBTex.Desc.OptimizedClearValue);
    if (!ColorCV)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Render target clear value must be a ClearColor.");

    CADesc->setClearColor(
        MTL::ClearColor(ColorCV->R, ColorCV->G, ColorCV->B, ColorCV->A));
    CADesc->setStoreAction(MTL::StoreActionStore);
    Desc->colorAttachments()->setObject(CADesc, 0);

    // Depth/stencil attachment.
    const auto *DepthCV =
        std::get_if<ClearDepthStencil>(&*DS.Desc.OptimizedClearValue);
    if (!DepthCV)
      return llvm::createStringError(
          std::errc::invalid_argument,
          "Depth/stencil clear value must be a ClearDepthStencil.");

    auto *DADesc = Desc->depthAttachment();
    DADesc->setTexture(DS.Tex);
    DADesc->setLoadAction(MTL::LoadActionClear);
    DADesc->setClearDepth(DepthCV->Depth);
    DADesc->setStoreAction(MTL::StoreActionDontCare);

    auto *SADesc = Desc->stencilAttachment();
    SADesc->setTexture(DS.Tex);
    SADesc->setLoadAction(MTL::LoadActionClear);
    SADesc->setClearStencil(DepthCV->Stencil);
    SADesc->setStoreAction(MTL::StoreActionDontCare);

    MTL::RenderCommandEncoder *CmdEncoder =
        IS.CB->CmdBuffer->renderCommandEncoder(Desc);

    const auto &PS = llvm::cast<MTLPipelineState>(IS.Pipeline.get());
    CmdEncoder->setRenderPipelineState(PS->RenderPipeline);

    // Configure depth stencil state: depth test enabled, write all, less.
    MTL::DepthStencilDescriptor *DSDesc =
        MTL::DepthStencilDescriptor::alloc()->init();
    DSDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    DSDesc->setDepthWriteEnabled(true);
    MTL::DepthStencilState *DSState = Device->newDepthStencilState(DSDesc);
    CmdEncoder->setDepthStencilState(DSState);
    DSDesc->release();
    DSState->release();

    if (IS.DescHeap) {
      IS.DescHeap->bind(CmdEncoder);
      // NOTE: This code assumes 1 descriptor set (D3D12 backend also assumes
      // this)
      IS.ArgBuffer->setRootDescriptorTable(
          0, IS.DescHeap->getGPUDescriptorHandleForHeapStart());
    }
    IS.ArgBuffer->bind(CmdEncoder);
    for (const auto &Table : IS.DescTables)
      for (const auto &ResPair : Table.Resources)
        for (const auto &ResSet : ResPair.second)
          CmdEncoder->useResource(ResSet.Resource.get(),
                                  MTL::ResourceUsageRead |
                                      MTL::ResourceUsageWrite);

    // Explicitly set viewport to texture dimensions.
    CmdEncoder->setViewport(
        MTL::Viewport{0.0, 0.0, (double)Width, (double)Height, 0.0, 1.0});
    CmdEncoder->setCullMode(MTL::CullModeNone);

    // Bind vertex buffer at slot 0 to match the vertex descriptor which
    // references buffer index 0.
    CmdEncoder->setVertexBuffer(IS.VertexBuffer, 0, 0);

    CmdEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle, NS::UInteger(0),
                               P.Bindings.getVertexCount());

    CmdEncoder->endEncoding();

    // Blit the render target into the readback buffer for CPU access.
    MTL::BlitCommandEncoder *Blit = IS.CB->CmdBuffer->blitCommandEncoder();
    const size_t ElemSize = getFormatSizeInBytes(FBTex.Desc.Fmt);
    const size_t RowBytes = Width * ElemSize;
    Blit->copyFromTexture(FBTex.Tex, 0, 0, MTL::Origin(0, 0, 0),
                          MTL::Size(Width, Height, 1), FBReadback.Buf, 0,
                          RowBytes, 0);
    Blit->endEncoding();

    return llvm::Error::success();
  }

  llvm::Expected<offloadtest::SubmitResult>
  executeCommands(InvocationState &IS) {
    return GraphicsQueue.submit(std::move(IS.CB));
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

    if (P.isGraphics()) {
      CPUBuffer *RTarget = P.Bindings.RTargetBufferPtr;
      const uint64_t Width = RTarget->OutputProps.Width;
      const uint64_t Height = RTarget->OutputProps.Height;
      const size_t ElemSize = RTarget->getElementSize();
      const size_t RowBytes = Width * ElemSize;

      // Read from the readback buffer. The blit copied the texture data in
      // GPU layout order, so we flip rows here to produce an upright image.
      auto &FBReadback = llvm::cast<MTLBuffer>(*IS.FrameBufferReadback);
      const unsigned char *Src =
          reinterpret_cast<const unsigned char *>(FBReadback.Buf->contents());
      unsigned char *Buf =
          reinterpret_cast<unsigned char *>(RTarget->Data[0].get());
      for (uint64_t R = 0; R < Height; ++R) {
        const uint64_t SrcRow = (Height - 1) - R;
        memcpy(Buf + R * RowBytes, Src + SrcRow * RowBytes, RowBytes);
      }
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

  Queue &getGraphicsQueue() override { return GraphicsQueue; }

  llvm::Expected<std::unique_ptr<offloadtest::Fence>>
  createFence(llvm::StringRef Name) override {
    return MTLFence::create(Device, Name);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Buffer>>
  createBuffer(std::string Name, BufferCreateDesc &Desc,
               size_t SizeInBytes) override {
    MTL::Buffer *Buf = Device->newBuffer(
        SizeInBytes, getMetalBufferResourceOptions(Desc.Location));
    if (!Buf)
      return llvm::createStringError(std::errc::not_enough_memory,
                                     "Failed to create Metal buffer.");
    return std::make_unique<MTLBuffer>(Buf, Name, Desc, SizeInBytes);
  }

  llvm::Expected<std::unique_ptr<offloadtest::Texture>>
  createTexture(std::string Name, TextureCreateDesc &Desc) override {
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

  llvm::Expected<std::unique_ptr<offloadtest::CommandBuffer>>
  createCommandBuffer() override {
    return MTLCommandBuffer::create(GraphicsQueue.Queue);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineCs(llvm::StringRef Name, const InvocationState &IS,
                   const BindingsDesc & /*unused on metal*/,
                   ShaderContainer CS) override {
    NS::Error *Error = nullptr;
    const llvm::StringRef Program = CS.Shader->getBuffer();

    auto MetalIR = convertToMetalIR(P, Stages::Compute, CS, IS);
    if (!MetalIR)
      return MetalIR.takeError();

    IS.ComputeReflection = std::move(MetalIR->Reflection);

    dispatch_data_t Data = IRMetalLibGetBytecodeData(MetalIR->Binary.get());
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

    return std::make_unique<MTLPipelineState>(Name, PSO);
  }

  llvm::Expected<std::unique_ptr<PipelineState>>
  createPipelineVsPs(llvm::StringRef Name, const InvocationState &IS,
                     const BindingsDesc & /*unused on metal*/,
                     llvm::ArrayRef<InputLayoutDesc> InputLayout,
                     llvm::ArrayRef<Format> RTFormats,
                     std::optional<Format> DSFormat, ShaderContainer VS,
                     ShaderContainer PS) override {
    NS::Error *Error = nullptr;

    // Load vertex shader.
    auto VSMetalIR = convertToMetalIR(P, Stages::Vertex, VS, IS);
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
    auto PSMetalIR = convertToMetalIR(P, Stages::Pixel, PS, IS);
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

    MTL::RenderPipelineDescriptor *Desc =
        MTL::RenderPipelineDescriptor::alloc()->init();
    auto DescScope = llvm::scope_exit([&] { Desc->release(); });
    Desc->setVertexFunction(VSFn);
    Desc->setFragmentFunction(PSFn);

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

      if (FnAttrs->count() != InputLayout.size())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Mismatch between vertex shader attribute count and pipeline "
            "vertex input count.");

      // Collect the attribute indices the shader expects so that we can map the
      // specified attributes onto the correct indices.
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

        const uint32_t ElemSize = getFormatSizeInBytes(Elem.Fmt);
        MTL::VertexAttributeDescriptor *AttrDesc =
            MTL::VertexAttributeDescriptor::alloc()->init();
        AttrDesc->setBufferIndex(0);
        AttrDesc->setOffset(Elem.OffsetInBytes);
        AttrDesc->setFormat(getMetalVertexFormat(Elem.Fmt));
        VtxDesc->attributes()->setObject(AttrDesc, ShaderAttrIndices[AttrName]);
        AttrDesc->release();
        Stride = std::max(Stride, Elem.OffsetInBytes + ElemSize);
      }

      MTL::VertexBufferLayoutDescriptor *LDesc =
          MTL::VertexBufferLayoutDescriptor::alloc()->init();
      LDesc->setStride(Stride);
      LDesc->setStepRate(1);
      LDesc->setStepFunction(MTL::VertexStepFunctionPerVertex);
      VtxDesc->layouts()->setObject(LDesc, 0);
      LDesc->release();

      Desc->setVertexDescriptor(VtxDesc);
    }

    // Configure render target color attachments.
    for (size_t I = 0; I < RTFormats.size(); ++I) {
      MTL::RenderPipelineColorAttachmentDescriptor *RPCA =
          MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init();
      RPCA->setPixelFormat(getMetalPixelFormat(RTFormats[I]));
      Desc->colorAttachments()->setObject(RPCA, I);
      RPCA->release();
    }

    // Configure depth/stencil attachment.
    if (DSFormat) {
      const MTL::PixelFormat DSPixelFormat = getMetalPixelFormat(*DSFormat);
      Desc->setDepthAttachmentPixelFormat(DSPixelFormat);
      if (isStencilFormat(*DSFormat))
        Desc->setStencilAttachmentPixelFormat(DSPixelFormat);
    }

    MTL::RenderPipelineState *PSO =
        Device->newRenderPipelineState(Desc, &Error);
    if (Error)
      return toError(Error);

    return std::make_unique<MTLPipelineState>(Name, PSO);
  }

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState IS;

    auto CBOrErr = MTLCommandBuffer::create(GraphicsQueue.Queue);
    if (!CBOrErr)
      return CBOrErr.takeError();
    IS.CB = std::move(*CBOrErr);

    if (auto Err = createRootSignature(P, IS))
      return Err;

    if (auto Err = createDescriptorHeap(P, IS))
      return Err;

    if (auto Err = createBuffers(P, IS))
      return Err;

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
          createPipelineCs("Compute Pipeline State", IS, Bindings, CS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      IS.Pipeline = std::move(*PipelineStateOrErr);
      llvm::outs() << "Compute Pipeline created.\n";

      if (auto Err = createComputeCommands(P, IS))
        return Err;
    } else {
      ShaderContainer VS = {};
      ShaderContainer PS = {};
      for (auto &Shader : P.Shaders) {
        if (Shader.Stage == Stages::Vertex) {
          VS.EntryPoint = Shader.Entry;
          VS.Shader = Shader.Shader.get();
        } else if (Shader.Stage == Stages::Pixel) {
          PS.EntryPoint = Shader.Entry;
          PS.Shader = Shader.Shader.get();
        }
      }

      llvm::SmallVector<InputLayoutDesc> InputLayout;
      for (auto &Attr : P.Bindings.VertexAttributes) {
        auto FormatOrErr = toFormat(Attr.Format, Attr.Channels);
        if (!FormatOrErr)
          return FormatOrErr.takeError();

        InputLayoutDesc Desc = {};
        Desc.Name = Attr.Name;
        Desc.Fmt = *FormatOrErr;
        Desc.OffsetInBytes = Attr.Offset;
        InputLayout.push_back(Desc);
      }

      auto FormatOrErr = toFormat(P.Bindings.RTargetBufferPtr->Format,
                                  P.Bindings.RTargetBufferPtr->Channels);
      if (!FormatOrErr)
        return FormatOrErr.takeError();

      llvm::SmallVector<Format> RTFormats;
      RTFormats.push_back(*FormatOrErr);

      auto PipelineStateOrErr = createPipelineVsPs(
          "Graphics Pipeline State", IS, Bindings, InputLayout, RTFormats,
          Format::D32FloatS8Uint, VS, PS);
      if (!PipelineStateOrErr)
        return PipelineStateOrErr.takeError();
      IS.Pipeline = std::move(*PipelineStateOrErr);

      if (auto Err = createGraphicsCommands(P, IS))
        return Err;
    }

    auto SubmitResult = executeCommands(IS);
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
  void queryCapabilities() {}
};
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
