# Including the native target is important because some of LLVM's tests fail if
# you don't.
set(LLVM_TARGETS_TO_BUILD "Native;SPIRV" CACHE STRING "")

# Include the DirectX target for DXIL code generation.
set(LLVM_EXPERIMENTAL_TARGETS_TO_BUILD "DirectX" CACHE STRING "")

set(LLVM_ENABLE_PROJECTS "clang;clang-tools-extra" CACHE STRING "")

set(CLANG_ENABLE_HLSL On CACHE BOOL "")

set(LLVM_INSTALL_UTILS ON CACHE BOOL "")
set(LLVM_INSTALL_TOOLCHAIN_ONLY OFF CACHE BOOL "")
set(LLVM_DISTRIBUTION_COMPONENTS
    clang
    hlsl-resource-headers
    FileCheck
    split-file
    obj2yaml
    not
    llvm-headers
    LLVMSupport
    LLVMDemangle   # Dependency of LLVMSupport
    LLVMObject
    # Dependencies of LLVMObject
    LLVMBitReader
    LLVMBitstreamReader # Dependency of LLVMBitReader
    LLVMCore
    LLVMRemarks # Dependency of LLVMCore
    LLVMMC
    LLVMDebugInfoDWARFLowLevel # Dependency of LLVMMC
    LLVMIRReader
    LLVMAsmParser # Dependency of LLVMIRReader
    LLVMBinaryFormat
    LLVMMCParser
    LLVMTargetParser
    LLVMTextAPI
    cmake-exports
    CACHE STRING "")
