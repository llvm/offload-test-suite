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
      if (Fn)
        Fn->release();
      if (Lib)
        Lib->release();
      if (PipelineState)
        PipelineState->release();
      if (Queue)
        Queue->release();

      Pool->release();
    }

    NS::AutoreleasePool *Pool = nullptr;
    MTL::CommandQueue *Queue = nullptr;
    MTL::Library *Lib = nullptr;
    MTL::Function *Fn = nullptr;
    MTL::ComputePipelineState *PipelineState;
    MTL::Buffer *ArgBuffer;
    llvm::SmallVector<MTL::Texture *> Textures;
    llvm::SmallVector<MTL::Buffer *> Buffers;
  };

  llvm::Error loadShaders(InvocationState &IS, const Shader &P) {
    NS::Error *Error = nullptr;
    const llvm::StringRef Program = P.Shader->getBuffer();
    dispatch_data_t Data = dispatch_data_create(Program.data(), Program.size(),
                                                dispatch_get_main_queue(),
                                                ^{
                                                });
    IS.Lib = Device->newLibrary(Data, &Error);
    if (Error)
      return toError(Error);

    IS.Fn = IS.Lib->newFunction(
        NS::String::string(P.Entry.c_str(), NS::UTF8StringEncoding));
    IS.PipelineState = Device->newComputePipelineState(IS.Fn, &Error);
    if (Error)
      return toError(Error);

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
    const size_t TableSize =
        sizeof(IRDescriptorTableEntry) * P.getDescriptorCount();
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
    return llvm::Error::success();
  }

  llvm::Error executeCommands(Pipeline &P, InvocationState &IS) {
    MTL::CommandBuffer *CmdBuffer = IS.Queue->commandBuffer();

    MTL::ComputeCommandEncoder *CmdEncoder = CmdBuffer->computeCommandEncoder();

    CmdEncoder->setComputePipelineState(IS.PipelineState);
    CmdEncoder->setBuffer(IS.ArgBuffer, 0, 2);
    for (uint64_t I = 0; I < IS.Textures.size(); ++I)
      CmdEncoder->useResource(IS.Textures[I],
                              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);
    for (uint64_t I = 0; I < IS.Buffers.size(); ++I)
      CmdEncoder->useResource(IS.Buffers[I],
                              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);

    const NS::UInteger TGS = IS.PipelineState->maxTotalThreadsPerThreadgroup();
    const llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
    const MTL::Size GridSize =
        MTL::Size(TGS * DispatchSize[0], DispatchSize[1], DispatchSize[2]);
    const MTL::Size GroupSize(TGS, 1, 1);

    CmdEncoder->dispatchThreads(GridSize, GroupSize);
    CmdEncoder->memoryBarrier(MTL::BarrierScopeBuffers);

    CmdEncoder->endEncoding();

    CmdBuffer->commit();
    CmdBuffer->waitUntilCompleted();

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
    return llvm::Error::success();
  }

public:
  MTLDevice(MTL::Device *D) : Device(D) {
    Description = Device->name()->utf8String();
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
    if (auto Err = loadShaders(IS, P.Shaders[0]))
      return Err;

    if (auto Err = createBuffers(P, IS))
      return Err;

    if (auto Err = executeCommands(P, IS))
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

llvm::Error Device::initializeMtlDevices() {
  return MTLContext::instance().initialize();
}
