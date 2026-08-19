#!/bin/bash

target_dir="./tmp"

# Key logic: -d detects whether a path is an existing directory
if [ -d "${target_dir}" ]; then
    # rm -rf Force recursive deletedirectory(Deletes files/subdirectories inside, no errors)
    rm -rf "${target_dir}"
    echo "tmp directory in current directory existed; successfully deleted"
else
    echo "当前directory下not foundtmpdirectory,noneneedDelete"
fi

mkdir tmp
cd tmp

# Try to copy waf from a shared cache or sibling app's cache first
CACHED_WAF=""
WORKSPACE_DIR="${2:-$(cd ../.. && pwd)}"
SHARED_CACHE="$WORKSPACE_DIR/.waf_cache"
if [[ -f "$SHARED_CACHE/waf" ]]; then
    CACHED_WAF="$SHARED_CACHE/waf"
else
    for sibling in ../intra_process_demo/config/tmp ../examples/config/tmp; do
        if [[ -f "$sibling/waf" ]]; then
            CACHED_WAF="$sibling/waf"
            break
        fi
    done
fi

if [[ -n "$CACHED_WAF" ]]; then
    cp "$CACHED_WAF" waf
    echo "Copied waf from cache: $CACHED_WAF"
else
    curl https://waf.io/waf-2.0.19 > waf || { echo "Failed to download waf"; exit 1; }
    echo "Downloaded waf from internet"
fi
chmod +x ./waf

# Try to copy rtems_waf from a shared cache or sibling app's cache first
CACHED_RTEMS_WAF=""
if [[ -d "$SHARED_CACHE/rtems_waf" ]]; then
    CACHED_RTEMS_WAF="$SHARED_CACHE/rtems_waf"
else
    for sibling in ../intra_process_demo/config/tmp ../examples/config/tmp; do
        if [[ -d "$sibling/rtems_waf" ]]; then
            CACHED_RTEMS_WAF="$sibling/rtems_waf"
            break
        fi
    done
fi

if [[ -n "$CACHED_RTEMS_WAF" ]]; then
    cp -r "$CACHED_RTEMS_WAF" rtems_waf
    echo "Copied rtems_waf from cache: $CACHED_RTEMS_WAF"
else
    git init
    git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf || { echo "Failed to clone rtems_waf"; exit 1; }
    echo "Cloned rtems_waf from internet"
fi

touch init.c
echo "/*
 * Simple RTEMS configuration
 */

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>" > init.c

touch wscript
echo '
#!/usr/bin/env python
# A Waf script to build an RTEMS project with static libraries

from __future__ import print_function

rtems_version = "6"

try:
    import rtems_waf.rtems as rtems
except ImportError:
    print("error: no rtems_waf git submodule")
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)
    ctx.load("compiler_cxx")  # Ensure C++ compiler is loaded

