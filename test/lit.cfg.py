# -*- Python -*-

# Configuration file for the 'lit' test runner.

import os
import sys
import re
import platform
import subprocess
import yaml

import lit.util
import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool
from lit.llvm.subst import ToolSubst

# name: The name of this test suite.
config.name = "OffloadTest-" + config.offloadtest_suite

# testFormat: The test format to use to interpret tests.
config.test_format = lit.formats.ShTest(not llvm_config.use_lit_shell)

# suffixes: A list of file extensions to treat as test files. This is overriden
# by individual lit.local.cfg files in the test subdirectories.
config.suffixes = [".test", ".yaml"]

# excludes: A list of directories to exclude from the testsuite. The 'Inputs'
# subdirectories contain auxiliary inputs for various tests in their parent
# directories.
config.excludes = ["Inputs", "CMakeLists.txt", "README.txt", "LICENSE.txt"]

# test_source_root: The root path where tests are located.
config.test_source_root = os.path.dirname(__file__)

# test_exec_root: The root path where tests should be run.
config.test_exec_root = os.path.join(
    config.offloadtest_obj_root, "test", config.offloadtest_suite
)

# Tweak the PATH to include the tools dir.
llvm_config.with_environment("PATH", config.llvm_tools_dir, append_path=True)

tools = [
    ToolSubst("FileCheck", FindTool("FileCheck")),
    ToolSubst("split-file", FindTool("split-file")),
    ToolSubst("imgdiff", FindTool("imgdiff")),
]


def getHighestShaderModel(features):
    if features == None:
        return 6, 0
    sm = features.get("HighestShaderModel", 6.0)
    major, minor = str(sm).split(".")
    return int(major), int(minor)


def setDeviceFeatures(config, device, compiler):
    API = device["API"]
    config.available_features.add(API)
    config.available_features.add("%s-%s" % (API, config.offloadtest_os))
    if "Microsoft Basic Render Driver" in device["Description"]:
        config.available_features.add("%s-WARP" % API)
        config.available_features.add("WARP-%s" % config.warp_arch)
        config.available_features.add("WARP")
    if "Intel" in device["Description"]:
        config.available_features.add("%s-Intel" % API)
        if "UHD Graphics" in device["Description"] and API == "DirectX":
            # When Intel resolves the driver issue and tests XFAILing on the
            # feature below are resolved we can resolve
            # https://github.com/llvm/offload-test-suite/issues/226 by updating
            # this check to only XFAIL on old driver versions.
            config.available_features.add("Intel-Memory-Coherence-Issue-226")
    if "NVIDIA" in device["Description"]:
        config.available_features.add("%s-NV" % API)
        NV50SeriesRegex = re.compile("NVIDIA GeForce [A-Z]+ 50[0-9]+")
        NV50SeriesMatch = NV50SeriesRegex.match(device["Description"])
        if NV50SeriesMatch and API == "DirectX":
            config.available_features.add("NV-Reconvergence-Issue-320")
    if "AMD" in device["Description"]:
        config.available_features.add("%s-AMD" % API)
    if "Qualcomm" in device["Description"]:
        config.available_features.add("%s-QC" % API)

    config.available_features.add("%s-%s" % (compiler, API))

    HighestShaderModel = getHighestShaderModel(device["Features"])
    if (6, 6) <= HighestShaderModel:
        # https://github.com/microsoft/DirectX-Specs/blob/master/d3d/HLSL_ShaderModel6_6.md#derivatives
        config.available_features.add("DerivativesInCompute")

    if device["API"] == "DirectX":
        if device["Features"].get("Native16BitShaderOpsSupported", False):
            config.available_features.add("Int16")
            config.available_features.add("Half")
        if device["Features"].get("DoublePrecisionFloatShaderOps", False):
            config.available_features.add("Double")
        if device["Features"].get("Int64ShaderOps", False):
            config.available_features.add("Int64")

    if device["API"] == "Metal":
        config.available_features.add("Int16")
        config.available_features.add("Int64")
        config.available_features.add("Half")

    if device["API"] == "Vulkan":
        if device["Features"].get("shaderInt16", False):
            config.available_features.add("Int16")
        if device["Features"].get("shaderFloat16", False):
            config.available_features.add("Half")
        if device["Features"].get("shaderFloat64", False):
            config.available_features.add("Double")
        if device["Features"].get("shaderInt64", False):
            config.available_features.add("Int64")

        # Add supported extensions.
        for Extension in device["Extensions"]:
            config.available_features.add(Extension["ExtensionName"])


