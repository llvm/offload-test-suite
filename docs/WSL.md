# Using OffloadTest in the Windows Subsystem for Linux (WSL)

Since [2020](https://devblogs.microsoft.com/directx/directx-heart-linux/), WSL 2 has had support for DirectX.
Specifically, the DxCore (`libdxcore.so`) and D3D12 (`libd3d12.so`) APIs have been exposed via closed-source, pre-compiled user-mode binaries shipped as part of Windows and made available to glibc-based distros within WSL 2 under `/usr/lib/wsl/lib`.
These binaries, in addition to the WSL-specific headers and libraries made available from the [DirectX-Headers](https://github.com/microsoft/DirectX-Headers) repository, makes building and running the experimental runtime test suite for HLSL within WSL 2 feasible.

## Requirements and Setup

1. Have an installation of Windows Subsystem for Linux. See [here](https://learn.microsoft.com/en-us/windows/wsl/install) for instructions and requirements. The rest of this setup assumes you are working within a WSL 2 Linux distribution
1. Install the DirectX-Headers package (usually named `directx-headers-dev`) from your preferred package manager.
This is to obtain the libraries: DirectX-Guids (`libDirectX-Guids.a`) and d3dx12-format-properties (`libd3dx12-format-properties.a`)
Alternatively, build and install the DirectX-Headers from the [original repo](https://github.com/microsoft/DirectX-Headers) or the git submodule of this repository: `third-party/DirectX-Headers`
1. Install the rest of the [prerequisites](https://github.com/llvm-beanz/offload-test-suite/tree/main?tab=readme-ov-file#prerequisites) and follow the [instructions](https://github.com/llvm-beanz/offload-test-suite/tree/main?tab=readme-ov-file#adding-to-llvm-build) to add the experimental runtime test suite for HLSL to an LLVM build

## Known Issues

- The D3D12 Debug Layer does not appear to work within WSL. (`D3D12GetDebugInterface` causes a segfault when called.)
  - Good luck debugging!
- PSOs can fail to be created due to unsigned shaders
  - Fix: Use a newer version of DXC that includes the validator binary `dxv`
- `double free or corruption (!prev)` may occur when `api-query` and `offloader` exit, which then follows with an infinite stall instead of program termination
  - A workaround is implemented which avoids this issue on normal program exit (i.e., `return 0;` from main)
  - This issue appears to occur whenever `DeviceContext::Devices` from `lib/API/Device.cpp` contains more than one device. 
    When `api-query` or `offloader` exit, some memory resources are failing to automatically free themselves. 
    Valgrind output suggests the core issue stems from the implementation of `Microsoft::WRL::ComPtr::InternalRelease` in WSL. 
    Manually clearing `DeviceContext::Devices` before a program exit appears to eliminate the issue for the exit
  - An additional workaround that would cover all possible program exit points is to only select one `DXDevice` by editing `lib/API/DX/Device.cpp` to `break;` at the end of the first iteration of the for-loop in the function `InitializeDXDevices()`
    - Before implementing this workaround, you can run `api-query` to list all GPU devices available. Afterwards, edit the `AdapterIndex` of the for-loop to select the index of the desired GPU

