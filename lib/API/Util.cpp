#include "API/Util.h"
#include "API/Texture.h"

llvm::Error offloadtest::findAndValidateRenderPassTextureSize(
    const RenderPassBeginDesc &Desc, uint32_t *OutWidth, uint32_t *OutHeight) {
  // Vulkan and Metal require us to pass a size for the render pass.
  // DX12 doesn't require us to pass the size, but those require all render
  // targets and depth stencil targets to have the same size.

  bool AnyTexture = false;
  uint32_t Width = 0, Height = 0;
  for (const auto &Attachment : Desc.ColorAttachments) {
    const TextureCreateDesc &TexDesc = Attachment->getDesc();

    if (!AnyTexture) {
      Width = TexDesc.Width;
      Height = TexDesc.Height;
      AnyTexture = true;
    } else {
      if (TexDesc.Width != Width || TexDesc.Height != Height)
        return llvm::createStringError(
            "All Render Targets and Depth Stencil Targets in a render pass "
            "must have the same size.");
    }
  }

  if (Desc.DepthStencil) {
    const TextureCreateDesc &TexDesc = Desc.DepthStencil->getDesc();

    if (!AnyTexture) {
      Width = TexDesc.Width;
      Height = TexDesc.Height;
      AnyTexture = true;
    } else {
      if (TexDesc.Width != Width || TexDesc.Height != Height)
        return llvm::createStringError(
            "All Render Targets and Depth Stencil Targets in a render pass "
            "must have the same size.");
    }
  }

  if (OutWidth)
    *OutWidth = Width;
  if (OutHeight)
    *OutHeight = Height;

  return llvm::Error::success();
}

offloadtest::IntelGpuEra offloadtest::getIntelGpuEra(uint16_t DeviceId) {
  const uint16_t FamilyPrefix = DeviceId & 0xFF00;
  switch (FamilyPrefix) {
  case 0x5900: // Kaby Lake (7th Gen)
  case 0x3E00: // Coffee Lake / Whiskey Lake (8th/9th Gen)
  case 0x9B00: // Comet Lake (10th Gen)
  case 0x8A00: // Ice Lake (10th Gen)
    return IntelGpuEra::Gen7_to_10;
  case 0x9A00: // Tiger Lake (11th Gen)
  case 0x4C00: // Rocket Lake (11th Gen Desktop)
  case 0x4600: // Alder Lake (12th Gen)
  case 0xA700: // Raptor Lake (13th Gen)
  case 0x7D00: // Meteor Lake (14th Gen Core Ultra)
  case 0x5600: // Arc Alchemist (Discrete Xe-HPG)
  case 0x4F00: // DG1 (Discrete Xe-LP)
  case 0x0B00: // Ponte Vecchio (Xe-HPC / Data Center Max)
  case 0xE200: // Battlemage Discrete
  case 0x6400: // Lunar Lake Integrated
    return IntelGpuEra::Gen11_to_14_and_Xe;
  default:
    return IntelGpuEra::UnknownOrLegacy;
  }
}
