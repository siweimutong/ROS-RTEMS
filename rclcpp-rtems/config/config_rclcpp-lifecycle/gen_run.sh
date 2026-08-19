#!/bin/bash

target_dir="./tmp"
build_modle=$1
flag=0

root_path=$2
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_rclcpp.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"
fi

if [[ ! -f "$log_file_path" ]]; then
    touch "$log_file_path"
fi

if [ -d "${target_dir}" ]; then
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

    if eval $GIT_CMD; then
        echo "submodule rtems_waf added successfully" >> $log_file_path 2>&1
    else
        ERROR_CODE=$?
        echo "submodule rtems_waf add failed! exit code: ${ERROR_CODE}" >> $log_file_path 2>&1
        exit $ERROR_CODE
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
    ctx.load("compiler_cxx")

def bsp_configure(conf, arch_bsp):
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    bld.stlib(
        target="rclcpp-lifecycle",
        source=[
            "init.c",
            "../../../rclcpp_lifecycle/src/lifecycle_node.cpp",
            "../../../rclcpp_lifecycle/src/managed_entity.cpp",
            "../../../rclcpp_lifecycle/src/mutex_map.cpp",
            "../../../rclcpp_lifecycle/src/node_interfaces/lifecycle_node_interface.cpp",
            "../../../rclcpp_lifecycle/src/state.cpp",
            "../../../rclcpp_lifecycle/src/transition.cpp"
        ],
        includes=[
            "../../../rclcpp_lifecycle/include",
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
            "../../../rclcpp/include",
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