def bsp_configure(conf, arch_bsp):
    # 这里can以进Row BSP 相关的ConfigurationCheck
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    # pingpong target (executable)
    bld.program(
        target="pingpong.exe",
        source=[
            "../../code/src/examples-0.20.2/rclcpp/executors/cbg_executor/src/examples_rclcpp_cbg_executor/ping_node.cpp",
            "../../code/src/examples-0.20.2/rclcpp/executors/cbg_executor/src/examples_rclcpp_cbg_executor/pong_node.cpp",
            "../../code/src/examples-0.20.2/rclcpp/executors/cbg_executor/src/ping_pong.cpp",
        ],
        includes=[
            "../../code/src/examples-0.20.2/rclcpp/executors/cbg_executor/include",
            "../../../rclcpp/code/src/rclcpp-16.0.8/rclcpp/include",
            "../../../rcl/code/src/rcl-5.3.7/rcl/include",
            "../../../rcutils/code/src/rcutils-5.1.5/include",
            "../../../rmw/code/src/rmw-6.1.1/rmw/include",
            "../../../rosidl/code/src/rosidl/rosidl_runtime_c/include",
            "../../../rosidl/code/src/rosidl/rosidl_typesupport_interface/include",
            "../../../rcpputils/code/src/rcpputils-2.4.2/include",
            "../../../builtin_interfaces/code/src/builtin_interfaces/rosidl_generator_cpp",
            "../../../rosidl/code/src/rosidl/rosidl_runtime_cpp/include",
            "../../../rcl_interfaces/code/src/rcl_interfaces/rosidl_generator_cpp",
            "../../../statistics_msgs/code/src/statistics_msgs/rosidl_generator_cpp",
            "../../../statistics_collector/code/src/include",
            "../../../rcl/code/src/rcl-5.3.7/rcl_yaml_param_parser/include",
            "../../../std_msgs/code/src/std_msgs/rosidl_generator_cpp",
        ],
        defines=[
            "RMW_IMPLEMENTATION=rmw_fastrtps_cpp"
        ],
        cxxflags="-DRTEMS -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g -fPIC",
        linkflags="-static",
        lib=[
            "rclcpp",
            "rcl",
            "statistics_collector",

            "statistics_msgs__rosidl_generator_c",
            "statistics_msgs__rosidl_typesupport_c",
            "statistics_msgs__rosidl_typesupport_cpp",
            "statistics_msgs__rosidl_typesupport_fastrtps_c",
            "statistics_msgs__rosidl_typesupport_fastrtps_cpp",
            "statistics_msgs__rosidl_typesupport_introspection_c",
            "statistics_msgs__rosidl_typesupport_introspection_cpp",

            "rcl_interfaces__rosidl_generator_c",
            "rcl_interfaces__rosidl_typesupport_c",
            "rcl_interfaces__rosidl_typesupport_cpp",
            "rcl_interfaces__rosidl_typesupport_fastrtps_c",
            "rcl_interfaces__rosidl_typesupport_fastrtps_cpp",
            "rcl_interfaces__rosidl_typesupport_introspection_c",
            "rcl_interfaces__rosidl_typesupport_introspection_cpp",

            "rmw_fastrtps_cpp",
            "rmw_fastrtps_shared_cpp",
            "rmw_fastrtps_dynamic_cpp",
            "rmw_implementation",
            "rmw",
            "ament_index_cpp",
            "class-loader",

            "example_interfaces__rosidl_generator_c",
            "example_interfaces__rosidl_typesupport_c",
            "example_interfaces__rosidl_typesupport_cpp",
            "example_interfaces__rosidl_typesupport_fastrtps_c",
            "example_interfaces__rosidl_typesupport_fastrtps_cpp",
            "example_interfaces__rosidl_typesupport_introspection_c",
            "example_interfaces__rosidl_typesupport_introspection_cpp",

            "action_msgs__rosidl_generator_c",
            "action_msgs__rosidl_typesupport_c",
            "action_msgs__rosidl_typesupport_cpp",
            "action_msgs__rosidl_typesupport_fastrtps_c",
            "action_msgs__rosidl_typesupport_fastrtps_cpp",
            "action_msgs__rosidl_typesupport_introspection_c",
            "action_msgs__rosidl_typesupport_introspection_cpp",

            "rcl-yaml-param-parser",
            "rcl_logging_noop",

            "rosgraph_msgs__rosidl_generator_c",
            "rosgraph_msgs__rosidl_typesupport_c",
            "rosgraph_msgs__rosidl_typesupport_cpp",
            "rosgraph_msgs__rosidl_typesupport_fastrtps_c",
            "rosgraph_msgs__rosidl_typesupport_fastrtps_cpp",
            "rosgraph_msgs__rosidl_typesupport_introspection_c",
            "rosgraph_msgs__rosidl_typesupport_introspection_cpp",

            "rosidl_typesupport_c",
            "rosidl_typesupport_fastrtps_cpp",
            "rosidl_typesupport_cpp",
            "rosidl_typesupport_fastrtps_c",
            "rosidl_typesupport_introspection_c",
            "rosidl_typesupport_introspection_cpp",

            "std_msgs__rosidl_generator_c",
            "std_msgs__rosidl_typesupport_c",
            "std_msgs__rosidl_typesupport_cpp",
            "std_msgs__rosidl_typesupport_fastrtps_c",
            "std_msgs__rosidl_typesupport_fastrtps_cpp",
            "std_msgs__rosidl_typesupport_introspection_c",
            "std_msgs__rosidl_typesupport_introspection_cpp",

            "rmw_dds_common",
            "rmw_dds_common__rosidl_generator_c",
            "rmw_dds_common__rosidl_typesupport_c",
            "rmw_dds_common__rosidl_typesupport_cpp",
            "rmw_dds_common__rosidl_typesupport_fastrtps_c",
            "rmw_dds_common__rosidl_typesupport_fastrtps_cpp",
            "rmw_dds_common__rosidl_typesupport_introspection_c",
            "rmw_dds_common__rosidl_typesupport_introspection_cpp",

            "rcl_interfaces__rosidl_typesupport_introspection_cpp",
            "rcl_interfaces__rosidl_typesupport_introspection_c",
            "rcl_interfaces__rosidl_typesupport_fastrtps_cpp",
            "rcl_interfaces__rosidl_typesupport_fastrtps_c",
            "rcl_interfaces__rosidl_typesupport_cpp",
            "rcl_interfaces__rosidl_typesupport_c",
            "rcl_interfaces__rosidl_generator_c",

            "builtin_interfaces__rosidl_generator_c",
            "builtin_interfaces__rosidl_typesupport_c",
            "builtin_interfaces__rosidl_typesupport_cpp",
            "builtin_interfaces__rosidl_typesupport_fastrtps_c",
            "builtin_interfaces__rosidl_typesupport_fastrtps_cpp",
            "builtin_interfaces__rosidl_typesupport_introspection_c",
            "builtin_interfaces__rosidl_typesupport_introspection_cpp",

            "rosidl_runtime_c",

            "fastrtps",
            "fastcdr",
            "foonathan_memory",
            "tinyxml2",
            "rcpputils",
            "yaml",
            "rcutils",

            "stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c", "bsd"
        ],
    )
' > wscript
