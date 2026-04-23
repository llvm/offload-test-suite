 # Check if in WSL and if has DirectX driver and runtime
if (NOT EXISTS "/usr/lib/wsl/lib/")
  return()
endif()

# List of D3D libraries from WSL
find_library(LIBD3D12 d3d12 HINTS /usr/lib/wsl/lib)
find_library(LIBD3D12CORE d3d12core HINTS /usr/lib/wsl/lib)
find_library(LIBDXCORE dxcore HINTS /usr/lib/wsl/lib)

# List of D3D libraries from DirectX-Headers
find_library(LIBD3DX12-FORMAT-PROPERTIES d3dx12-format-properties)
find_library(LIBDIRECTX-GUIDS DirectX-Guids)

# libd3d12.so is a shim that loads libd3d12core.so at runtime via dlopen and
# resolves D3D12GetInterface via dlsym. Due to ELF symbol interposition rules,
# dlsym may resolve D3D12GetInterface back to libd3d12.so's own export instead
# of libd3d12core.so's implementation, causing infinite recursion. Explicitly
# linking libd3d12core.so *before* libd3d12.so ensures it appears first in the
# global symbol table, so dlsym finds the correct implementation.
if (LIBD3D12CORE)
  list(APPEND D3D12_WSL_LIBRARIES ${LIBD3D12CORE})
endif()

list(APPEND D3D12_WSL_LIBRARIES
     ${LIBD3D12}
     ${LIBDXCORE}
     ${LIBD3DX12-FORMAT-PROPERTIES}
     ${LIBDIRECTX-GUIDS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set D3D12_WSL_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(D3D12_WSL DEFAULT_MSG
                                  LIBD3D12 LIBDXCORE
                                  LIBD3DX12-FORMAT-PROPERTIES LIBDIRECTX-GUIDS)

mark_as_advanced(D3D12_WSL_LIBRARIES)

