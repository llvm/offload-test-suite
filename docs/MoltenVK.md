# Enabling Vulkan on macOS with MoltenVK

> Note: MoltenVK uses SPIRV-Cross to convert SPIRV shaders to the Metal shading
> language. This increases the mismatch between shader behavior, and as a result
> many of the tests in this suite have known failures or behavior differences.

To enable Vulkan testing on macOS you first need to download and install the
Vulkan SDK from [here](https://vulkan.lunarg.com).

By default the SDK installs into your home directory under
`~/VulkanSDK/${SDK_Version}/macOS`. For CMake to find the SDK you either need to
set the `VULKAN_SDK` environment variable to the macOS subdirectory of the
VulkanSDK installation you wish to use. You also need to run `sudo
~/VulkanSDK/${SDK_Version}/install_vulkan.py --force-install`, to install the
development binaries into `/usr/local/...` so that launched applications can
find them.

Once the SDK is installed and exposed to CMake, a clean configuration will
detect Vulkan and the MoltenVK portability layer and enable the Vulkan test
configurations.
