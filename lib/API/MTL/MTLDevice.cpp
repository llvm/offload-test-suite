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

bool IsTexture(offloadtest::ResourceKind RK) {
  switch (RK) {
  case ResourceKind::Buffer:
  case ResourceKind::RWBuffer:
  case ResourceKind::Texture2D:
    return true;
  case ResourceKind::StructuredBuffer:
  case ResourceKind::RWStructuredBuffer:
  case ResourceKind::ConstantBuffer:
    return false;
  }
  llvm_unreachable("All cases handled");
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

  llvm::Error loadShaders(InvocationState &IS, const Shader &P) {
    NS::Error *Error = nullptr;
    llvm::StringRef Program = P.Shader->getBuffer();
    dispatch_data_t data = dispatch_data_create(Program.data(), Program.size(),
                                                dispatch_get_main_queue(),
                                                ^{
                                                });
    IS.Lib = Device->newLibrary(data, &Error);
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

    if (R.isRaw()) {
      MTL::Buffer *Buf = Device->newBuffer(R.BufferPtr->Data.get(), R.size(),
                                           MTL::ResourceStorageModeManaged);
      IRBufferView View = {};
      View.buffer = Buf;
      View.bufferSize = R.size();

      IRDescriptorTableSetBufferView(&TablePtr[HeapIdx], &View);
      IS.Buffers.push_back(Buf);
    } else {
      uint64_t Width = R.size() / R.getElementSize();
      MTL::TextureUsage UsageFlags = MTL::ResourceUsageRead;
      if (R.isReadWrite())
        UsageFlags |= MTL::ResourceUsageWrite;
      MTL::TextureDescriptor *Desc =
          MTL::TextureDescriptor::textureBufferDescriptor(
              getMTLFormat(R.BufferPtr->Format, R.BufferPtr->Channels), Width,
              MTL::ResourceStorageModeManaged, UsageFlags);

      MTL::Texture *NewTex = Device->newTexture(Desc);
      NewTex->replaceRegion(MTL::Region(0, 0, Width, 1), 0,
                            R.BufferPtr->Data.get(), 0);

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

    NS::UInteger TGS = IS.PipelineState->maxTotalThreadsPerThreadgroup();
    llvm::ArrayRef<int> DispatchSize =
        llvm::ArrayRef<int>(P.Shaders[0].DispatchSize);
    MTL::Size GridSize =
        MTL::Size(TGS * DispatchSize[0], DispatchSize[1], DispatchSize[2]);
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
        if (R.isReadWrite()) {
          if (R.isRaw()) {
            memcpy(R.BufferPtr->Data.get(),
                   IS.Buffers[BufferIndex++]->contents(), R.size());
          } else {
            uint64_t Width = R.size() / R.getElementSize();
            IS.Textures[TextureIndex++]->getBytes(
                R.BufferPtr->Data.get(), 0, MTL::Region(0, 0, Width, 1), 0);
          }
        } else {
          if (R.isRaw())
            ++BufferIndex;
          else
            ++TextureIndex;
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

llvm::Error InitializeMTLDevices() {
  return MTLContext::instance().initialize();
}
