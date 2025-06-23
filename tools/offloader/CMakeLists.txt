add_offloadtest_tool(offloader
              offloader.cpp)

target_link_libraries(offloader PRIVATE
                      LLVMSupport
                      OffloadTestAPI
                      OffloadTestImage
                      OffloadTestSupport)

if (APPLE AND OFFLOADTEST_ENABLE_VULKAN)
  get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
  set_property(TARGET offloader APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,-rpath,${_Vulkan_LIB_DIR} ")
endif()
