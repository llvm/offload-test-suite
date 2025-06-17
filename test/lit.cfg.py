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


def setDeviceFeatures(config, device, compiler):
    API = device["API"]
    config.available_features.add(API)
    if "Microsoft Basic Render Driver" in device["Description"]:
        config.available_features.add("%s-WARP" % API)
    if "Intel" in device["Description"]:
        config.available_features.add("%s-Intel" % API)
    if "NVIDIA" in device["Description"]:
        config.available_features.add("%s-NV" % API)
    if "AMD" in device["Description"]:
        config.available_features.add("%s-AMD" % API)

    config.available_features.add("%s-%s" % (compiler, API))

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


if config.offloadtest_test_warp:
    tools.append(
        ToolSubst("%offloader", command=FindTool("offloader"), extra_args=["-warp"])
    )
else:
    tools.append(ToolSubst("%offloader", FindTool("offloader")))

ExtraCompilerArgs = []
if config.offloadtest_enable_vulkan:
    ExtraCompilerArgs = ["-spirv", "-fspv-target-env=vulkan1.3"]
if config.offloadtest_enable_metal:
    ExtraCompilerArgs = ["-metal"]
    # metal-irconverter version: 3.0.0
    MSCVersionOutput = subprocess.check_output(
        ["metal-shaderconverter", "--version"]
    ).decode("UTF-8")
    VersionRegex = re.compile("metal-irconverter version: ([0-9]+)\.([0-9]+)\.([0-9]+)")
    VersionMatch = VersionRegex.match(MSCVersionOutput)
    if VersionMatch:
        FullVersion = ".".join(VersionMatch.groups()[0:3])
        config.available_features.add("metal-shaderconverter-%s" % FullVersion)
        MajorVersion = int(VersionMatch.group(1))
        for I in range(1, MajorVersion + 1):
            config.available_features.add("metal-shaderconverter-%s.0.0-or-later" % I)

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
    # Bail from th eloop if we found a device that matches what we're looking for.
    if target_device:
        break

if not target_device:
    config.fatal("No target device found!")
setDeviceFeatures(config, target_device, HLSLCompiler)

if os.path.exists(config.goldenimage_dir):
  config.substitutions.append(("%goldenimage_dir", config.goldenimage_dir))
  config.available_features.add("goldenimage")

# Ask llvm-config about asserts
llvm_config.feature_config(
    [
        ("--assertion-mode", {"ON": "asserts"}),
        ("--build-mode", {"[Dd][Ee][Bb][Uu][Gg]": "debug"}),
    ]
)