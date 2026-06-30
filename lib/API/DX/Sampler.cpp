#include "API/DX/Sampler.h"

using namespace offloadtest;

DXSampler::DXSampler(llvm::StringRef Name, SamplerCreateDesc Desc,
                     D3D12_CPU_DESCRIPTOR_HANDLE Handle)
    : offloadtest::Sampler(GPUAPI::DirectX), Handle(Handle), Name(Name),
      Desc(Desc) {}

const SamplerCreateDesc &DXSampler::getDesc() const { return Desc; }

bool DXSampler::classof(const offloadtest::Sampler *S) {
  return S->getAPI() == GPUAPI::DirectX;
}
