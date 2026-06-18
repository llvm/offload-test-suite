# Third Party Vendored Libraries

## DirectX-Headers

* URL: https://github.com/microsoft/DirectX-Headers
* Version: 7d52e38
* License: MIT

The DirectX headers are used by the test runtime on Windows for executing GPU
binaries through the DirectX API.

## libpng

* URL: http://libpng.org
* Version: 1.6.34
* License: PNG Reference Library License version 2

The libpng library is used by the test runtime to emit viewable images in PNG
format.

## metal_ir_converter

* URL: https://developer.apple.com/metal/shader-converter/
* Version: 3.1.0
* License: Apache 2.0

Metal shader converter converts shader intermediate representations in LLVM IR bytecode
into a form suitable to be loaded into Metal.

## metal_ir_converter_runtime

* URL: https://developer.apple.com/metal/shader-converter/
* Version: 3.1.0
* License: Apache 2.0

The Metal IR Converter runtime is part of the Metal Shader Converter tooling
provided by Apple for converting DXIL for execution on the Metal runtime. These
headers are used to read and construct Metal command buffers for executing HLSL
shaders on Metal.

## metal-cpp

* URL: https://developer.apple.com/metal/cpp/
* Version: macOS 14, iOS 17
* License: Apache 2.0

The Metal-CPP library is a header-only wrapper implementation around Metal's
Objective-C API to provide a modern C++ API interface.

## zlib

* URL: https://www.zlib.net
* Version: 1.3.1
* License: zlib license

On Windows where zlib is not available on the base system, the vendored copy of
zlib is used when building libpng for image compression.
