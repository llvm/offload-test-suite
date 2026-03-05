# Enabling GPU API virtualization on UTM VMs with macOS hosts 

> Note: These docs are for mac users wanting on device linux development or a
> frictionless way to try out Venus.

## 1. What is UTM?

**UTM** is a macOS application for running virtual machines using Apple’s
**Hypervisor.framework** and **QEMU**. It supports both:

- **Native virtualization** (Apple Silicon → ARM64 guests)
- **Emulated architectures** (e.g. x86_64 on ARM, with reduced performance)

### Hypervisor.framework

QEMU supports the `hvf` accelerator, which enables 
**same-architecture virtualization** on macOS:
- ARM64 → ARM64 (Apple Silicon)
- x86_64 → x86_64 (Intel Macs)

This provides near-native CPU performance for supported guests

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

The important one for the offload test suite is the 
[virtio-gpu Venus Mesa Driver](https://docs.mesa3d.org/drivers/venus.html)
Venus lets a VM use Vulkan by forwarding Vulkan API calls to the host GPU via
virtio-gpu. In other words the guest does not need a native Vulkan driver for
the physical GPU. Instead of GPU Pass through its like command-stream forwarding.
 
 ---

## 2. Installing UTM & Linux Setup

To use Venus you need **UTM v5.0.0 or newer**. 
> ⚠️ This version is not currently availalable on the Mac App Store and can
only be downloaded from [UTM's github](https://github.com/utmapp/UTM/releases).

### Step 1: Download Linux
Choose a Linux distribution and architecture that **matches your Mac's CPU**.

These instructions assume:

- **Ubuntu 24.04 LTS (ARM64)**
- Apple Silicon host

There is no offical 24.04 ARM64 ISO but you You can find daily ISOs here:

- https://cdimage.ubuntu.com/noble/daily-live/pending/


### Step 2: Setup UTM

#### Storage Requirements

Allocate **at least 100 GB** of disk space for:

- LLVM build trees
- DirectXShaderCompiler builds
- Mesa and Vulkan tooling
- LLVM and DXC software requirments
  - (complers, linkers, build systems, etc)
- Offload Test Suite artifacts

> ⚠️ Do **not** rely on expanding the disk later—UTM disk resizing is
unreliable.

#### CPU and Memory

Recommended minimums:

- **4 CPU cores**
- **14 GB RAM**

This avoids excessive LLVM build times and out-of-memory failures.

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

To run the LLVM offload test suite against Venus, set the target GPU name
explicitly:

```bash
OFFLOADTEST_GPU_NAME=Venus ninja -C <offload_build_dir> check-hlsl-clang-vk
OFFLOADTEST_GPU_NAME=Venus ninja -C <offload_build_dir> check-hlsl-vk
```

This forces the test harness to select the Venus Vulkan device instead of the
software render `llvmpipe`.
