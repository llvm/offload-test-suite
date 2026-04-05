#define IR_RUNTIME_METALCPP
#include "MTLTopLevelArgumentBuffer.h"
#include "metal_irconverter_runtime.h"

using namespace offloadtest;

static llvm::StringRef getResourceTypeString(IRResourceType Type) {
  switch (Type) {
  case IRResourceTypeCBV:
    return "CBV";
  case IRResourceTypeSRV:
    return "SRV";
  case IRResourceTypeUAV:
    return "UAV";
  case IRResourceTypeTable:
    return "Table";
  default:
    return "Unknown";
  }
}

llvm::Expected<std::unique_ptr<MTLTopLevelArgumentBuffer>>
MTLTopLevelArgumentBuffer::create(MTL::Device *Device,
                                  IRRootSignature *RootSig) {
  if (!Device)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Invalid MTL::Device pointer.");

  if (!RootSig)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Invalid IRRootSignature pointer.");

  std::vector<IRResourceLocation> ResourceLocs(
      IRRootSignatureGetResourceCount(RootSig));
  // Empty root signature is valid, bind methods will be no-ops
  if (ResourceLocs.empty()) {
    return std::make_unique<MTLTopLevelArgumentBuffer>(ResourceLocs, nullptr);
  }

  IRRootSignatureGetResourceLocations(RootSig, ResourceLocs.data());

  size_t BufferSize = 0;
  for (size_t ResourceIdx = 0; ResourceIdx < ResourceLocs.size();
       ++ResourceIdx) {
    const IRResourceLocation &Loc = ResourceLocs[ResourceIdx];
    BufferSize += Loc.sizeBytes;

    llvm::outs() << "Resource[" << ResourceIdx << "] {"
                 << " Type=" << getResourceTypeString(Loc.resourceType) << ","
                 << " Space=" << Loc.space << ","
                 << " Slot=" << Loc.slot << ","
                 << " TopLevelOffset=" << Loc.topLevelOffset << ","
                 << " SizeInBytes=" << Loc.sizeBytes << " }\n";
  }
  MTL::Buffer *Buffer =
      Device->newBuffer(BufferSize, MTL::ResourceStorageModeShared);
  if (!Buffer)
    return llvm::createStringError(
        std::errc::not_enough_memory,
        "Failed to create top-level argument buffer.");
  return std::make_unique<MTLTopLevelArgumentBuffer>(std::move(ResourceLocs),
                                                     Buffer);
}

MTLTopLevelArgumentBuffer::~MTLTopLevelArgumentBuffer() {
  if (Buffer)
    Buffer->release();
}

bool MTLTopLevelArgumentBuffer::checkIndex(uint32_t Index) const {
  if (Index >= ResourceLocs.size()) {
    llvm::errs() << "Invalid index " << Index << ", only "
                 << ResourceLocs.size()
                 << " resources in root "
                    "signature.\n";
    return false;
  }
  return true;
}

bool MTLTopLevelArgumentBuffer::checkResourceType(
    uint32_t Index, IRResourceType ExpectedType) const {
  const IRResourceLocation &Loc = ResourceLocs[Index];
  if (Loc.resourceType != ExpectedType) {
    llvm::errs() << "Resource type mismatch for index " << Index
                 << ", expected " << static_cast<uint32_t>(ExpectedType)
                 << " but root signature specifies "
                 << static_cast<uint32_t>(Loc.resourceType) << ".\n";
    return false;
  }
  return true;
}

bool MTLTopLevelArgumentBuffer::checkResourceSize(uint32_t Index,
                                                  size_t ExpectedSize) const {
  const IRResourceLocation &Loc = ResourceLocs[Index];
  if (Loc.sizeBytes != ExpectedSize) {
    llvm::errs() << "Size mismatch for index " << Index << ", expected "
                 << ExpectedSize << " but root signature specifies "
                 << Loc.sizeBytes << ".\n";
    return false;
  }
  return true;
}

