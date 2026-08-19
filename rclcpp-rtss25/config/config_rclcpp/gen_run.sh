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

    # rclcpp target (static library)
    bld.stlib(
        target="rclcpp",
        source=[
            "init.c",
            "../../../rclcpp/src/rclcpp/any_executable.cpp",
            "../../../rclcpp/src/rclcpp/callback_group.cpp",
            "../../../rclcpp/src/rclcpp/client.cpp",
            "../../../rclcpp/src/rclcpp/clock.cpp",
            "../../../rclcpp/src/rclcpp/cond.cpp",
            "../../../rclcpp/src/rclcpp/context.cpp",
            "../../../rclcpp/src/rclcpp/contexts/default_context.cpp",
            "../../../rclcpp/src/rclcpp/detail/add_guard_condition_to_rcl_wait_set.cpp",
            "../../../rclcpp/src/rclcpp/detail/resolve_parameter_overrides.cpp",
            "../../../rclcpp/src/rclcpp/detail/rmw_implementation_specific_payload.cpp",
            "../../../rclcpp/src/rclcpp/detail/rmw_implementation_specific_publisher_payload.cpp",
            "../../../rclcpp/src/rclcpp/detail/rmw_implementation_specific_subscription_payload.cpp",
            "../../../rclcpp/src/rclcpp/detail/utilities.cpp",
            "../../../rclcpp/src/rclcpp/duration.cpp",
            "../../../rclcpp/src/rclcpp/event.cpp",
            "../../../rclcpp/src/rclcpp/exceptions/exceptions.cpp",
            "../../../rclcpp/src/rclcpp/executable_list.cpp",
            "../../../rclcpp/src/rclcpp/executor.cpp",
            "../../../rclcpp/src/rclcpp/executors/multi_threaded_executor.cpp",
            "../../../rclcpp/src/rclcpp/executors/no_executor.cpp",
            "../../../rclcpp/src/rclcpp/executors/single_threaded_executor.cpp",
            "../../../rclcpp/src/rclcpp/executors/static_executor_entities_collector.cpp",
            "../../../rclcpp/src/rclcpp/executors/static_single_threaded_executor.cpp",
            "../../../rclcpp/src/rclcpp/executors.cpp",
            "../../../rclcpp/src/rclcpp/expand_topic_or_service_name.cpp",
            "../../../rclcpp/src/rclcpp/future_return_code.cpp",
            "../../../rclcpp/src/rclcpp/generic_publisher.cpp",
            "../../../rclcpp/src/rclcpp/generic_subscription.cpp",
            "../../../rclcpp/src/rclcpp/graph_listener.cpp",
            "../../../rclcpp/src/rclcpp/guard_condition.cpp",
            "../../../rclcpp/src/rclcpp/init_options.cpp",
            "../../../rclcpp/src/rclcpp/intra_process_manager.cpp",
            "../../../rclcpp/src/rclcpp/logger.cpp",
            "../../../rclcpp/src/rclcpp/logging_mutex.cpp",
            "../../../rclcpp/src/rclcpp/memory_strategies.cpp",
            "../../../rclcpp/src/rclcpp/memory_strategy.cpp",
            "../../../rclcpp/src/rclcpp/message_info.cpp",
            "../../../rclcpp/src/rclcpp/network_flow_endpoint.cpp",
            "../../../rclcpp/src/rclcpp/node.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_base.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_clock.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_graph.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_logging.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_parameters.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_services.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_timers.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_time_source.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_topics.cpp",
            "../../../rclcpp/src/rclcpp/node_interfaces/node_waitables.cpp",
            "../../../rclcpp/src/rclcpp/node_options.cpp",
            "../../../rclcpp/src/rclcpp/parameter.cpp",
            "../../../rclcpp/src/rclcpp/parameter_client.cpp",
            "../../../rclcpp/src/rclcpp/parameter_events_filter.cpp",
            "../../../rclcpp/src/rclcpp/parameter_event_handler.cpp",
            "../../../rclcpp/src/rclcpp/parameter_map.cpp",
            "../../../rclcpp/src/rclcpp/parameter_service.cpp",
            "../../../rclcpp/src/rclcpp/parameter_value.cpp",
            "../../../rclcpp/src/rclcpp/publisher_base.cpp",
            "../../../rclcpp/src/rclcpp/qos.cpp",
            "../../../rclcpp/src/rclcpp/qos_event.cpp",
            "../../../rclcpp/src/rclcpp/qos_overriding_options.cpp",
            "../../../rclcpp/src/rclcpp/sched_base.cpp",
            "../../../rclcpp/src/rclcpp/serialization.cpp",
            "../../../rclcpp/src/rclcpp/serialized_message.cpp",
            "../../../rclcpp/src/rclcpp/service.cpp",
            "../../../rclcpp/src/rclcpp/signal_handler.cpp",
            "../../../rclcpp/src/rclcpp/subscription_base.cpp",
            "../../../rclcpp/src/rclcpp/subscription_intra_process_base.cpp",
            "../../../rclcpp/src/rclcpp/time.cpp",
            "../../../rclcpp/src/rclcpp/timer.cpp",
            "../../../rclcpp/src/rclcpp/time_source.cpp",
            "../../../rclcpp/src/rclcpp/typesupport_helpers.cpp",
            "../../../rclcpp/src/rclcpp/type_support.cpp",
            "../../../rclcpp/src/rclcpp/utilities.cpp",
            "../../../rclcpp/src/rclcpp/waitable.cpp",
            "../../../rclcpp/src/rclcpp/wait_set_policies/detail/write_preferring_read_write_lock.cpp",
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
            "../../../rclcpp/include"
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
