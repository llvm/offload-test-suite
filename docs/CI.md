# Continuous Integration

## Build Support Tiers

### Tier 1 Configurations

Tier 1 configurations are regularly tested and expected to be known good.
Builders of these configurations are enabled in pre-merge testing and PRs should
not regress these configurations.

When these configurations fail it should be high priority to correct them.

Configurations:
* Windows Intel GPU DirectX
* Windows Intel GPU Vulkan
* Windows NVIDIA GPU DirectX
* Windows WARP LKG x64
* Windows WARP LKG arm64

### Tier 2 Configurations

Tier 2 configurations are are regularly tested and expected to be known good.
Builders of these configurations are enabled in pre-merge testing and PRs should
avoid regressing these configurations.

Because these configurations are less stable and rely more heavily on tooling
outside our direct control PRs may liberally `XFAIL` new tests that only fail on
Tier 2 configurations.

Configurations:
* macOS Metal

### Experimental Configurations

Experimental configurations are testing configurations that have been
recently added and are not yet completely passing. These configurations are not
included in pre-merge testing unless a PR has the `test-all` label applied to
it.

Experimental configurations should be promoted out to other tiers once they are
robustly passing.

Configurations:
* Windows AMD GPU DirectX
* Windows AMD GPU Vulkan
* Windows NVIDIA GPU Vulkan
* Windows QC GPU DirectX
* Windows QC GPU Vulkan

## Build Hardware

### Apple M4 Pro

* CPU: Apple M4 Pro
* GPU: Apple M4 Pro
* RAM: 24 GiB
* Configurations:
  * macOS Metal

### Windows AMD GPU

* CPU: AMD Ryzen 7 9700X
* GPU: AMD Radeon RX 9070
* RAM: 32 GiB
* Configurations:
  * Windows AMD GPU DirectX
  * Windows AMD GPU Vulkan

### Windows Intel GPU

* CPU: AMD Ryzen Threadripper 3970X
* GPU: Intel Arc Pro B50 Graphics
* RAM: 64 GiB
* Configurations:
  * Windows Intel GPU DirectX
  * Windows Intel GPU Vulkan
  * Windows WARP LKG x64

### Windows NVIDIA GPU

* CPU: Intel(R) Core(TM) i5-14400F
* GPU: NVIDIA GeForce RTX 5070
* RAM: 16 GiB
* Configurations:
  * Windows NVIDIA GPU DirectX
  * Windows NVIDIA GPU Vulkan

### Windows Qualcomm GPU

* CPU: Qualcomm Snapdragon X Plus - X1P-64-100
* GPU: Qualcomm Adreno X1-85
* RAM: 32 GiB
* Configurations:
  * Windows QC GPU DirectX
  * Windows QC GPU Vulkan
  * Windows WARP LKG arm64