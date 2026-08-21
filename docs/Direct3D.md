# Direct3D on Windows

The offload-test-suite uses Direct3D 12 to execute DXIL binaries on Windows.
The build uses separate sources for the Direct3D headers, import libraries, and
runtime.

## Headers and import libraries

The test runtime compiles against the DirectX-Headers copy in
`third-party/DirectX-Headers`. The repository pins these headers so builds do
not depend on the header version installed on the machine.

CMake locates the installed Windows SDK to confirm that Direct3D 12 is
available. The build links its `d3d12.lib`, `dxcore.lib`, `dxguid.lib`, and
`d3dcompiler.lib` import libraries.

## DirectX 12 Agility SDK

By default, Windows builds download the `Microsoft.Direct3D.D3D12` NuGet
package and use its app-local DirectX 12 Agility SDK runtime. The build places
`D3D12Core.dll` and `D3D12SDKLayers.dll` in a `D3D12` directory next to
`offloader` and `api-query`. Both executables export the `D3D12SDKVersion` and
`D3D12SDKPath` values required to activate that runtime.

The **AGILITY_SDK_VERSION** CMake option selects the package:

* `LKG` is the default and uses the repository's known-good version, currently
  `1.619.5`.
* `System` disables app-local Agility SDK activation and uses the Windows
  Direct3D 12 runtime.
* `Latest` downloads the latest stable package from NuGet.
* An explicit NuGet package version may be specified.

For example:

```shell
cmake -DAGILITY_SDK_VERSION=System <other options...>
```

WARP is a Direct3D software adapter rather than a Direct3D runtime. Its version
is configured independently as described in [Using WARP on
Windows](WARP.md).

## WSL

The Agility SDK NuGet package contains Windows runtime DLLs and is not used as
the WSL runtime. WSL builds use the Direct3D libraries provided under
`/usr/lib/wsl/lib` and continue to compile against the vendored
DirectX-Headers, including their WSL stubs.
