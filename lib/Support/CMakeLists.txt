if (UNIX)
  include(CheckCXXSymbolExists)
  check_cxx_symbol_exists(sysctlbyname "sys/types.h;sys/sysctl.h"
                          OFFLOADTEST_HAVE_SYSCTLBYNAME)
endif()

add_offloadtest_library(Support
  Pipeline.cpp
  Check.cpp
  OffloadMigration.cpp
  HostInfo.cpp)

if (OFFLOADTEST_HAVE_SYSCTLBYNAME)
  target_compile_definitions(OffloadTestSupport
    PRIVATE OFFLOADTEST_HAVE_SYSCTLBYNAME)
endif()
