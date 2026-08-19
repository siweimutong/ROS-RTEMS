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
# 文   件   名: librclcpp-lifecycle.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2024 年 05 月 23 日
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
LOCAL_TARGET_NAME := librclcpp-lifecycle.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/rclcpp-16.0.8/rclcpp_lifecycle/src/lifecycle_node.cpp \
src/rclcpp-16.0.8/rclcpp_lifecycle/src/managed_entity.cpp \
src/rclcpp-16.0.8/rclcpp_lifecycle/src/mutex_map.cpp \
src/rclcpp-16.0.8/rclcpp_lifecycle/src/node_interfaces/lifecycle_node_interface.cpp \
src/rclcpp-16.0.8/rclcpp_lifecycle/src/state.cpp \
src/rclcpp-16.0.8/rclcpp_lifecycle/src/transition.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/rclcpp-16.0.8/rclcpp_lifecycle/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl_lifecycle/include" \
-I"$(WORKSPACE_lifecycle_msgs)/src/lifecycle_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rcpputils)/src/rcpputils-2.4.2/include" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"$(WORKSPACE_class_loader)/src/class_loader-2.2.0/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include" \
-I"$(WORKSPACE_lifecycle_msgs)/src/lifecycle_msgs/rosidl_generator_c" \
-I"$(WORKSPACE_rcl_interfaces)/src/rcl_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_rclcpp)/src/rclcpp-16.0.8/rclcpp/include" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_statistics_collector)/src/libstatistics_collector-1.3.1/include" \
-I"$(WORKSPACE_statistics_msgs)/src/statistics_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_rosgraph_msgs)/src/rosgraph_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_libstatistics_collector)/src/include"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS := 
LOCAL_CXXFLAGS := -fpermissive -fexceptions -frtti -std=c++17 -Wno-register
LOCAL_LINKFLAGS := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB :=  \
-lrosidl_runtime_c \
-lrcl-lifecyle \
-lclass-loader \
-lrcpputils \
-lrcutils \
-lrmw \
-lrcl \
-llifecycle_msgs__rosidl_generator_c \
-lrclcpp \
-llibstatistics_collector
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_RCL)/Release/strip" \
-L"$(WORKSPACE_class_loader)/Release/strip" \
-L"$(WORKSPACE_rcpputils)/Release/strip" \
-L"$(WORKSPACE_rcutils)/Release/strip" \
-L"$(WORKSPACE_rmw)/Release/strip" \
-L"$(WORKSPACE_lifecycle_msgs)/Release/strip" \
-L"$(WORKSPACE_rclcpp)/Release/strip" \
-L"$(WORKSPACE_statistics_collector)/Release/strip" \
-L"$(WORKSPACE_libstatistics_collector)/Release/strip"

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
$(WORKSPACE_rosidl)/Release/strip/librosidl_runtime_c.so \
$(WORKSPACE_RCL)/Release/strip/librcl-lifecyle.so \
$(WORKSPACE_class_loader)/Release/strip/libclass-loader.so \
$(WORKSPACE_rcpputils)/Release/strip/librcpputils.so \
$(WORKSPACE_rcutils)/Release/strip/librcutils.so \
$(WORKSPACE_rmw)/Release/strip/librmw.so \
$(WORKSPACE_RCL)/Release/strip/librcl.so \
$(WORKSPACE_lifecycle_msgs)/Release/strip/liblifecycle_msgs__rosidl_generator_c.so \
$(WORKSPACE_rclcpp)/Release/strip/librclcpp.so \
$(WORKSPACE_libstatistics_collector)/Release/strip/liblibstatistics_collector.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
