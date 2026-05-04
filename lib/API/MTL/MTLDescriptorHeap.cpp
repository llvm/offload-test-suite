#include "MTLDescriptorHeap.h"
#include "MetalIRConverter.h"

using namespace offloadtest;

static NS::UInteger getDescriptorHeapBindPoint(MTLDescriptorHeapType Type) {
  switch (Type) {
  case MTLDescriptorHeapType::CBV_SRV_UAV:
    return kIRDescriptorHeapBindPoint;
  case MTLDescriptorHeapType::Sampler:
    return kIRSamplerHeapBindPoint;
  }
  llvm_unreachable("All cases handled.");
}

MTLGPUDescriptorHandle &
MTLGPUDescriptorHandle::Offset(int32_t OffsetInDescriptors) {
  Ptr = MTL::GPUAddress(int64_t(Ptr) + int64_t(OffsetInDescriptors) *
                                           sizeof(IRDescriptorTableEntry));
  return *this;
}

llvm::Expected<std::unique_ptr<MTLDescriptorHeap>>
MTLDescriptorHeap::create(MTL::Device *Device,
                          const MTLDescriptorHeapDesc &Desc) {
  if (!Device)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Invalid MTL::Device pointer.");

  if (Desc.NumDescriptors == 0)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Invalid descriptor heap description.");

  MTL::Buffer *Buf =
      Device->newBuffer(Desc.NumDescriptors * sizeof(IRDescriptorTableEntry),
                        MTL::ResourceStorageModeShared);
  if (!Buf)
    return llvm::createStringError(std::errc::not_enough_memory,
                                   "Failed to create MTLDescriptorHeap.");
  return std::make_unique<MTLDescriptorHeap>(Desc, Buf);
}

MTLDescriptorHeap::~MTLDescriptorHeap() {
  if (Buffer)
    Buffer->release();
}

MTLGPUDescriptorHandle
MTLDescriptorHeap::getGPUDescriptorHandleForHeapStart() const {
  return MTLGPUDescriptorHandle{Buffer->gpuAddress()};
}

IRDescriptorTableEntry *
MTLDescriptorHeap::getEntryHandle(uint32_t Index) const {
  assert(Index < Desc.NumDescriptors && "Descriptor index out of bounds.");
  return static_cast<IRDescriptorTableEntry *>(Buffer->contents()) + Index;
}

void MTLDescriptorHeap::bind(MTL::RenderCommandEncoder *Encoder) {
  Encoder->useResource(Buffer, MTL::ResourceUsageRead);
  // Dynamic resource indexing
  const NS::UInteger BindPoint = getDescriptorHeapBindPoint(Desc.Type);
  Encoder->setVertexBuffer(Buffer, 0, BindPoint);
  Encoder->setFragmentBuffer(Buffer, 0, BindPoint);
}

void MTLDescriptorHeap::bind(MTL::ComputeCommandEncoder *Encoder) {
  Encoder->useResource(Buffer, MTL::ResourceUsageRead);
  // Dynamic resource indexing
  const NS::UInteger BindPoint = getDescriptorHeapBindPoint(Desc.Type);
  Encoder->setBuffer(Buffer, 0, BindPoint);
}