offloader_args = []
if config.offloadtest_test_warp:
    offloader_args.append("-warp")
if config.offloadtest_enable_debug:
    offloader_args.append("-debug-layer")
if config.offloadtest_enable_validation:
    offloader_args.append("-validation-layer")

tools.append(
    ToolSubst("%offloader", command=FindTool("offloader"), extra_args=offloader_args)
)

ExtraCompilerArgs = []
if config.offloadtest_enable_vulkan:
    ExtraCompilerArgs = ["-spirv", "-fspv-target-env=vulkan1.3"]
    if config.offloadtest_test_clang:
        ExtraCompilerArgs.append("-fspv-extension=DXC")
if config.offloadtest_enable_metal:
    ExtraCompilerArgs = ["-metal"]
    # metal-irconverter version: 3.0.0
    MSCVersionOutput = subprocess.check_output(
        ["metal-shaderconverter", "--version"]
    ).decode("UTF-8")
    VersionRegex = re.compile(
        r"metal-irconverter version: ([0-9]+)\.([0-9]+)\.([0-9]+)")
    VersionMatch = VersionRegex.match(MSCVersionOutput)
    if VersionMatch:
        FullVersion = ".".join(VersionMatch.groups()[0:3])
        config.available_features.add("metal-shaderconverter-%s" % FullVersion)
        MajorVersion = int(VersionMatch.group(1))
        for I in range(1, MajorVersion + 1):
            config.available_features.add(
                f"metal-shaderconverter-{I}.0.0-or-later")

HLSLCompiler = ""
if config.offloadtest_test_clang:
    if os.path.exists(config.offloadtest_dxc_dir):
        ExtraCompilerArgs.append("--dxv-path=%s" % config.offloadtest_dxc_dir)
        tools.append(
            ToolSubst(
                "%dxc_target", FindTool("clang-dxc"), extra_args=ExtraCompilerArgs
            )
        )
    else:
        tools.append(
            ToolSubst(
                "%dxc_target", FindTool("clang-dxc"), extra_args=ExtraCompilerArgs
            )
        )
    HLSLCompiler = "Clang"
else:
    tools.append(
        ToolSubst("%dxc_target", config.offloadtest_dxc, extra_args=ExtraCompilerArgs)
    )
    HLSLCompiler = "DXC"

config.available_features.add(HLSLCompiler)

tools.append(ToolSubst("obj2yaml", FindTool("obj2yaml")))

llvm_config.add_tool_substitutions(tools, config.llvm_tools_dir)

api_query = os.path.join(config.llvm_tools_dir, "api-query")
query_string = subprocess.check_output(api_query)
devices = yaml.safe_load(query_string)
target_device = None

# Find the right device to configure against
for device in devices["Devices"]:
    is_warp = "Microsoft Basic Render Driver" in device["Description"]
    if device["API"] == "DirectX" and config.offloadtest_enable_d3d12:
        if is_warp and config.offloadtest_test_warp:
            target_device = device
        elif not is_warp and not config.offloadtest_test_warp:
            target_device = device
    if device["API"] == "Metal" and config.offloadtest_enable_metal:
        target_device = device
    if device["API"] == "Vulkan" and config.offloadtest_enable_vulkan:
        target_device = device
    # Bail from the loop if we found a device that matches what we're looking for.
    if target_device:
        break

if not target_device:
    config.fatal("No target device found!")
setDeviceFeatures(config, target_device, HLSLCompiler)

if os.path.exists(config.goldenimage_dir):
    config.substitutions.append(("%goldenimage_dir", config.goldenimage_dir))
    config.available_features.add("goldenimage")
