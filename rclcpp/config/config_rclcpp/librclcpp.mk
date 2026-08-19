#*********************************************************************************************************
#
#                                    中国软件开源组织
#
#                                   嵌入式实时操作系统
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------文件信息--------------------------------------------------------------------------------
#
# 文   件   名: librclcpp.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2024 年 05 月 12 日
#
# 描        述: 本文件由 RealEvo-IDE 生成，用于配置 Makefile 功能，请勿手动修改
#*********************************************************************************************************

#*********************************************************************************************************
# Clear setting
#*********************************************************************************************************
include $(CLEAR_VARS_MK)

#*********************************************************************************************************
# Target
#*********************************************************************************************************
LOCAL_TARGET_NAME := librclcpp.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/any_executable.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/callback_group.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/client.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/clock.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/context.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/contexts/default_context.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/add_guard_condition_to_rcl_wait_set.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/resolve_parameter_overrides.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/rmw_implementation_specific_payload.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/rmw_implementation_specific_publisher_payload.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/rmw_implementation_specific_subscription_payload.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/detail/utilities.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/duration.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/event.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/exceptions/exceptions.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executable_list.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executor.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/multi_threaded_executor.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/single_threaded_executor.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/static_executor_entities_collector.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors/static_single_threaded_executor.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/executors.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/expand_topic_or_service_name.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/future_return_code.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/generic_publisher.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/generic_subscription.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/graph_listener.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/guard_condition.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/init_options.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/intra_process_manager.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/logger.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/logging_mutex.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/memory_strategies.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/memory_strategy.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/message_info.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/network_flow_endpoint.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_base.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_clock.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_graph.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_logging.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_parameters.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_services.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_timers.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_time_source.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_topics.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_interfaces/node_waitables.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/node_options.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_client.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_events_filter.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_event_handler.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_map.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_service.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/parameter_value.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/publisher_base.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/qos.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/qos_event.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/qos_overriding_options.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/serialization.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/serialized_message.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/service.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/signal_handler.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/subscription_base.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/subscription_intra_process_base.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/time.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/timer.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/time_source.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/typesupport_helpers.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/type_support.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/utilities.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/waitable.cpp \
src/rclcpp-16.0.8/rclcpp/src/rclcpp/wait_set_policies/detail/write_preferring_read_write_lock.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl_yaml_param_parser/include" \
-I"$(WORKSPACE_rcpputils)/src/rcpputils-2.4.2/include" \
-I"$(WORKSPACE_logging)/src/rcl_logging-2.3.1/rcl_logging_interface/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include" \
-I"$(WORKSPACE_rcl_interfaces)/src/rcl_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_statistics_msgs)/src/statistics_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_rosgraph_msgs)/src/rosgraph_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_libstatistics_collector)/src/include" \
-I"$(WORKSPACE_ament_index_cpp)/src/ament_index_cpp/include" \
-I"./src/rclcpp-16.0.8/rclcpp/include"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS := 
LOCAL_CXXFLAGS := -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g
LOCAL_LINKFLAGS := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB :=  \
-lament_index_cpp \
-lbuiltin_interfaces__rosidl_generator_c \
-lbuiltin_interfaces__rosidl_typesupport_c \
-lbuiltin_interfaces__rosidl_typesupport_cpp \
-lbuiltin_interfaces__rosidl_typesupport_fastrtps_c \
-lbuiltin_interfaces__rosidl_typesupport_fastrtps_cpp \
-lbuiltin_interfaces__rosidl_typesupport_introspection_c \
-lbuiltin_interfaces__rosidl_typesupport_introspection_cpp \
-llibstatistics_collector \
-lrcl \
-lrcl_interfaces__rosidl_generator_c \
-lrcl_interfaces__rosidl_typesupport_c \
-lrcl_interfaces__rosidl_typesupport_cpp \
-lrcl_interfaces__rosidl_typesupport_fastrtps_c \
-lrcl_interfaces__rosidl_typesupport_fastrtps_cpp \
-lrcl_interfaces__rosidl_typesupport_introspection_c \
-lrcl_interfaces__rosidl_typesupport_introspection_cpp \
-lrcl-yaml-param-parser \
-lrcpputils \
-lrcutils \
-lrmw \
-lrosgraph_msgs__rosidl_generator_c \
-lrosgraph_msgs__rosidl_typesupport_c \
-lrosgraph_msgs__rosidl_typesupport_cpp \
-lrosgraph_msgs__rosidl_typesupport_fastrtps_c \
-lrosgraph_msgs__rosidl_typesupport_fastrtps_cpp \
-lrosgraph_msgs__rosidl_typesupport_introspection_c \
-lrosgraph_msgs__rosidl_typesupport_introspection_cpp \
-lrosidl_typesupport_c \
-lrosidl_typesupport_cpp \
-lstatistics_msgs__rosidl_generator_c \
-lstatistics_msgs__rosidl_typesupport_c \
-lstatistics_msgs__rosidl_typesupport_cpp \
-lstatistics_msgs__rosidl_typesupport_fastrtps_c \
-lstatistics_msgs__rosidl_typesupport_fastrtps_cpp \
-lstatistics_msgs__rosidl_typesupport_introspection_c \
-lstatistics_msgs__rosidl_typesupport_introspection_cpp
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_RCL)/Release/strip" \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_rmw)/Release/strip" \
-L"$(WORKSPACE_rcutils)/Release/strip" \
-L"$(WORKSPACE_rcpputils)/Release/strip" \
-L"$(WORKSPACE_statistics_collector)/Release/strip" \
-L"$(WORKSPACE_logging)/Release/strip" \
-L"$(WORKSPACE_libstatistics_collector)/Release/strip" \
-L"$(WORKSPACE_builtin_interfaces)/Release/strip" \
-L"$(WORKSPACE_rcl_interfaces)/Release/strip" \
-L"$(WORKSPACE_rosgraph_msgs)/Release" \
-L"$(WORKSPACE_statistics_msgs)/Release/strip" \
-L"$(WORKSPACE_libstatistics_collector)/Release" \
-L"$(WORKSPACE_ament_index_cpp)/Release/strip" \
-L"$(WORKSPACE_rcl_interfaces)/Release" \
-L"$(WORKSPACE_rosgraph_msgs)/Release/strip"

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX := yes
LOCAL_USE_CXX_EXCEPT := no

