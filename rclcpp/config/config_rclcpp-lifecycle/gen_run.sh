#!/bin/bash

target_dir="./tmp"
build_modle=$1
flag=0

root_path=$2
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_rclcpp.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"  # -pis key: creates build even if no subdirectories exist under root_path
fi

# Check if the file is not found; create it if missing
if [[ ! -f "$log_file_path" ]]; then
    touch "$log_file_path"  # Create empty file (if directory exists)
fi

# Key logic: -d detects whether a path is an existing directory
if [ -d "${target_dir}" ]; then
    # rm -rf Force recursive deletedirectory(Deletes files/subdirectories inside, no errors)
    if [[ "$build_modle" == "force" ]]; then
        rm -rf "${target_dir}"
        flag=1
    elif [[ "$build_modle" == "no-force" ]]; then
        flag=0
    else
        echo "build_modle arg error!!!"
        exit 1
    fi
else
    flag=1
fi

if [[ "$flag" == 1 ]]; then
    mkdir tmp
    cd tmp
    curl -s https://waf.io/waf-2.0.19 > waf
    chmod +x ./waf
    git init >> $log_file_path 2>&1
    GIT_CMD="git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf >> $log_file_path 2>&1"

    # Execute command and capture result
    if eval $GIT_CMD; then
        echo "✅ Submodule rtems_waf added successfully!" >> $log_file_path 2>&1
    else
        # Capture failure, output error info, and exit
        ERROR_CODE=$?  # Get command exit code
        echo "❌ Submodule rtems_waf add failed! Exit code: ${ERROR_CODE}" >> $log_file_path 2>&1
        echo "❌ Failed, possibly due to: network issues / repository not found / path already exists / insufficient permissions" >> $log_file_path 2>&1
        
        # Optional: output detailed error log (helps diagnose issues)
        echo "📝 Detailed error log: " >> $log_file_path 2>&1
        $GIT_CMD 2>&1  # Re-execute and output full error info (2>&1 redirects stderr to stdout)
        
        exit $ERROR_CODE  # Exit with non-zero code to notify caller of failure
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

# python ../mk_to_wscript_static.py ../librclcpp-lifecycle.mk wscript

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
    # BSP-related configuration checks can be performed here
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    # rclcpp-lifecycle target (static library)
    # Compile and link static library
    bld.stlib(
        target="rclcpp-lifecycle",
        source=[
            "init.c",
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/src/lifecycle_node.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/src/managed_entity.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/src/mutex_map.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/src/node_interfaces/lifecycle_node_interface.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/src/state.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/src/transition.cpp"
        ],
        includes=[
            "../../../code/src/rclcpp-16.0.8/rclcpp_lifecycle/include",
            "../../../../rosidl/code/src/rosidl/rosidl_runtime_c/include",
            "../../../../rosidl/code/src/rosidl/rosidl_typesupport_interface/include",
            "../../../../rcl/code/src/rcl-5.3.7/rcl_lifecycle/include",
            "../../../../lifecycle_msgs/code/src/lifecycle_msgs/rosidl_generator_cpp",
            "../../../../rcutils/code/src/rcutils-5.1.5/include",
            "../../../../rcpputils/code/src/rcpputils-2.4.2/include",
            "../../../../rmw/code/src/rmw-6.1.1/rmw/include",
            "../../../../class_loader/code/src/class_loader-2.2.0/include",
            "../../../../rcl/code/src/rcl-5.3.7/rcl/include",
            "../../../../rosidl/code/src/rosidl/rosidl_runtime_cpp/include",
            "../../../../lifecycle_msgs/code/src/lifecycle_msgs/rosidl_generator_c",
            "../../../../rcl_interfaces/code/src/rcl_interfaces/rosidl_generator_cpp",
            "../../../../rclcpp/code/src/rclcpp-16.0.8/rclcpp/include",
            "../../../../builtin_interfaces/code/src/builtin_interfaces/rosidl_generator_cpp",
            "../../../../statistics_collector/code/src/libstatistics_collector-1.3.1/include",
            "../../../../statistics_msgs/code/src/statistics_msgs/rosidl_generator_cpp",
            "../../../../rosgraph_msgs/code/src/rosgraph_msgs/rosidl_generator_cpp",
            "../../../../statistics_collector/code/src/include"
        ],
        defines=[
            "RMW_IMPLEMENTATION=rmw_fastrtps_cpp",
        ],
        cxxflags="-DRTEMS -fpermissive -fexceptions -Wno-psabi -Wno-register -fPIC",
        linkflags="-static",
        lib=[
            "rosidl_runtime_c",
            "rcl-lifecyle",
            "class-loader",
            "rcpputils",
            "rcutils",
            "rmw",
            "rcl",
            "lifecycle_msgs__rosidl_generator_c",
            "rclcpp",
            "statistics_collector",
            "stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c"
        ],
    )
' > wscript

fi
exit 0