template <IRResourceType ResourceType, typename T>
void MTLTopLevelArgumentBuffer::setResource(uint32_t Index, T Resource) const {
  if (!Buffer || !checkIndex(Index) ||
      !checkResourceType(Index, ResourceType) ||
      !checkResourceSize(Index, sizeof(T)))
    return;

  const IRResourceLocation &Loc = ResourceLocs[Index];
  std::byte *Dst =
      static_cast<std::byte *>(Buffer->contents()) + Loc.topLevelOffset;
  memcpy(Dst, &Resource, sizeof(T));
}

void MTLTopLevelArgumentBuffer::setRoot32BitConstant(
    uint32_t Index, uint32_t SrcData, uint32_t DestOffsetIn32BitValues) const {
  setRoot32BitConstants(Index, 1, &SrcData, DestOffsetIn32BitValues);
}

void MTLTopLevelArgumentBuffer::setRoot32BitConstants(
    uint32_t Index, uint32_t Num32BitValuesToSet, const void *pSrcData,
    uint32_t DestOffsetIn32BitValues) const {
  if (!Buffer || !checkIndex(Index) ||
      !checkResourceType(Index, IRResourceTypeConstant))
    return;

  const IRResourceLocation &Loc = ResourceLocs[Index];
  if ((DestOffsetIn32BitValues + Num32BitValuesToSet) * sizeof(uint32_t) >
      Loc.sizeBytes) {
    llvm::errs() << "Size mismatch for index " << Index << ", root signature "
                 << "specifies " << Loc.sizeBytes << " bytes but trying to set "
                 << (DestOffsetIn32BitValues + Num32BitValuesToSet) *
                        sizeof(uint32_t)
                 << " bytes.\n";
    return;
  }

  std::byte *Dst = static_cast<std::byte *>(Buffer->contents()) +
                   Loc.topLevelOffset +
                   DestOffsetIn32BitValues * sizeof(uint32_t);
  memcpy(Dst, pSrcData, Num32BitValuesToSet * sizeof(uint32_t));
}

void MTLTopLevelArgumentBuffer::setRootConstantBufferView(
    uint32_t Index, uint64_t GPUAddr) const {
  setResource<IRResourceTypeCBV, uint64_t>(Index, GPUAddr);
}

void MTLTopLevelArgumentBuffer::setRootShaderResourceView(
    uint32_t Index, uint64_t GPUAddr) const {
  setResource<IRResourceTypeSRV, uint64_t>(Index, GPUAddr);
}

void MTLTopLevelArgumentBuffer::setRootUnorderedAccessView(
    uint32_t Index, uint64_t GPUAddr) const {
  setResource<IRResourceTypeUAV, uint64_t>(Index, GPUAddr);
}

void MTLTopLevelArgumentBuffer::setRootDescriptorTable(
    uint32_t Index, METAL_GPU_DESCRIPTOR_HANDLE BaseHandle) const {
  setResource<IRResourceTypeTable, METAL_GPU_DESCRIPTOR_HANDLE>(Index,
                                                                BaseHandle);
}

void MTLTopLevelArgumentBuffer::bind(MTL::RenderCommandEncoder *Encoder) const {
  if (!Buffer)
    return;

  Encoder->useResource(Buffer, MTL::ResourceUsageRead);
  Encoder->setVertexBuffer(Buffer, 0, kIRArgumentBufferBindPoint);
  Encoder->setFragmentBuffer(Buffer, 0, kIRArgumentBufferBindPoint);
}

void MTLTopLevelArgumentBuffer::bind(
    MTL::ComputeCommandEncoder *Encoder) const {
  if (!Buffer)
    return;

  Encoder->useResource(Buffer, MTL::ResourceUsageRead);
  Encoder->setBuffer(Buffer, 0, kIRArgumentBufferBindPoint);
}
