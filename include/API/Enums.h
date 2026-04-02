
#ifndef OFFLOADTEST_API_ENUMS_H
#define OFFLOADTEST_API_ENUMS_H

namespace offloadtest {

enum class ResourceKind {
  Buffer,
  StructuredBuffer,
  ByteAddressBuffer,
  Texture2D,
  RWBuffer,
  RWStructuredBuffer,
  RWByteAddressBuffer,
  RWTexture2D,
  ConstantBuffer,
  Sampler,
  SampledTexture2D,
};

enum ShaderContainerType {
  DXIL,
  SPIRV,
  Metal,
};

} // namespace offloadtest

#endif // OFFLOADTEST_API_ENUMS_H
