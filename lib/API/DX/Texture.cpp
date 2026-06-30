#include "API/DX/Texture.h"
#include "API/DX/Device.h"
#include "Support/WinError.h"

using namespace offloadtest;

DXTexture::DXTexture(ComPtr<ID3D12Resource> Resource, llvm::StringRef Name,
                     TextureCreateDesc Desc,
                     D3D12_RESOURCE_STATES PreferredState,
                     D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle,
                     D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle)
    : offloadtest::Texture(GPUAPI::DirectX), Resource(Resource),
      PreferredState(PreferredState), SRVHandle(SRVHandle),
      UAVHandle(UAVHandle), Name(Name), Desc(Desc) {}

TileShape DXTexture::querySparseTileShape(const Device &Dev) const {
  const DXDevice &DeviceDX = llvm::cast<DXDevice>(Dev);

  D3D12_TILE_SHAPE Shape = {};
  UINT NumTiles = 0;
  UINT NumSubresourceTilings = 0;
  DeviceDX.Device->GetResourceTiling(Resource.Get(), &NumTiles, nullptr, &Shape,
                                     &NumSubresourceTilings, 0, nullptr);
  return TileShape{Shape.WidthInTexels, Shape.HeightInTexels,
                   Shape.DepthInTexels};
}

const TextureCreateDesc &DXTexture::getDesc() const { return Desc; }

bool DXTexture::classof(const offloadtest::Texture *T) {
  return T->getAPI() == GPUAPI::DirectX;
}
