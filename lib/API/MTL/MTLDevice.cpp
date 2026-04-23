#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include "QuartzCore/QuartzCore.hpp"

#define IR_RUNTIME_METALCPP
#define IR_PRIVATE_IMPLEMENTATION
#include "metal_irconverter_runtime.h"

#include "API/Device.h"
#include "MTLResources.h"
#include "Support/Pipeline.h"

#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

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

#define MTLVTXFormats(Base)                                                    \
  if (Channels == 1)                                                           \
    return MTL::VertexFormat##Base;                                            \
  if (Channels == 2)                                                           \
    return MTL::VertexFormat##Base##2;                                         \
  if (Channels == 3)                                                           \
    return MTL::VertexFormat##Base##3;                                         \
  if (Channels == 4)                                                           \
    return MTL::VertexFormat##Base##4;

static MTL::VertexFormat getMTLVertexFormat(DataFormat Format, int Channels) {
  switch (Format) {
  case DataFormat::Float32:
    MTLVTXFormats(Float) break;
  default:
    llvm_unreachable("Unsupported Resource format specified");
  }
  return MTL::VertexFormatInvalid;
}

namespace {
class MTLQueue : public offloadtest::Queue {
public:
  MTL::CommandQueue *Queue;
  MTLQueue(MTL::CommandQueue *Queue) : Queue(Queue) {}
  ~MTLQueue() {
    if (Queue)
      Queue->release();
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

class MTLDevice : public offloadtest::Device {
  Capabilities Caps;
  MTL::Device *Device;
  MTLQueue GraphicsQueue;

  struct InvocationState {
    InvocationState() { Pool = NS::AutoreleasePool::alloc()->init(); }
    ~InvocationState() {
      for (MTL::Texture *T : Textures)
        T->release();
      for (MTL::Buffer *B : Buffers)
        B->release();
      if (ComputePipeline)
        ComputePipeline->release();
      if (RenderPipeline)
        RenderPipeline->release();

      Pool->release();
    }

    NS::AutoreleasePool *Pool = nullptr;
    MTL::ComputePipelineState *ComputePipeline = nullptr;
    MTL::RenderPipelineState *RenderPipeline = nullptr;
    MTL::Buffer *ArgBuffer;
    MTL::Buffer *VertexBuffer;
    MTL::VertexDescriptor *VertexDescriptor;
    llvm::SmallVector<MTL::Texture *> Textures;
    llvm::SmallVector<MTL::Buffer *> Buffers;
    std::unique_ptr<offloadtest::Texture> FrameBufferTexture;
    std::unique_ptr<offloadtest::Buffer> FrameBufferReadback;
    std::unique_ptr<offloadtest::Texture> DepthStencil;
    std::unique_ptr<MTLCommandBuffer> CB;
    std::unique_ptr<offloadtest::Fence> CompletionFence;
  };

  llvm::Error setupVertexShader(InvocationState &IS, const Pipeline &P,
                                MTL::Function *Fn) {
    if (P.Bindings.VertexBufferPtr) {
      NS::Array *FnAttrs = Fn->vertexAttributes();
      // I'm not really sure if there's any valid case for a vertex shader with
      // no vertex attributes, so we just error if that ever occurs.
      if (!FnAttrs)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Vertex shader has no vertex attributes.");
      if (FnAttrs->count() != P.Bindings.VertexAttributes.size())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Mismatch between vertex shader attribute count and pipeline "
            "vertex input count.");
      // Collect the attribute indices the shader expects so that we can map the
      // specified attributes onto the correct indices.
      llvm::StringMap<uint32_t> ShaderAttrIndices;
      for (uint32_t Ai = 0; Ai < FnAttrs->count(); ++Ai) {
        auto *A = static_cast<MTL::VertexAttribute *>(FnAttrs->object(Ai));
        if (A && A->isActive()) {
          ShaderAttrIndices.insert(std::make_pair(
              llvm::StringRef(A->name()->utf8String()), A->attributeIndex()));
          llvm::errs() << "Shader attr: " << A->name()->utf8String()
                       << " at index " << A->attributeIndex() << "\n";
        }
      }

      IS.VertexDescriptor = MTL::VertexDescriptor::alloc()->init();
      const uint32_t Stride = P.Bindings.getVertexStride();
      for (const VertexAttribute &VA : P.Bindings.VertexAttributes) {
        llvm::SmallString<32> AttrName(VA.Name);
        llvm::transform(AttrName, AttrName.begin(), tolower);
        // Append a zero since we're only supporting one attribute per name.
        // We'll need to revisit this if we ever support indexed attributes.
        AttrName += "0";
        MTL::VertexAttributeDescriptor *VADesc =
            MTL::VertexAttributeDescriptor::alloc()->init();
        VADesc->setBufferIndex(0);
        VADesc->setOffset(VA.Offset);
        VADesc->setFormat(getMTLVertexFormat(VA.Format, VA.Channels));
        IS.VertexDescriptor->attributes()->setObject(
            VADesc, ShaderAttrIndices[AttrName]);
      }

      MTL::VertexBufferLayoutDescriptor *LDesc =
          MTL::VertexBufferLayoutDescriptor::alloc()->init();
      LDesc->setStride(Stride);
      LDesc->setStepRate(1);
      LDesc->setStepFunction(MTL::VertexStepFunctionPerVertex);
      IS.VertexDescriptor->layouts()->setObject(LDesc, 0);
    }
    return llvm::Error::success();
  }

  llvm::Error loadShaders(InvocationState &IS, const Pipeline &P) {
    NS::Error *Error = nullptr;
    if (P.isCompute()) {
      // This is an arbitrary distinction that we could alter in the future.
      if (P.Shaders.size() != 1)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Compute pipeline must have exactly one compute shader.");
      const llvm::StringRef Program = P.Shaders[0].Shader->getBuffer();
      dispatch_data_t Data = dispatch_data_create(
          Program.data(), Program.size(), dispatch_get_main_queue(),
          ^{
          });
      MTL::Library *Lib = Device->newLibrary(Data, &Error);
      if (Error)
        return toError(Error);
      IS.Pool->addObject(Lib);

      MTL::Function *Fn = Lib->newFunction(NS::String::string(
          P.Shaders[0].Entry.c_str(), NS::UTF8StringEncoding));
      IS.ComputePipeline = Device->newComputePipelineState(Fn, &Error);
      if (Error)
        return toError(Error);
      IS.Pool->addObject(Fn);
    } else {
      MTL::RenderPipelineDescriptor *Desc =
          MTL::RenderPipelineDescriptor::alloc()->init();
      IS.Pool->addObject(Desc);
      for (const auto &S : P.Shaders) {
        const llvm::StringRef Program = S.Shader->getBuffer();
        dispatch_data_t Data = dispatch_data_create(
            Program.data(), Program.size(), dispatch_get_main_queue(),
            ^{
            });
        MTL::Library *Lib = Device->newLibrary(Data, &Error);
        if (Error)
          return toError(Error);
        IS.Pool->addObject(Lib);

        MTL::Function *Fn = Lib->newFunction(
            NS::String::string(S.Entry.c_str(), NS::UTF8StringEncoding));
        switch (S.Stage) {
        case Stages::Vertex:
          Desc->setVertexFunction(Fn);
          if (llvm::Error Err = setupVertexShader(IS, P, Fn))
            return Err;

          Desc->setVertexDescriptor(IS.VertexDescriptor);
          break;
        case Stages::Pixel:
          Desc->setFragmentFunction(Fn);
          break;
        case Stages::Compute:
          return llvm::createStringError(
              std::errc::not_supported,
              "Metal: Compute shader invalid with render pipeline!");
        }
        if (Error)
          return toError(Error);
        IS.Pool->addObject(Fn);
      }

      // TODO: Add support for more shader stages and different pipeline shapes.
      if (Desc->vertexFunction() == nullptr ||
          Desc->fragmentFunction() == nullptr)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Graphics pipeline requires both a vertex shader and a fragment "
            "shader.");

      if (P.Bindings.RTargetBufferPtr) {
        // Configure the render target color attachment.
        const MTL::PixelFormat PF =
            getMTLFormat(P.Bindings.RTargetBufferPtr->Format,
                         P.Bindings.RTargetBufferPtr->Channels);
        MTL::RenderPipelineColorAttachmentDescriptor *RPCA =
            MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init();
        RPCA->setPixelFormat(PF);
        Desc->colorAttachments()->setObject(RPCA, 0);

        // Set the depth/stencil format on the pipeline descriptor.
        const MTL::PixelFormat DepthFmt =
            getMetalPixelFormat(Format::D32FloatS8Uint);
        Desc->setDepthAttachmentPixelFormat(DepthFmt);
        Desc->setStencilAttachmentPixelFormat(DepthFmt);
      }

      IS.RenderPipeline = Device->newRenderPipelineState(Desc, &Error);
      if (Error)
        return toError(Error);
    }

    return llvm::Error::success();
  }

