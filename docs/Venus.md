# Enabling GPU API virtualization on UTM VMs with macOS hosts 

> Note: These docs are for mac users wanting on device linux development or a frictionless way to try out Venus.
>

## 1. What is UTM?

UTM is a macOS app that runs virtual machines using Apple’s Hypervisor framework
and QEMU, with support for both native (Apple Silicon) and emulated architectures.

### Hypervisor.framework

QEMU includes support for the `hvf` accelerator which provides same architecture 
virtualization (x86 -> x86 or ARM64 -> ARM64) on macOS.

### UTM's GPU Acceleration

The graphics architecture of UTM involves many separate translation layers.

```
                             ┌────────────────────────────────────────────────┐
                             │  Host                                          │
                             │ ┌────────────────────────────────┬───────────┐ │
                          ┌──┼─► virglrenderer       │ +Venus†  │ gfxstream†│ │
                          │  │ │                     │          │           │ │
                          │  │ ├──────────┬──────────┬──────────┴───────────┤ │
┌─────────────────────┐   │  │ │ ANGLE    │ ANGLE    │ MoltenVK†            │ │
│ Guest               │   Q  │ │ Metal    │ OpenGL   │                      │ │
│ ┌─────────────────┐ │   E  │ ├──────────┴──────────┴──────────────────────┤ │
│ │ Userland 3D API │ │   M  │ │ CocoaSpice Metal Renderer                  │ │
│ │ (e.g. Mesa)     │ │   U  │ │                                            │ │
│ ├─────────────────┤ │   │  │ ├────────────────────────────────────────────┤ │
│ │ Kernel Driver   │ │   │  │ │ Metal Device                               │ │
│ │ (virtio-gpu)    ├─┼───┘  │ │                                            │ │
│ └─────────────────┘ │      │ └────────────────────────────────────────────┘ │
│                     │      │                                                │
└─────────────────────┘      └────────────────────────────────────────────────┘
```
### Venus

The important one for the offload test suite is the [virtio-gpu Venus Mesa Driver](https://docs.mesa3d.org/drivers/venus.html)
Venus lets a VM use Vulkan by forwarding Vulkan API calls to the host GPU via
virtio-gpu. In other words the guest does not need a native Vulkan driver for
the physical GPU. Instead of GPU Pass through its like command-stream forwarding.
 
 ---

## 2. Installing UTM & Linux Setup

To use Venus you need UTM v5.0.0 or newer. This version is not currently
availalable on the Mac App Store and can only be downloaded from
 [UTM's github](https://github.com/utmapp/UTM/releases).

### Step 1: Download Linux
Choose a Linux version and architecture that matches your Mac. These docs used [**ARM64 Ubuntu 24.04 LTS**](https://cdimage.ubuntu.com/noble/daily-live/pending/) Since its a relible VM iso for Apple silicon based macs.

### Step 2: Setup UTM

#### Storage requirments
Between all the dependencies you will need to install and the build directories for llvm and dxc plan to devote at least 100GB to this vm. Don't rely on expanding the drive later. That doesn't work well.

#### Memory and CPU requirements
For best results plan to devote atleast 4 cores and 14GB of ram to your vm so you can build llvm smoothly.


### Step 3: Create a New VM in UTM

1. Open **UTM**
2. Click **Create**
3. Select **Virtualize**

### Step 4: Boot the Ubuntu ISO

1. Choose **Boot ISO Image**
2. Select the downloaded Ubuntu `.iso`
3. Click **Continue**
4. Run the Ubuntu installer

### Step 5: Install software
- You will need to install [LLVM's software dependencies](https://llvm.org/docs/GettingStarted.html#software).
- You will need to git clone:
  - [llvm/offload-test-suite](https://github.com/llvm/offload-test-suite)
  - [llvm/llvm-project](https://github.com/llvm/lvm-project)
  - [microsoft/DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler)
- follow [adding-to-llvm-build](https://github.com/llvm/offload-test-suite?tab=readme-ov-file#adding-to-llvm-build) setup for enableing offload
- In the Ubuntu VM run `sudo apt install -y mesa-vulkan-drivers`
- Optionally run `sudo apt install -y vulkan-tools` to install `vulkaninfo`. 

### Step 6: Test for Venus
```
vulkaninfo --summary | grep driverName
```
You should see:
```
	driverName         = venus
	driverName         = llvmpipe
```

If you don't have vulkaninfo. You can test via `api-query`
after building the offload test suite like so:
```bash
<llvm_build_dir>/bin/api-query | grep "Description"
  Description: Virtio-GPU Venus (Apple M5)
  Description: llvmpipe (LLVM 20.1.2, 128 bits)
```

---

## 3. Running the Offload Test Suite with Venus

To run the LLVM offload test suite against Venus, set the target GPU name explicitly:

```bash
OFFLOADTEST_GPU_NAME=Venus ninja -C <offload_build_dir> check-hlsl-clang-vk
OFFLOADTEST_GPU_NAME=Venus ninja -C <offload_build_dir> check-hlsl-vk
```

This forces the test harness to select the Venus Vulkan device instead of the software render `llvmpipe`.
