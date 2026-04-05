#define IR_RUNTIME_METALCPP
#include "MTLDescriptorHeap.h"
#include "metal_irconverter_runtime.h"

using namespace offloadtest;

METAL_GPU_DESCRIPTOR_HANDLE &
METAL_GPU_DESCRIPTOR_HANDLE::Offset(int32_t OffsetInDescriptors) {
  ptr = MTL::GPUAddress(int64_t(ptr) + int64_t(OffsetInDescriptors) *
                                           sizeof(IRDescriptorTableEntry));
  return *this;
}

llvm::Expected<std::unique_ptr<MTLDescriptorHeap>>
MTLDescriptorHeap::create(MTL::Device *Device,
                          const METAL_DESCRIPTOR_HEAP_DESC &Desc) {
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

METAL_GPU_DESCRIPTOR_HANDLE
MTLDescriptorHeap::getGPUDescriptorHandleForHeapStart() const {
  return METAL_GPU_DESCRIPTOR_HANDLE{Buffer->gpuAddress()};
}

IRDescriptorTableEntry *
MTLDescriptorHeap::getEntryHandle(uint32_t Index) const {
  assert(Index < Desc.NumDescriptors && "Descriptor index out of bounds.");
  return static_cast<IRDescriptorTableEntry *>(Buffer->contents()) + Index;
}

void MTLDescriptorHeap::bind(MTL::RenderCommandEncoder *Encoder) {
  Encoder->useResource(Buffer, MTL::ResourceUsageRead);
  // Dynamic resource indexing
  const NS::UInteger BindPoint =
      Desc.Type == METAL_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
          ? kIRDescriptorHeapBindPoint
      : Desc.Type == METAL_DESCRIPTOR_HEAP_TYPE_SAMPLER
          ? kIRSamplerHeapBindPoint
          : 0; // Should not happen.
  Encoder->setVertexBuffer(Buffer, 0, BindPoint);
  Encoder->setFragmentBuffer(Buffer, 0, BindPoint);
}

void MTLDescriptorHeap::bind(MTL::ComputeCommandEncoder *Encoder) {
  Encoder->useResource(Buffer, MTL::ResourceUsageRead);
  // Dynamic resource indexing
  const NS::UInteger BindPoint =
      Desc.Type == METAL_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
          ? kIRDescriptorHeapBindPoint
      : Desc.Type == METAL_DESCRIPTOR_HEAP_TYPE_SAMPLER
          ? kIRSamplerHeapBindPoint
          : 0; // Should not happen.
  Encoder->setBuffer(Buffer, 0, BindPoint);
}