  llvm::Error createDescriptor(Resource &R, InvocationState &IS,
                               const uint32_t HeapIdx) {
    auto *TablePtr = (IRDescriptorTableEntry *)IS.ArgBuffer->contents();

    assert(R.BufferPtr->ArraySize == 1 &&
           "Resource arrays are not yet supported on Metal.");

    if (R.isRaw()) {
      MTL::Buffer *Buf =
          Device->newBuffer(R.BufferPtr->Data.back().get(), R.size(),
                            MTL::ResourceStorageModeManaged);
      IRBufferView View = {};
      View.buffer = Buf;
      View.bufferSize = R.size();

      IRDescriptorTableSetBufferView(&TablePtr[HeapIdx], &View);
      IS.Buffers.push_back(Buf);
    } else {
      const uint64_t Width = R.isTexture() ? R.BufferPtr->OutputProps.Width
                                           : R.size() / R.getElementSize();
      const uint64_t Height =
          R.isTexture() ? R.BufferPtr->OutputProps.Height : 1;
      MTL::TextureUsage UsageFlags = MTL::ResourceUsageRead;
      if (R.isReadWrite())
        UsageFlags |= MTL::ResourceUsageWrite;
      MTL::TextureDescriptor *Desc = nullptr;
      const MTL::PixelFormat Format =
          getMTLFormat(R.BufferPtr->Format, R.BufferPtr->Channels);
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
                            R.BufferPtr->Data.back().get(),
                            Width * R.getElementSize());

      IS.Textures.push_back(NewTex);

      IRDescriptorTableSetTexture(&TablePtr[HeapIdx], NewTex, 0, 0);
    }

