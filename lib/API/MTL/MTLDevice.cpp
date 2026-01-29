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
#include "Support/Pipeline.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Error.h"
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
class MTLDevice : public offloadtest::Device {
  Capabilities Caps;
  MTL::Device *Device;

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
      if (Queue)
        Queue->release();

      Pool->release();
    }

    NS::AutoreleasePool *Pool = nullptr;
    MTL::CommandQueue *Queue = nullptr;
    MTL::ComputePipelineState *ComputePipeline = nullptr;
    MTL::RenderPipelineState *RenderPipeline = nullptr;
    MTL::Buffer *ArgBuffer;
    MTL::Buffer *VertexBuffer;
    MTL::VertexDescriptor *VertexDescriptor;
    llvm::SmallVector<MTL::Texture *> Textures;
    llvm::SmallVector<MTL::Buffer *> Buffers;
    MTL::Texture *FrameBufferTexture = nullptr;
    MTL::CommandBuffer *CmdBuffer = nullptr;
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
        if (A && A->active()) {
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
    IS.CmdBuffer = IS.Queue->commandBuffer();

    MTL::ComputeCommandEncoder *CmdEncoder =
        IS.CmdBuffer->computeCommandEncoder();

    CmdEncoder->setComputePipelineState(IS.ComputePipeline);
    CmdEncoder->setBuffer(IS.ArgBuffer, 0, 2);
    for (uint64_t I = 0; I < IS.Textures.size(); ++I)
      CmdEncoder->useResource(IS.Textures[I],
                              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    for (uint64_t I = 0; I < IS.Buffers.size(); ++I)
      CmdEncoder->useResource(IS.Buffers[I],
                              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);

    const NS::UInteger TGS =
        IS.ComputePipeline->maxTotalThreadsPerThreadgroup();
    const llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
    const MTL::Size GridSize =
        MTL::Size(TGS * DispatchSize[0], DispatchSize[1], DispatchSize[2]);
    const MTL::Size GroupSize(TGS, 1, 1);
    CmdEncoder->dispatchThreads(GridSize, GroupSize);
    CmdEncoder->memoryBarrier(MTL::BarrierScopeBuffers);

    CmdEncoder->endEncoding();
    return llvm::Error::success();
  }

  llvm::Error createGraphicsCommands(Pipeline &P, InvocationState &IS) {
    IS.CmdBuffer = IS.Queue->commandBuffer();

    MTL::RenderPassDescriptor *Desc =
        MTL::RenderPassDescriptor::alloc()->init();

    // Setup the render target texture.
    Buffer *RTarget = P.Bindings.RTargetBufferPtr;

    const MTL::PixelFormat Format =
        getMTLFormat(RTarget->Format, RTarget->Channels);

    const uint64_t Width = RTarget->OutputProps.Width;
    const uint64_t Height = RTarget->OutputProps.Height;
    MTL::TextureDescriptor *TDesc = MTL::TextureDescriptor::texture2DDescriptor(
        Format, Width, Height, false);
    // Create a shared texture used for both rendering and CPU readback.
    MTL::TextureDescriptor *SharedDesc = TDesc->copy();
    SharedDesc->setUsage(MTL::TextureUsageRenderTarget |
                         MTL::TextureUsageShaderRead |
                         MTL::TextureUsageShaderWrite);
    SharedDesc->setStorageMode(MTL::StorageModeShared);
    IS.FrameBufferTexture = Device->newTexture(SharedDesc);

    auto *CADesc = MTL::RenderPassColorAttachmentDescriptor::alloc()->init();
    CADesc->setTexture(IS.FrameBufferTexture);
    CADesc->setLoadAction(MTL::LoadActionClear);
    CADesc->setClearColor(MTL::ClearColor());
    CADesc->setStoreAction(MTL::StoreActionStore);
    Desc->colorAttachments()->setObject(CADesc, 0);

    MTL::RenderCommandEncoder *CmdEncoder =
        IS.CmdBuffer->renderCommandEncoder(Desc);

    CmdEncoder->setRenderPipelineState(IS.RenderPipeline);
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

    return llvm::Error::success();
  }

  llvm::Error executeCommands(InvocationState &IS) {
    IS.CmdBuffer->commit();
    IS.CmdBuffer->waitUntilCompleted();

    // Check and surface any errors that occurred during execution.
    NS::Error *CBErr = IS.CmdBuffer->error();
    if (CBErr)
      return toError(CBErr);

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
      Buffer *RTarget = P.Bindings.RTargetBufferPtr;
      const uint64_t Width = RTarget->OutputProps.Width;
      const uint64_t Height = RTarget->OutputProps.Height;
      const size_t ElemSize = RTarget->getElementSize();
      const size_t RowBytes = Width * ElemSize;

      // Read the framebuffer one row at a time into the output buffer.
      // Read rows from the texture bottom-to-top into the buffer top-to-bottom
      // so the final image is upright.
      unsigned char *Buf =
          reinterpret_cast<unsigned char *>(RTarget->Data[0].get());
      for (uint64_t R = 0; R < Height; ++R) {
        const uint32_t SrcRow = (uint32_t)((Height - 1) - R);
        unsigned char *Dst = Buf + R * RowBytes;
        IS.FrameBufferTexture->getBytes(
            Dst, RowBytes, MTL::Region(0, SrcRow, (uint32_t)Width, 1), 0);
      }
    }
    return llvm::Error::success();
  }

