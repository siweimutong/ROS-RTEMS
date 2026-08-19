#!/bin/bash

target_dir="./tmp"
build_modle=$1
flag=0

root_path=$2
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_rosidl.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"  # -pis key: creates build even if no subdirectories exist under root_path
fi

# Check if the file does not exist; create it if so
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
        echo "❌ Failed, possible causes: network issue / repository not found / path already exists / insufficient permissions" >> $log_file_path 2>&1
        
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

# python ../mk_to_wscript_static.py ../librosidl_typesupport_cpp.mk wscript

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
    # Add BSP-related configuration checks here
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    # rosidl_typesupport_cpp target (static library)
    # Compile and link static library
    bld.stlib(
        target="rosidl_typesupport_cpp",
        source=[
            "init.c",
            "../../../code/src/rosidl_typesupport/rosidl_typesupport_cpp/src/get_func_pointer_by_name.cpp",
            "../../../code/src/rosidl_typesupport/rosidl_typesupport_cpp/src/identifier.cpp",
            "../../../code/src/rosidl_typesupport/rosidl_typesupport_cpp/src/message_type_support_dispatch.cpp",
            "../../../code/src/rosidl_typesupport/rosidl_typesupport_cpp/src/service_type_support_dispatch.cpp"
        ],
        includes=[
            "../../../code/src/rosidl_typesupport/rosidl_typesupport_cpp/include",
            "../../../code/src/rosidl/rosidl_runtime_c/include",
            "../../../code/src/rosidl/rosidl_typesupport_interface/include",
            "../../../../rcpputils/code/src/rcpputils-2.4.2/include",
            "../../../../rcutils/code/src/rcutils-5.1.5/include",
            "../../../code/src/rosidl_typesupport/rosidl_typesupport_c/include",
            "../../../../rmw_dds_common/code/src/rmw_dds_common/rosidl_typesupport_fastrtps_cpp/rmw_dds_common/msg/detail",
            "../../../../rmw_dds_common/code/src/rmw_dds_common/rosidl_typesupport_fastrtps_cpp",
            "../../../../rmw_dds_common/code/src/rmw_dds_common/rosidl_generator_cpp",
            "../../../code/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_cpp/include",
            "../../../code/src/rosidl/rosidl_runtime_cpp/include",
            "../../../../DDS/Fast-CDR/code/include",
            "../../../../DDS/Fast-CDR/code/build/include",
            "../../../../rcl_interfaces/code/src/rcl_interfaces/rosidl_typesupport_fastrtps_cpp",
            "../../../../rcl_interfaces/code/src/rcl_interfaces/rosidl_generator_cpp",
            "../../../../rmw/code/src/rmw-6.1.1/rmw/include",
            "../../../../builtin_interfaces/code/src/builtin_interfaces/rosidl_generator_cpp",
            "../../../../std_msgs/code/src/std_msgs/rosidl_typesupport_fastrtps_cpp",
            "../../../../std_msgs/code/src/std_msgs/rosidl_generator_cpp",
            "../../../../std_msgs/code/src/std_msgs/rosidl_typesupport_introspection_cpp",
            "../../../code/src/rosidl/rosidl_typesupport_introspection_cpp/include"
        ],
        defines=[
            "RMW_IMPLEMENTATION=rmw_fastrtps_cpp",
        ],
        cxxflags="-DRTEMS -fexceptions -fPIC",
        linkflags="-static",
        lib=["rosidl_runtime_c", "rcpputils", "rcutils", "rosidl_typesupport_c", "stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c"],
    )


' > wscript
fi
exit 0