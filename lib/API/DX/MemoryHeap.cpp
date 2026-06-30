#include <d3dx12.h>

#include "API/DX/MemoryHeap.h"
#include "Support/WinError.h"

using namespace offloadtest;

DXMemoryHeap::DXMemoryHeap(ComPtr<ID3D12Heap> Heap, llvm::StringRef Name)
    : offloadtest::MemoryHeap(GPUAPI::DirectX), Name(Name), Heap(Heap) {}

bool DXMemoryHeap::classof(const offloadtest::MemoryHeap *H) {
  return H->getAPI() == GPUAPI::DirectX;
}

llvm::Expected<std::unique_ptr<DXMemoryHeap>>
DXMemoryHeap::create(ID3D12DeviceX *Device, llvm::StringRef Name,
                     size_t SizeInBytes) {
  D3D12_HEAP_DESC HeapDesc = {};
  HeapDesc.Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  HeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  // Heap sizes must be a multiple of the placement alignment (64KB), which
  // is also the tiled-resource tile size.
  HeapDesc.SizeInBytes =
      llvm::alignTo(SizeInBytes, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
  HeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;

  ComPtr<ID3D12Heap> Heap;
  if (auto Err = HR::toError(Device->CreateHeap(&HeapDesc, IID_PPV_ARGS(&Heap)),
                             "Failed to create memory heap."))
    return Err;

  const std::wstring WStr(Name.begin(), Name.end());
  Heap->SetName(WStr.c_str());

  return std::make_unique<DXMemoryHeap>(Heap, Name);
}
