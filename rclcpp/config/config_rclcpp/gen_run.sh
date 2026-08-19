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

# Checkfile是nonot found,not found则Create
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
        # CaptureFailed,OutputErrorinfo并退出
        ERROR_CODE=$?  # Get command exit code
        echo "❌ Submodule rtems_waf add failed! Exit code: ${ERROR_CODE}" >> $log_file_path 2>&1
        echo "❌ Failed原becausecan能:网络问题/仓库not found/pathalready存在/权限不足" >> $log_file_path 2>&1
        
        # Optional:Output详细Error日志(Help diagnose issues)
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

# python ../mk_to_wscript_static.py ../librclcpp.mk wscript

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

    # rclcpp target (static library)
    # Compile and link static library
    bld.stlib(
        target="rclcpp",
        source=[
            "init.c",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/any_executable.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/callback_group.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/client.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/clock.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/context.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/contexts/default_context.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/add_guard_condition_to_rcl_wait_set.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/resolve_parameter_overrides.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/rmw_implementation_specific_payload.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/rmw_implementation_specific_publisher_payload.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/rmw_implementation_specific_subscription_payload.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/utilities.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/duration.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/event.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/exceptions/exceptions.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executable_list.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executor.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/multi_threaded_executor.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/single_threaded_executor.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/static_executor_entities_collector.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/static_single_threaded_executor.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/expand_topic_or_service_name.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/future_return_code.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/generic_publisher.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/generic_subscription.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/graph_listener.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/guard_condition.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/init_options.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/intra_process_manager.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/logger.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/logging_mutex.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/memory_strategies.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/memory_strategy.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/message_info.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/network_flow_endpoint.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_base.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_clock.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_graph.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_logging.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_parameters.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_services.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_timers.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_time_source.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_topics.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_waitables.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_options.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_client.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_events_filter.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_event_handler.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_map.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_service.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_value.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/publisher_base.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/qos.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/qos_event.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/qos_overriding_options.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/serialization.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/serialized_message.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/service.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/signal_handler.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/subscription_base.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/subscription_intra_process_base.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/time.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/timer.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/time_source.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/typesupport_helpers.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/type_support.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/utilities.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/waitable.cpp",
            "../../../code/src/rclcpp-16.0.8/rclcpp/src/rclcpp/wait_set_policies/detail/write_preferring_read_write_lock.cpp"
        ],
        includes=[
            "../../../../rmw/code/src/rmw-6.1.1/rmw/include",
            "../../../../rcutils/code/src/rcutils-5.1.5/include",
            "../../../../rcl/code/src/rcl-5.3.7/rcl_yaml_param_parser/include",
            "../../../../rcpputils/code/src/rcpputils-2.4.2/include",
            "../../../../logging/code/src/rcl_logging-2.3.1/rcl_logging_interface/include",
            "../../../../rcl/code/src/rcl-5.3.7/rcl/include",
            "../../../../rosidl/code/src/rosidl/rosidl_runtime_c/include",
            "../../../../rosidl/code/src/rosidl/rosidl_typesupport_interface/include",
            "../../../../builtin_interfaces/code/src/builtin_interfaces/rosidl_generator_cpp",
            "../../../../rosidl/code/src/rosidl/rosidl_runtime_cpp/include",
            "../../../../rcl_interfaces/code/src/rcl_interfaces/rosidl_generator_cpp",
            "../../../../statistics_msgs/code/src/statistics_msgs/rosidl_generator_cpp",
            "../../../../rosgraph_msgs/code/src/rosgraph_msgs/rosidl_generator_cpp",
            "../../../../statistics_collector/code/src/include",
            "../../../../ament_index_cpp/code/src/ament_index_cpp/include",
            "../../../code/src/rclcpp-16.0.8/rclcpp/include"
        ],
        defines=[
            "RMW_IMPLEMENTATION=rmw_fastrtps_cpp",
        ],
        cxxflags="-DRTEMS -fpermissive -fexceptions -Wno-psabi -Wno-register -g -fPIC",
        linkflags="-static",
        lib=[
            "rmw",
            "rcpputils",
            "rcutils",
            "rcl-yaml-param-parser",
            "rcl",
            "builtin_interfaces__rosidl_typesupport_cpp",
            "rcl_interfaces__rosidl_typesupport_cpp",
            "rosgraph_msgs__rosidl_typesupport_cpp",
            "statistics_msgs__rosidl_typesupport_cpp",
            "statistics_collector",
            "ament_index_cpp",
            "builtin_interfaces__rosidl_generator_c",
            "builtin_interfaces__rosidl_typesupport_c",
            "builtin_interfaces__rosidl_typesupport_fastrtps_c",
            "builtin_interfaces__rosidl_typesupport_fastrtps_cpp",
            "builtin_interfaces__rosidl_typesupport_introspection_c",
            "builtin_interfaces__rosidl_typesupport_introspection_cpp",
            "rcl_interfaces__rosidl_generator_c",
            "rcl_interfaces__rosidl_typesupport_c",
            "rcl_interfaces__rosidl_typesupport_fastrtps_c",
            "rcl_interfaces__rosidl_typesupport_fastrtps_cpp",
            "rcl_interfaces__rosidl_typesupport_introspection_c",
            "rcl_interfaces__rosidl_typesupport_introspection_cpp",
            "rosgraph_msgs__rosidl_generator_c",
            "rosgraph_msgs__rosidl_typesupport_c",
            "rosgraph_msgs__rosidl_typesupport_fastrtps_c",
            "rosgraph_msgs__rosidl_typesupport_fastrtps_cpp",
            "rosgraph_msgs__rosidl_typesupport_introspection_c",
            "rosgraph_msgs__rosidl_typesupport_introspection_cpp",
            "rosidl_typesupport_c",
            "rosidl_typesupport_cpp",
            "statistics_msgs__rosidl_generator_c",
            "statistics_msgs__rosidl_typesupport_c",
            "statistics_msgs__rosidl_typesupport_fastrtps_c",
            "statistics_msgs__rosidl_typesupport_fastrtps_cpp",
            "statistics_msgs__rosidl_typesupport_introspection_c",
            "statistics_msgs__rosidl_typesupport_introspection_cpp",
            "stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c"
        ],
    )
' > wscript

fi
exit 0