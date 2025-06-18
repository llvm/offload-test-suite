add_offloadtest_tool(api-query
              api-query.cpp)

target_link_libraries(api-query PRIVATE LLVMSupport OffloadTestAPI)

if (APPLE AND OFFLOADTEST_ENABLE_VULKAN)
  set_property(TARGET api-query APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,-rpath,/usr/local/lib ")
endif()
