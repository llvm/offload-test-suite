# Enabling Vulkan on macOS with KosmicKrisp

> Note: KosmicKrisp is implemented on top of Metal 4 which requires macOS 26
> and newer. These docs were last updated for sdk versions `1.4.335.1`.

Just like with MoltenVK To enable Vulkan testing on macOS download and install the
Vulkan SDK from [here](https://vulkan.lunarg.com). Make sure KosmicKrisp is 
selected in the installer.![vulkan sdk installer](pics/KosmicKrisp_installer.png)
By default This will install the Vulkan SDK at 
/Users/<username>/VulkanSDK/<sdk_version>/. After install run `vulkaninfo --summary`.
You should notice that only one Vulkan GPU target is availalble and it is MoltenVK.
```
Devices:
========
GPU0:
        apiVersion         = 1.4.323
        driverVersion      = 0.2.2208
        vendorID           = 0x106b
        deviceID           = 0x1a020209
        deviceType         = PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        deviceName         = Apple M5
        driverID           = DRIVER_ID_MOLTENVK
        driverName         = MoltenVK
        driverInfo         = 1.4.0
        conformanceVersion = 1.4.2.0
        deviceUUID         = 0000106b-1a02-0209-0000-000000000000
        driverUUID         = 4d564b00-0000-28a0-1a02-020900000000
```
If you scroll up you should see the following error message at the top.
```
ERROR: [Loader Message] Code 0 : dlopen(/usr/local/share/vulkan/icd.d/../../../lib/libvulkan_kosmickrisp.dylib, 0x0005): tried: '/usr/local/share/vulkan/icd.d/../../../lib/libvulkan_kosmickrisp.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/usr/local/share/vulkan/icd.d/../../../lib/libvulkan_kosmickrisp.dylib' (no such file), '/usr/local/share/vulkan/icd.d/../../../lib/libvulkan_kosmickrisp.dylib' (no such file), '/usr/local/lib/libvulkan_kosmickrisp.dylib' (no such file), '/System/Volumes/Preboot/Cryptexes/OS/usr/local/lib/
ERROR: [Loader Message] Code 0 : loader_icd_scan: Failed loading library associated with ICD JSON /usr/local/share/vulkan/icd.d/../../../lib/libvulkan_kosmickrisp.dylib. Ignoring this JSON
```
This is because `/usr/local/share/vulkan/icd.d/libkosmickrisp_icd.json` has set
the library path to `"../../../lib/libvulkan_kosmickrisp.dylib"` but there is no
`libvulkan_kosmickrisp.dylib` in `/usr/local/lib/`. The dylib is in 
`/Users/<username>/VulkanSDK/<sdk_version>/macOS/lib/`. You can fix this error
by either editing `libkosmickrisp_icd.json` to point to the `VulkanSDK` dylib
or by creating a symlink like so.

```bash
sudo ln -s \\
  /Users/<username>/VulkanSDK/<sdk_version>/macOS/lib/libvulkan_kosmickrisp.dylib \\
  /usr/local/lib/libvulkan_kosmickrisp.dylib
```

After doing all this you will still only see one GPU when running `vulkaninfo --summary`.
you now need to run `vkconfig-gui` and switch from `Force a single Vulkan physical device`
to `Order Vulkan physical devices`. ![vkconfig-gui settings](pics/vkconfig-gui-settings.png)

Now when you run `vulkaninfo --summary` you should see two GPUs
```
Devices:
========
GPU0:
        apiVersion         = 1.4.323
        driverVersion      = 0.2.2208
        vendorID           = 0x106b
        deviceID           = 0x1a020209
        deviceType         = PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        deviceName         = Apple M5
        driverID           = DRIVER_ID_MOLTENVK
        driverName         = MoltenVK
        driverInfo         = 1.4.0
        conformanceVersion = 1.4.2.0
        deviceUUID         = 0000106b-1a02-0209-0000-000000000000
        driverUUID         = 4d564b00-0000-28a0-1a02-020900000000
GPU1:
        apiVersion         = 1.3.335
        driverVersion      = 25.99.99
        vendorID           = 0x106b
        deviceID           = 0x0064
        deviceType         = PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU
        deviceName         = Apple M5
        driverID           = DRIVER_ID_MESA_KOSMICKRISP
        driverName         = KosmicKrisp
        driverInfo         = sdk-1.4.335
        conformanceVersion = 1.4.3.2
        deviceUUID         = 8f050000-0100-0000-0000-000000000000
        driverUUID         = e2195912-1e0a-37cf-a513-f0753b130518
```
Now we can run the offload test suite against KosmicKrisp like so
```bash
OFFLOADTEST_GPU_NAME=KosmicKrisp ninja -C <offload_build_dir> check-hlsl-clang-vk
OFFLOADTEST_GPU_NAME=KosmicKrisp ninja -C <offload_build_dir> check-hlsl-vk 
```
