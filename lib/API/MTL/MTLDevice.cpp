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
  std::error_code EC =
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
      for (auto T : Textures)
        T->release();
      for (auto B : Buffers)
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

  llvm::Error loadShaders(InvocationState &IS, llvm::StringRef Program) {
    NS::Error *Error = nullptr;
    dispatch_data_t data = dispatch_data_create(Program.data(), Program.size(),
                                                dispatch_get_main_queue(),
                                                ^{
                                                });
    IS.Lib = Device->newLibrary(data, &Error);
    if (Error)
      return toError(Error);

    IS.Fn =
        IS.Lib->newFunction(NS::String::string("main", NS::UTF8StringEncoding));
    IS.PipelineState = Device->newComputePipelineState(IS.Fn, &Error);
    if (Error)
      return toError(Error);

    return llvm::Error::success();
  }

  llvm::Error createUAV(Resource &R, InvocationState &IS,
                        const uint32_t HeapIdx) {
    auto *TablePtr = (IRDescriptorTableEntry *)IS.ArgBuffer->contents();

    if (R.isRaw()) {
      MTL::Buffer *Buf =
          Device->newBuffer(R.Data.get(), R.Size,
                            MTL::ResourceStorageModeManaged);
      IRBufferView View = {};
      View.buffer = Buf;
      View.bufferSize = R.Size;

      IRDescriptorTableSetBufferView(&TablePtr[HeapIdx], &View);
      IS.Buffers.push_back(Buf);
    } else {
      uint64_t Width = R.Size / R.getElementSize();
      MTL::TextureDescriptor *Desc =
          MTL::TextureDescriptor::textureBufferDescriptor(
              getMTLFormat(R.Format, R.Channels), Width,
              MTL::StorageModeManaged,
              MTL::ResourceUsageRead | MTL::ResourceUsageWrite);

      MTL::Texture *NewTex = Device->newTexture(Desc);
      NewTex->replaceRegion(MTL::Region(0, 0, Width, 1), 0, R.Data.get(), 0);

      IS.Textures.push_back(NewTex);

      IRDescriptorTableSetTexture(&TablePtr[HeapIdx], NewTex, 0, 0);
    }

    return llvm::Error::success();
  }

  llvm::Error createSRV(Resource &R, InvocationState &IS,
                        const uint32_t HeapIdx) {
    auto *TablePtr = (IRDescriptorTableEntry *)IS.ArgBuffer->contents();

    if (R.isRaw()) {
      MTL::Buffer *Buf = Device->newBuffer(R.Data.get(), R.Size,
                                           MTL::ResourceStorageModeManaged);
      IRBufferView View = {};
      View.buffer = Buf;
      View.bufferSize = R.Size;

      IRDescriptorTableSetBufferView(&TablePtr[HeapIdx], &View);
      IS.Buffers.push_back(Buf);
    } else {
      uint64_t Width = R.Size / R.getElementSize();
      MTL::TextureDescriptor *Desc =
          MTL::TextureDescriptor::textureBufferDescriptor(
              getMTLFormat(R.Format, R.Channels), Width,
              MTL::ResourceStorageModeManaged, MTL::ResourceUsageRead);

      MTL::Texture *NewTex = Device->newTexture(Desc);
      NewTex->replaceRegion(MTL::Region(0, 0, Width, 1), 0, R.Data.get(), 0);

      IS.Textures.push_back(NewTex);

      IRDescriptorTableSetTexture(&TablePtr[HeapIdx], NewTex, 0, 0);
    }

    return llvm::Error::success();
  }

  llvm::Error createCBV(Resource &R, InvocationState &IS,
                        const uint32_t HeapIdx) {
    return llvm::createStringError(std::errc::not_supported,
                                   "MTLDevice::createCBV not supported.");
  }

  llvm::Error createBuffers(Pipeline &P, InvocationState &IS) {
    size_t TableSize = sizeof(IRDescriptorTableEntry) * P.getDescriptorCount();
    IS.ArgBuffer =
        Device->newBuffer(TableSize, MTL::ResourceStorageModeManaged);

    uint32_t HeapIndex = 0;
    for (auto &D : P.Sets) {
      for (auto &R : D.Resources) {
        switch (R.Access) {
        case DataAccess::ReadOnly:
          if (auto Err = createSRV(R, IS, HeapIndex++))
            return Err;
          break;
        case DataAccess::ReadWrite:
          if (auto Err = createUAV(R, IS, HeapIndex++))
            return Err;
          break;
        case DataAccess::Constant:
          if (auto Err = createCBV(R, IS, HeapIndex++))
            return Err;
          break;
        }
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

    NS::UInteger TGS = IS.PipelineState->maxTotalThreadsPerThreadgroup();
    MTL::Size GridSize = MTL::Size(TGS * P.DispatchSize[0], P.DispatchSize[1],
                                   P.DispatchSize[2]);
    MTL::Size GroupSize(TGS, 1, 1);

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
        switch (R.Access) {

        case DataAccess::ReadWrite: {
          if (R.isRaw()) {
            memcpy(R.Data.get(), IS.Buffers[BufferIndex++]->contents(), R.Size);
          } else {
            uint64_t Width = R.Size / R.getElementSize();
            IS.Textures[TextureIndex++]->getBytes(
                R.Data.get(), 0, MTL::Region(0, 0, Width, 1), 0);
          }
          break;
        }
        case DataAccess::ReadOnly:
          // Nothing to copy back, just increment the appropriate index.
          if (R.isRaw())
            ++BufferIndex;
          else
            ++TextureIndex;
          break;
        case DataAccess::Constant:
          return llvm::createStringError(
              std::errc::not_supported, "MTLDevice does not support Constant.");
          break;
        }
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

  llvm::Error executeProgram(llvm::StringRef Program, Pipeline &P) override {
    InvocationState IS;
    IS.Queue = Device->newCommandQueue();
    if (auto Err = loadShaders(IS, Program))
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

llvm::Error InitializeMTLDevices() {
  return MTLContext::instance().initialize();
}
