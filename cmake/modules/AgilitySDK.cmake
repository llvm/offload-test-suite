include(NuGet)

function(setup_agility_sdk version)
  if (NOT WIN32)
    return()
  endif()

  if (version STREQUAL "System")
    set_property(GLOBAL PROPERTY OFFLOADTEST_AGILITY_SDK_SYSTEM TRUE)
    return()
  endif()

  guess_nuget_arch(nuget_arch)
  if (nuget_arch STREQUAL "x86")
    set(agility_arch "win32")
  elseif (nuget_arch STREQUAL "x64" OR nuget_arch STREQUAL "arm64")
    set(agility_arch "${nuget_arch}")
  else()
    message(FATAL_ERROR
      "The DirectX 12 Agility SDK does not support ${nuget_arch}.")
  endif()

  if (version STREQUAL "LKG")
    set(version "1.619.5")
    set(version_description "Latest Known Good (${version})")
  elseif (version STREQUAL "Latest")
    set(version_description "Latest stable")
  else()
    set(version_description "Custom (${version})")
  endif()

  message(STATUS "Fetching DirectX 12 Agility SDK ${version_description}...")
  download_nuget_package("Microsoft.Direct3D.D3D12" "${version}"
    agility_archive)

  set(extract_dir "${CMAKE_CURRENT_BINARY_DIR}/agility-sdk")
  file(REMOVE_RECURSE "${extract_dir}")
  file(ARCHIVE_EXTRACT
    INPUT "${agility_archive}"
    DESTINATION "${extract_dir}"
    PATTERNS
      "*.nuspec"
      "build/native/bin/${agility_arch}/D3D12Core.dll"
      "build/native/bin/${agility_arch}/d3d12SDKLayers.dll")

  file(GLOB nuspec_files "${extract_dir}/*.nuspec")
  list(LENGTH nuspec_files nuspec_count)
  if (NOT nuspec_count EQUAL 1)
    message(FATAL_ERROR
      "Expected one NuGet specification in the Agility SDK package, "
      "found ${nuspec_count}.")
  endif()

  list(GET nuspec_files 0 nuspec_file)
  file(READ "${nuspec_file}" nuspec)
  string(REGEX MATCH "<version>([^<]+)</version>" unused "${nuspec}")
  set(package_version "${CMAKE_MATCH_1}")
  if (NOT package_version MATCHES "^1\\.([0-9]+)(\\.|$)")
    message(FATAL_ERROR
      "Cannot derive D3D12SDKVersion from Agility SDK package version "
      "'${package_version}'.")
  endif()
  set(AGILITY_SDK_VERSION_NUMBER "${CMAKE_MATCH_1}")

  set(package_bin "${extract_dir}/build/native/bin/${agility_arch}")
  foreach(runtime_file D3D12Core.dll d3d12SDKLayers.dll)
    if (NOT EXISTS "${package_bin}/${runtime_file}")
      message(FATAL_ERROR
        "Agility SDK package ${package_version} does not contain "
        "${agility_arch}/${runtime_file}.")
    endif()
  endforeach()

  install(FILES
      "${package_bin}/D3D12Core.dll"
      "${package_bin}/d3d12SDKLayers.dll"
    DESTINATION "${LLVM_TOOLS_INSTALL_DIR}/D3D12"
    COMPONENT offload-tools)

  configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/AgilitySDK.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/AgilitySDK.cpp"
    @ONLY)
  set_property(GLOBAL PROPERTY OFFLOADTEST_AGILITY_SDK_SOURCE
    "${CMAKE_CURRENT_BINARY_DIR}/AgilitySDK.cpp")
  set_property(GLOBAL PROPERTY OFFLOADTEST_AGILITY_SDK_BIN_DIR
    "${package_bin}")

  message(STATUS
    "Using DirectX 12 Agility SDK ${package_version} "
    "(D3D12SDKVersion ${AGILITY_SDK_VERSION_NUMBER})")
endfunction()

function(target_enable_agility_sdk target)
  get_property(agility_source GLOBAL PROPERTY OFFLOADTEST_AGILITY_SDK_SOURCE)
  if (agility_source)
    target_sources(${target} PRIVATE "${agility_source}")
    get_property(agility_bin_dir GLOBAL
      PROPERTY OFFLOADTEST_AGILITY_SDK_BIN_DIR)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_FILE_DIR:${target}>/D3D12"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${agility_bin_dir}/D3D12Core.dll"
        "$<TARGET_FILE_DIR:${target}>/D3D12/D3D12Core.dll"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${agility_bin_dir}/d3d12SDKLayers.dll"
        "$<TARGET_FILE_DIR:${target}>/D3D12/d3d12SDKLayers.dll"
      VERBATIM)
  else()
    get_property(use_system_sdk GLOBAL
      PROPERTY OFFLOADTEST_AGILITY_SDK_SYSTEM)
    if (use_system_sdk)
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E remove_directory
          "$<TARGET_FILE_DIR:${target}>/D3D12"
        VERBATIM)
    endif()
  endif()
endfunction()

set(AGILITY_SDK_VERSION "LKG" CACHE STRING
  "DirectX 12 Agility SDK version (LKG, System, Latest, or a NuGet version)")
set_property(CACHE AGILITY_SDK_VERSION PROPERTY STRINGS LKG System Latest)
setup_agility_sdk("${AGILITY_SDK_VERSION}")
