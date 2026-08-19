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
# 文   件   名: lifecycle_service_client.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2024 年 07 月 31 日
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
LOCAL_TARGET_NAME := lifecycle_service_client

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/lifecycle/src/lifecycle_service_client.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"$(WORKSPACE_lifecycle_msgs)/src/lifecycle_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rclcpp)/src/rclcpp-16.0.8/rclcpp/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_rcpputils)/src/rcpputils-2.4.2/include" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_rcl_interfaces)/src/rcl_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_statistics_msgs)/src/statistics_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_libstatistics_collector)/src/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl_yaml_param_parser/include" \
-I"$(WORKSPACE_rclcpp)/src/rclcpp-16.0.8/rclcpp_lifecycle/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl_lifecycle/include" \
-I"$(WORKSPACE_lifecycle_msgs)/src/lifecycle_msgs/rosidl_generator_c" \
-I"$(WORKSPACE_std_msgs)/src/std_msgs/rosidl_generator_cpp"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS := -std=c++17 -fexceptions -frtti
LOCAL_LINKFLAGS :=

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB :=  \
-lrclcpp \
-lstatistics_msgs__rosidl_generator_c \
-lstatistics_msgs__rosidl_typesupport_c \
-lstatistics_msgs__rosidl_typesupport_cpp \
-lstatistics_msgs__rosidl_typesupport_fastrtps_c \
-lstatistics_msgs__rosidl_typesupport_fastrtps_cpp \
-lstatistics_msgs__rosidl_typesupport_introspection_c \
-lstatistics_msgs__rosidl_typesupport_introspection_cpp \
-lrclcpp-lifecycle \
-llifecycle_msgs__rosidl_generator_c \
-llifecycle_msgs__rosidl_typesupport_c \
-llifecycle_msgs__rosidl_typesupport_cpp \
-llifecycle_msgs__rosidl_typesupport_fastrtps_c \
-llifecycle_msgs__rosidl_typesupport_fastrtps_cpp \
-llifecycle_msgs__rosidl_typesupport_introspection_c \
-llifecycle_msgs__rosidl_typesupport_introspection_cpp \
-lstd_msgs__rosidl_generator_c \
-lstd_msgs__rosidl_typesupport_c \
-lstd_msgs__rosidl_typesupport_cpp \
-lstd_msgs__rosidl_typesupport_fastrtps_c \
-lstd_msgs__rosidl_typesupport_fastrtps_cpp \
-lstd_msgs__rosidl_typesupport_introspection_c \
-lstd_msgs__rosidl_typesupport_introspection_cpp
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_rclcpp)/Release/strip" \
-L"$(WORKSPACE_statistics_msgs)/Release/strip" \
-L"$(WORKSPACE_lifecycle_msgs)/Release/strip" \
-L"$(WORKSPACE_std_msgs)/Release/strip"

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
LOCAL_PRE_LINK_CMD   := 
LOCAL_POST_LINK_CMD  := 
LOCAL_PRE_STRIP_CMD  := 
LOCAL_POST_STRIP_CMD := 

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET := \
$(WORKSPACE_rclcpp)/Release/strip/librclcpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_generator_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_rclcpp)/Release/strip/librclcpp-lifecycle.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_generator_c.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_generator_c.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_std_msgs)/Release/strip/libstd_msgs__rosidl_typesupport_introspection_cpp.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