    return llvm::Error::success();
  }

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    const size_t ResourceCount = P.getDescriptorCount();
    const size_t TableSize = sizeof(IRDescriptorTableEntry) * ResourceCount;

    if (TableSize > 0) {
      IS.ArgBuffer =
          Device->newBuffer(TableSize, MTL::ResourceStorageModeManaged);
      uint32_t HeapIndex = 0;
      for (auto &D : P.Sets) {
        for (auto &R : D.Resources) {
          if (auto Err = createDescriptor(R, IS, HeapIndex++))
            return Err;
        }
      }
      IS.ArgBuffer->didModifyRange(NS::Range::Make(0, IS.ArgBuffer->length()));
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

    CmdEncoder->setComputePipelineState(IS.ComputePipeline);
    CmdEncoder->setBuffer(IS.ArgBuffer, 0, 2);
    for (uint64_t I = 0; I < IS.Textures.size(); ++I)
      CmdEncoder->useResource(IS.Textures[I],
                              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    for (uint64_t I = 0; I < IS.Buffers.size(); ++I)
      CmdEncoder->useResource(IS.Buffers[I],
                              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);

    NS::UInteger TGS[3] = {IS.ComputePipeline->maxTotalThreadsPerThreadgroup(),
                           1, 1};
    if (P.Shaders[0].Reflection) {
      llvm::Expected<llvm::json::Value> E = llvm::json::parse(
          llvm::StringRef(P.Shaders[0].Reflection->getBuffer()));
      if (!E)
        return E.takeError();
      llvm::json::Value Reflection = *E;

      const llvm::json::Object *ReflectionObj = Reflection.getAsObject();
      if (!ReflectionObj)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Shader reflection must be a JSON object.");
      auto StateIt = ReflectionObj->find("state");
      if (StateIt == ReflectionObj->end())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Key 'state' not found in shader reflection.");
      const llvm::json::Object *State = StateIt->second.getAsObject();
      auto TGSize = State->find("tg_size");
      if (TGSize == State->end())
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Key 'tg_size' not found in shader reflection.");
      const llvm::json::Array *TGSizeArr = TGSize->second.getAsArray();
      if (TGSizeArr->size() != 3)
        return llvm::createStringError(
            std::errc::invalid_argument,
            "Threadgroup size in reflection must have three components.");
      for (size_t I = 0; I < 3; ++I) {
        auto OpVal = (*TGSizeArr)[I].getAsUINT64();
        if (!OpVal)
          return llvm::createStringError(std::errc::invalid_argument,
                                         "Threadgroup size components in "
                                         "reflection must be integers.");
        TGS[I] = *OpVal;
      }
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

    CmdEncoder->setRenderPipelineState(IS.RenderPipeline);

    // Configure depth stencil state: depth test enabled, write all, less.
    MTL::DepthStencilDescriptor *DSDesc =
        MTL::DepthStencilDescriptor::alloc()->init();
    DSDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    DSDesc->setDepthWriteEnabled(true);
    MTL::DepthStencilState *DSState = Device->newDepthStencilState(DSDesc);
    CmdEncoder->setDepthStencilState(DSState);
    DSDesc->release();
    DSState->release();

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

  llvm::Error executeCommands(InvocationState &IS) {
    // This is a hack but it works since this is all single threaded code.
    static uint64_t FenceCounter = 0;
    const uint64_t CurrentCounter = FenceCounter + 1;
    auto *F = static_cast<MTLFence *>(IS.CompletionFence.get());

    IS.CB->CmdBuffer->encodeSignalEvent(F->Event, CurrentCounter);
    IS.CB->CmdBuffer->commit();

    if (auto Err = IS.CompletionFence->waitForCompletion(CurrentCounter))
      return Err;

    // Check and surface any errors that occurred during execution.
    NS::Error *CBErr = IS.CB->CmdBuffer->error();
    if (CBErr)
      return toError(CBErr);

    FenceCounter = CurrentCounter;
    return llvm::Error::success();
  }

  llvm::Error copyBack(Pipeline &P, InvocationState &IS) {
    uint32_t TextureIndex = 0;
    uint32_t BufferIndex = 0;
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        assert(R.BufferPtr->ArraySize == 1 &&
               "Resource arrays are not yet supported on Metal.");
        if (R.isReadOnly()) {
          if (R.isRaw())
            ++BufferIndex;
          else
            ++TextureIndex;
          continue;
        }
        if (R.isRaw()) {
          memcpy(R.BufferPtr->Data.back().get(),
                 IS.Buffers[BufferIndex++]->contents(), R.size());
          continue;
        }
        const uint64_t Width = R.isTexture() ? R.BufferPtr->OutputProps.Width
                                             : R.size() / R.getElementSize();
        const uint64_t Height =
            R.isTexture() ? R.BufferPtr->OutputProps.Height : 1;
        IS.Textures[TextureIndex++]->getBytes(
            R.BufferPtr->Data.back().get(), Width * R.getElementSize(),
            MTL::Region(0, 0, Width, Height), 0);
      }
    }
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
  MTLDevice(MTL::Device *D, MTL::CommandQueue *Q)
      : Device(D), GraphicsQueue(MTLQueue(Q)) {
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

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState IS;

    auto CBOrErr = MTLCommandBuffer::create(GraphicsQueue.Queue);
    if (!CBOrErr)
      return CBOrErr.takeError();
    IS.CB = std::move(*CBOrErr);

    auto FenceOrErr = createFence("Fence");
    if (!FenceOrErr)
      return FenceOrErr.takeError();
    IS.CompletionFence = std::move(*FenceOrErr);

    if (auto Err = createBuffers(P, IS))
      return Err;

    if (auto Err = loadShaders(IS, P))
      return Err;

    if (P.isCompute()) {
      if (auto Err = createComputeCommands(P, IS))
        return Err;
      llvm::outs() << "Created compute commands.\n";
    } else {
      if (auto Err = createGraphicsCommands(P, IS))
        return Err;
      llvm::outs() << "Created graphics commands.\n";
    }

    if (auto Err = executeCommands(IS))
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

  auto DefaultDev = std::make_unique<MTLDevice>(MetalDevice, MetalQueue);
  Devices.push_back(std::move(DefaultDev));

  return llvm::Error::success();
}
