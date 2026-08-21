function(guess_nuget_arch output_var)
  if (CMAKE_C_COMPILER_ARCHITECTURE_ID)
    set(compiler_arch "${CMAKE_C_COMPILER_ARCHITECTURE_ID}")
  else()
    set(compiler_arch "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
  endif()

  if ((CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR
      (compiler_arch STREQUAL "x64"))
    set(${output_var} "x64" PARENT_SCOPE)
  elseif ((CMAKE_GENERATOR_PLATFORM STREQUAL "x86") OR
          (compiler_arch STREQUAL "x86"))
    set(${output_var} "x86" PARENT_SCOPE)
  elseif ((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64.*") OR
          (compiler_arch MATCHES "ARM64.*"))
    set(${output_var} "arm64" PARENT_SCOPE)
  elseif ((CMAKE_GENERATOR_PLATFORM MATCHES "ARM.*") OR
          (compiler_arch MATCHES "ARM.*"))
    set(${output_var} "arm" PARENT_SCOPE)
  else()
    message(FATAL_ERROR
      "Failed to guess NuGet arch! "
      "(${CMAKE_GENERATOR_PLATFORM}, ${compiler_arch})")
  endif()
endfunction()

function(download_nuget_package package version output_var)
  set(archive "${CMAKE_CURRENT_BINARY_DIR}/${package}.${version}.zip")
  if (version STREQUAL "Latest")
    set(url "https://www.nuget.org/api/v2/package/${package}/")
  else()
    set(url "https://www.nuget.org/api/v2/package/${package}/${version}/")
  endif()

  file(DOWNLOAD "${url}" "${archive}"
    STATUS download_status
    TLS_VERIFY ON)
  list(GET download_status 0 status_code)
  if (NOT status_code EQUAL 0)
    list(GET download_status 1 status_message)
    file(REMOVE "${archive}")
    message(FATAL_ERROR
      "Failed to download NuGet package ${package} ${version}: "
      "${status_message}")
  endif()

  set(${output_var} "${archive}" PARENT_SCOPE)
endfunction()