#*********************************************************************************************************
# Linker specific
#*********************************************************************************************************
LOCAL_NO_UNDEF_SYM := no

#*********************************************************************************************************
# Code coverage config
#*********************************************************************************************************
LOCAL_USE_GCOV := no

#*********************************************************************************************************
# OpenMP config
#*********************************************************************************************************
LOCAL_USE_OMP := no

#*********************************************************************************************************
# Use short command for link and ar
#*********************************************************************************************************
LOCAL_USE_SHORT_CMD := no

#*********************************************************************************************************
# User link command
#*********************************************************************************************************
LOCAL_PRE_LINK_CMD := 
LOCAL_POST_LINK_CMD := 
LOCAL_PRE_STRIP_CMD := 
LOCAL_POST_STRIP_CMD := 

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET :=  \
$(WORKSPACE_rmw)/Release/strip/librmw.so \
$(WORKSPACE_rcpputils)/Release/strip/librcpputils.so \
$(WORKSPACE_rcutils)/Release/strip/librcutils.so \
$(WORKSPACE_RCL)/Release/strip/librcl-yaml-param-parser.so \
$(WORKSPACE_RCL)/Release/strip/librcl.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_cpp.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_cpp.so \
$(WORKSPACE_rosgraph_msgs)/Release/librosgraph_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_libstatistics_collector)/Release/strip/liblibstatistics_collector.so \
$(WORKSPACE_ament_index_cpp)/Release/strip/libament_index_cpp.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_generator_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_generator_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_rcl_interfaces)/Release/librcl_interfaces__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_rosgraph_msgs)/Release/strip/librosgraph_msgs__rosidl_generator_c.so \
$(WORKSPACE_rosgraph_msgs)/Release/strip/librosgraph_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_rosgraph_msgs)/Release/strip/librosgraph_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_rosgraph_msgs)/Release/strip/librosgraph_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_rosgraph_msgs)/Release/strip/librosgraph_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_rosgraph_msgs)/Release/strip/librosgraph_msgs__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_c.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_generator_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_introspection_cpp.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
