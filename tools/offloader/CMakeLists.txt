add_offloadtest_tool(offloader
              offloader.cpp)

target_link_libraries(offloader PRIVATE
                      LLVMSupport
                      OffloadTestAPI
                      OffloadTestImage
                      OffloadTestSupport)

if (APPLE AND OFFLOADTEST_ENABLE_VULKAN)
  set_property(TARGET offloader APPEND_STRING PROPERTY
               LINK_FLAGS " -Wl,-rpath,/usr/local/lib ")
endif()
