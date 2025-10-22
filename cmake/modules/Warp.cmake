function(guess_nuget_arch output_var)
  if ((CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "x64"))
    set(${output_var} "x64" PARENT_SCOPE)
  elseif ((CMAKE_GENERATOR_PLATFORM STREQUAL "x86") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "x86"))
    set(${output_var} "x86" PARENT_SCOPE)
  elseif ((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64.*") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" MATCHES "ARM64.*"))
    set(${output_var} "arm64" PARENT_SCOPE)
  elseif ((CMAKE_GENERATOR_PLATFORM MATCHES "ARM.*") OR ("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" MATCHES "ARM.*"))
    set(${output_var} "arm" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Failed to guess NuGet arch! (${CMAKE_GENERATOR_PLATFORM}, ${CMAKE_C_COMPILER_ARCHITECTURE_ID})")
  endif()
endfunction()

function(setup_warp version)
  if (NOT WIN32)
    return()
  endif()

  if (version STREQUAL "System")
    return()
  endif()

  guess_nuget_arch(NUGET_ARCH)

  if (version STREQUAL "LKG")
    set(version "1.0.15")
    set(version_description "Latest Known Good for ${NUGET_ARCH} (${version})")
  else ()
    set(version_description "Custom (${version})")
  endif()

  message(STATUS "Fetching WARP ${version_description}...")

  set(WARP_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/Microsoft.Direct3D.WARP.${version}.zip")
  if (version STREQUAL "Latest")
    file(DOWNLOAD "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.WARP/" ${WARP_ARCHIVE})
  else()
    file(DOWNLOAD "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.WARP/${version}/" ${WARP_ARCHIVE})
  endif()

  # This is all awfulness to work around the fact that the last known good WRAP
  # for x64 is before arm64 support was shipped via NuGet, and the packaging
  # changed and we just aren't allowed to have nice things.
  file(ARCHIVE_EXTRACT INPUT ${WARP_ARCHIVE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/warp" PATTERNS *dll *pdb)

  file(GLOB_RECURSE LIBS "${CMAKE_CURRENT_BINARY_DIR}/warp/build/native/*/${NUGET_ARCH}/*.dll"
       $<IF:$<CONFIG:DEBUG>,"${CMAKE_CURRENT_BINARY_DIR}/warp/build/native/*/${NUGET_ARCH}/*.pdb">)
  
  if (${NUGET_ARCH} STREQUAL "x64" AND NOT LIBS)
    file(GLOB_RECURSE LIBS "${CMAKE_CURRENT_BINARY_DIR}/warp/build/native/amd64/*.dll"
         $<IF:$<CONFIG:DEBUG>,"${CMAKE_CURRENT_BINARY_DIR}/warp/build/native/amd64/*.pdb">)
  endif ()
  
  if (NOT LIBS)
    message(FATAL_ERROR "Requested version of WARP does not support current architecture (or it was packaged in a way we don't handle).")
  endif()

  file(MAKE_DIRECTORY "${LLVM_RUNTIME_OUTPUT_INTDIR}")
  foreach(FILE ${LIBS})
    get_filename_component(FILENAME ${FILE} NAME)
    file(COPY_FILE ${FILE} "${LLVM_RUNTIME_OUTPUT_INTDIR}/${FILENAME}")
  endforeach()

  file(REMOVE_RECURSE "${CMAKE_CURRENT_BINARY_DIR}/warp")
  set_property(GLOBAL PROPERTY WARP_ARCHITECTURE ${NUGET_ARCH})
endfunction()

set(WARP_VERSION "LKG" CACHE STRING "")
setup_warp(${WARP_VERSION})
