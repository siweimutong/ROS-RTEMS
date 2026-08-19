#!/bin/bash

target_dir="./tmp"
build_modle=$1
flag=0

root_path=$2
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_rmw_fastrtps.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"  # -pis key: creates build even if no subdirectories exist under root_path
fi

# Check if the log file is missing; create it if not found
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
        # Capture the failure, output the error message, and exit
        ERROR_CODE=$?  # Get command exit code
        echo "❌ Submodule rtems_waf add failed! Exit code: ${ERROR_CODE}" >> $log_file_path 2>&1
        echo "❌ Failed, possibly because: network issues / repository not found / path already exists / insufficient permissions" >> $log_file_path 2>&1
        
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

# python ../mk_to_wscript_static.py ../librmw_fastrtps_dynamic_cpp.mk wscript

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
    # BSP-specific configuration checks can be added here
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    # rmw_fastrtps_dynamic_cpp target (static library)
    # Compile and link static library
    bld.stlib(
        target="rmw_fastrtps_dynamic_cpp",
        source=[
            "init.c",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/client_service_common.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/get_client.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/get_participant.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/get_publisher.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/get_service.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/get_subscriber.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/identifier.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/init_rmw_context_impl.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/publisher.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_client.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_compare_gids_equal.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_count.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_event.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_features.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_get_endpoint_network_flow.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_get_gid_for_publisher.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_get_implementation_identifier.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_get_serialization_format.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_get_topic_endpoint_info.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_guard_condition.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_init.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_logging.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_node.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_node_info_and_types.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_node_names.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_publish.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_publisher.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_qos.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_request.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_response.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_serialize.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_service.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_service_names_and_types.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_service_server_is_available.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_subscription.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_take.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_topic_names_and_types.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_trigger_guard_condition.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_wait.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/rmw_wait_set.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/serialization_format.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/subscription.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/type_support_common.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/type_support_proxy.cpp",
            "../../../code/src/rmw_fastrtps_dynamic_cpp/src/type_support_registry.cpp"
        ],
        includes=[
            "../../../code/src/rmw_fastrtps_dynamic_cpp/include",
            "../../../../rmw/code/src/rmw-6.1.1/rmw/include",
            "../../../../rcutils/code/src/rcutils-5.1.5/include",
            "../../../../rcpputils/code/src/rcpputils-2.4.2/include",
            "../../../code/src/rmw_fastrtps_shared_cpp/include",
            "../../../../DDS/Fast-DDS/code/include",
            "../../../../DDS/Fast-DDS/code/build/include",
            "../../../../DDS/Fast-CDR/code/include",
            "../../../../DDS/Fast-CDR/code/build/include",
            "../../../../rosidl/code/src/rosidl/rosidl_runtime_c/include",
            "../../../../rosidl/code/src/rosidl/rosidl_typesupport_interface/include",
            "../../../../rosidl/code/src/rosidl/rosidl_typesupport_introspection_cpp/include",
            "../../../../rosidl/code/src/rosidl/rosidl_typesupport_introspection_c/include",
            "../../../../rosidl/code/src/rosidl/rosidl_runtime_cpp/include",
            "../../../../rosidl/code/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_c/include",
            "../../../../rosidl/code/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_cpp/include",
            "../../../../rmw_dds_common/code/src/rmw_dds_common_src/rmw_dds_common/include",
            "../../../../rmw_dds_common/code/src/rmw_dds_common/rosidl_generator_cpp"
        ],
        defines=[
            "RMW_IMPLEMENTATION=rmw_fastrtps_cpp",
        ],
        cxxflags="-DRTEMS -fpermissive -fexceptions -Wno-psabi -Wno-register -g -fPIC",
        linkflags="-static",
        lib=[
            "rmw",
            "rcutils",
            "rcpputils",
            "rmw_fastrtps_shared_cpp",
            "fastrtps",
            "fastcdr",
            "rosidl_runtime_c",
            "rosidl_typesupport_introspection_cpp",
            "rosidl_typesupport_introspection_c",
            "rosidl_typesupport_fastrtps_c",
            "rosidl_typesupport_fastrtps_cpp",
            "rmw_dds_common",
            "rmw_dds_common__rosidl_generator_c",
            "rmw_dds_common__rosidl_typesupport_c",
            "rmw_dds_common__rosidl_typesupport_cpp",
            "rmw_dds_common__rosidl_typesupport_fastrtps_c",
            "rmw_dds_common__rosidl_typesupport_fastrtps_cpp",
            "rmw_dds_common__rosidl_typesupport_introspection_c",
            "rmw_dds_common__rosidl_typesupport_introspection_cpp",
            "stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c"
        ],
    )
' > wscript
fi
exit 0