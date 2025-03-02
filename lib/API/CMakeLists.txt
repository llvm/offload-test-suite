if (OFFLOADTEST_ENABLE_VULKAN)
  list(APPEND api_sources VK/Device.cpp)
  list(APPEND api_libraries ${Vulkan_LIBRARIES})
  list(APPEND api_headers PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()

if (OFFLOADTEST_ENABLE_D3D12)
  list(APPEND api_sources DX/Device.cpp DX/DXFeatures.cpp)
  list(APPEND api_headers PRIVATE
    ${DIRECTX_HEADERS_PATH}/include/directx
    ${DIRECTX_HEADERS_PATH}/include/dxguids
  )
  if (WIN32)
    list(APPEND api_libraries ${D3D12_LIBRARIES})
  elseif (D3D12_WSL_FOUND)
    list(APPEND api_libraries ${D3D12_WSL_LIBRARIES})
    list(APPEND api_headers PRIVATE ${DIRECTX_HEADERS_PATH}/include/wsl/stubs)
  endif()
endif()

if (APPLE)
  list(APPEND api_sources MTL/MTLDevice.cpp)
  list(APPEND api_libraries "-framework Metal"
                            "-framework MetalKit"
                            "-framework AppKit"
                            "-framework Foundation"
                            "-framework QuartzCore")
  list(APPEND api_headers PRIVATE ${METAL_INCLUDE_DIRS})
endif()

add_offloadtest_library(API
  Capabilities.cpp
  Device.cpp
  ${api_sources})

target_include_directories(OffloadTestAPI SYSTEM BEFORE ${api_headers})

target_link_libraries(OffloadTestAPI INTERFACE OffloadTestSupport ${api_libraries})
if (TARGET DIRECTX_HEADERS)
  add_dependencies(OffloadTestAPI DIRECTX_HEADERS)
endif()
