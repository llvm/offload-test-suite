option(OFFLOADTEST_USE_NUGET_EXE
  "Use nuget.exe from PATH to download NuGet packages" ON)

if (OFFLOADTEST_USE_NUGET_EXE)
  find_program(OFFLOADTEST_NUGET_EXECUTABLE NAMES nuget.exe)
  mark_as_advanced(OFFLOADTEST_NUGET_EXECUTABLE)
endif()

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

function(download_nuget_package_with_cli package version archive)
  set(package_dir
    "${CMAKE_CURRENT_BINARY_DIR}/nuget-downloads/${package}")
  # Avoid stale archives when Latest resolves to a different package version.
  file(REMOVE_RECURSE "${package_dir}")

  set(nuget_args
    install "${package}"
    -OutputDirectory "${package_dir}"
    -DependencyVersion Ignore
    -DirectDownload
    -PackageSaveMode nupkg
    -NonInteractive
    -ForceEnglishOutput)
  if (NOT version STREQUAL "Latest")
    list(APPEND nuget_args -Version "${version}")
  endif()

  message(STATUS
    "Downloading NuGet package ${package} ${version} with "
    "${OFFLOADTEST_NUGET_EXECUTABLE}")
  execute_process(
    COMMAND "${OFFLOADTEST_NUGET_EXECUTABLE}" ${nuget_args}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    RESULT_VARIABLE nuget_result
    OUTPUT_VARIABLE nuget_output
    ERROR_VARIABLE nuget_error
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE)
  if (NOT nuget_result EQUAL 0)
    # Remove any partial package left by the failed NuGet invocation.
    file(REMOVE_RECURSE "${package_dir}")
    message(FATAL_ERROR
      "nuget.exe failed to download package ${package} ${version} "
      "(exit code ${nuget_result}).\n${nuget_output}${nuget_error}")
  endif()

  file(GLOB_RECURSE package_archives LIST_DIRECTORIES FALSE
    "${package_dir}/*.nupkg")
  list(LENGTH package_archives archive_count)
  if (NOT archive_count EQUAL 1)
    # Remove the unexpected package layout before reporting the error.
    file(REMOVE_RECURSE "${package_dir}")
    message(FATAL_ERROR
      "Expected nuget.exe to save one package archive for "
      "${package} ${version}, found ${archive_count}.")
  endif()

  list(GET package_archives 0 package_archive)
  file(COPY_FILE "${package_archive}" "${archive}" ONLY_IF_DIFFERENT)
  # Callers extract the retained archive, so discard NuGet's staging tree.
  file(REMOVE_RECURSE "${package_dir}")
endfunction()

function(download_nuget_package_direct package version archive)
  if (version STREQUAL "Latest")
    set(url "https://www.nuget.org/api/v2/package/${package}/")
  else()
    set(url "https://www.nuget.org/api/v2/package/${package}/${version}/")
  endif()

  message(STATUS
    "Downloading NuGet package ${package} ${version} directly from nuget.org")
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
endfunction()

function(download_nuget_package package version output_var)
  set(archive "${CMAKE_CURRENT_BINARY_DIR}/${package}.${version}.zip")
  file(REMOVE "${archive}")

  if (OFFLOADTEST_USE_NUGET_EXE AND OFFLOADTEST_NUGET_EXECUTABLE)
    download_nuget_package_with_cli("${package}" "${version}" "${archive}")
  else()
    if (OFFLOADTEST_USE_NUGET_EXE)
      message(STATUS
        "nuget.exe was not found on PATH; using the direct web downloader")
    else()
      message(STATUS
        "OFFLOADTEST_USE_NUGET_EXE is OFF; using the direct web downloader")
    endif()
    download_nuget_package_direct("${package}" "${version}" "${archive}")
  endif()

  set(${output_var} "${archive}" PARENT_SCOPE)
endfunction()
