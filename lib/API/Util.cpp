#include "Util.h"

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
