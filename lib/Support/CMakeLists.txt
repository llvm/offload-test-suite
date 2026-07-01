add_offloadtest_library(Support Pipeline.cpp Check.cpp OffloadMigration.cpp)

if (OFFLOADTEST_ENABLE_VULKAN)
  target_include_directories(OffloadTestSupport SYSTEM BEFORE PRIVATE ${Vulkan_INCLUDE_DIRS})
endif()
