add_offloadtest_tool(api-query
              api-query.cpp)

target_link_libraries(api-query PRIVATE LLVMSupport OffloadTestAPI)

if (APPLE AND OFFLOADTEST_ENABLE_VULKAN)
  get_filename_component(_Vulkan_LIB_DIR ${Vulkan_LIBRARY} DIRECTORY)
  set_property(TARGET api-query APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,-rpath,${_Vulkan_LIB_DIR} ")
endif()
