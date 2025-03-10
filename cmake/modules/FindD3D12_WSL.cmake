 # Check if in WSL and if has DirectX driver and runtime
if (NOT EXISTS "/usr/lib/wsl/lib/")
  return()
endif()

# List of D3D libraries from WSL
find_library(LIBD3D12 d3d12 HINTS /usr/lib/wsl/lib)
find_library(LIBDXCORE dxcore HINTS /usr/lib/wsl/lib)

# List of D3D libraries from DirectX-Headers
find_library(LIBD3DX12-FORMAT-PROPERTIES d3dx12-format-properties)
find_library(LIBDIRECTX-GUIDS DirectX-Guids)

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