public:
  MTLDevice(MTL::Device *D) : Device(D) {
    Description = Device->name()->utf8String();
  }
  uint32_t getSubgroupSize() const override {
    const char *Src = R"(
      #include <metal_stdlib>
      using namespace metal;
      kernel void k() {}
    )";
    NS::Error *Err = nullptr;
    MTL::Library *Lib = Device->newLibrary(
        NS::String::string(Src, NS::UTF8StringEncoding), nullptr, &Err);
    if (!Lib) {
      if (Err)
        Err->release();
      return 0;
    }
    MTL::Function *Func =
        Lib->newFunction(NS::String::string("k", NS::UTF8StringEncoding));
    Lib->release();
    if (!Func)
      return 0;
    MTL::ComputePipelineState *PSO =
        Device->newComputePipelineState(Func, &Err);
    Func->release();
    if (!PSO) {
      if (Err)
        Err->release();
      return 0;
    }
    uint32_t SubgroupSize = PSO->threadExecutionWidth();
    PSO->release();
    return SubgroupSize;
  }
  void printExtra(llvm::raw_ostream &OS) override {
    OS << "  SubgroupSize: " << getSubgroupSize() << "\n";
  }

  std::pair<uint32_t, uint32_t> getMinMaxSubgroupSize() const override {
    // Metal currently only exposes a single subgroup size.
    const uint32_t SGSize = getSubgroupSize();
    return {SGSize, SGSize};
  }

  const Capabilities &getCapabilities() override {
    if (Caps.empty())
      queryCapabilities();
    return Caps;
  }

  llvm::StringRef getAPIName() const override { return "Metal"; };
  GPUAPI getAPI() const override { return GPUAPI::Metal; };

  llvm::Error executeProgram(Pipeline &P) override {
    InvocationState IS;
    IS.Queue = Device->newCommandQueue();

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

class MTLContext {
  MTLContext() = default;
  ~MTLContext() {}
  MTLContext(const MTLContext &) = delete;

  llvm::SmallVector<std::shared_ptr<MTLDevice>> Devices;

public:
  static MTLContext &instance() {
    static MTLContext Ctx;
    return Ctx;
  }

  llvm::Error initialize() {
    auto DefaultDev =
        std::make_shared<MTLDevice>(MTL::CreateSystemDefaultDevice());
    Devices.push_back(DefaultDev);
    Device::registerDevice(std::static_pointer_cast<Device>(DefaultDev));
    return llvm::Error::success();
  }
};

} // namespace

llvm::Error Device::initializeMtlDevices(const DeviceConfig /*Config*/) {
  return MTLContext::instance().initialize();
}
