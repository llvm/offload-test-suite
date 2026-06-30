#include "API/DX/Buffer.h"
#include "Support/WinError.h"

using namespace offloadtest;

DXBuffer::DXBuffer(ComPtr<ID3D12Resource> Buffer, llvm::StringRef Name,
                   BufferCreateDesc Desc, size_t SizeInBytes,
                   uint64_t CounterOffsetInBytes,
                   D3D12_RESOURCE_STATES PreferredState,
                   D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,
                   D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle,
                   D3D12_CPU_DESCRIPTOR_HANDLE CBVHandle)
    : offloadtest::Buffer(GPUAPI::DirectX), Buffer(Buffer), Name(Name),
      Desc(Desc), SizeInBytes(SizeInBytes),
      CounterOffsetInBytes(CounterOffsetInBytes),
      PreferredState(PreferredState), SRVHandle(SRVHandle),
      UAVHandle(UAVHandle), CBVHandle(CBVHandle) {}

size_t DXBuffer::getSizeInBytes() const { return SizeInBytes; }

size_t DXBuffer::querySparseTileSizeInBytes(const Device & /*Dev*/) const {
  return D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
}

llvm::Expected<void *> DXBuffer::map() {
  if (Desc.Location == MemoryLocation::GpuOnly)
    return llvm::createStringError(std::errc::invalid_argument,
                                   "Cannot map a GpuOnly buffer.");
  void *Ptr = nullptr;
  if (auto Err =
          HR::toError(Buffer->Map(0, nullptr, &Ptr), "Failed to map buffer."))
    return std::move(Err);
  return Ptr;
}

void DXBuffer::unmap() { Buffer->Unmap(0, nullptr); }

const BufferCreateDesc &DXBuffer::getDesc() const { return Desc; }

bool DXBuffer::classof(const offloadtest::Buffer *B) {
  return B->getAPI() == GPUAPI::DirectX;
}