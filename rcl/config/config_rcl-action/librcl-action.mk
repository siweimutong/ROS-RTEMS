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
# 文   件   名: librcl-action.mk
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
LOCAL_TARGET_NAME := librcl-action.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/rcl-5.3.7/rcl_action/src/rcl_action/action_client.c \
src/rcl-5.3.7/rcl_action/src/rcl_action/action_server.c \
src/rcl-5.3.7/rcl_action/src/rcl_action/goal_handle.c \
src/rcl-5.3.7/rcl_action/src/rcl_action/goal_state_machine.c \
src/rcl-5.3.7/rcl_action/src/rcl_action/graph.c \
src/rcl-5.3.7/rcl_action/src/rcl_action/names.c \
src/rcl-5.3.7/rcl_action/src/rcl_action/types.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/rcl-5.3.7/rcl_action/include" \
-I"./src/rcl-5.3.7/rcl/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"./src/rcl-5.3.7/rcl_yaml_param_parser/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_action_msgs)/src/action_msgs/rosidl_generator_c" \
-I"$(WORKSPACE_unique_identifier_msgs)/src/unique_identifier_msgs/rosidl_generator_c" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_c"

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
-lrcl \
-lrmw \
-lrcutils \
-lrosidl_runtime_c \
-laction_msgs__rosidl_generator_c \
-lunique_identifier_msgs__rosidl_generator_c \
-lbuiltin_interfaces__rosidl_generator_c
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_RCL)/Release/strip" \
-L"$(WORKSPACE_rcutils)/Release/strip" \
-L"$(WORKSPACE_rmw)/Release/strip" \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_action_msgs)/Release/strip" \
-L"$(WORKSPACE_unique_identifier_msgs)/Release/strip" \
-L"$(WORKSPACE_builtin_interfaces)/Release/strip"

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
$(WORKSPACE_RCL)/Release/strip/librcl.so \
$(WORKSPACE_rmw)/Release/strip/librmw.so \
$(WORKSPACE_rcutils)/Release/strip/librcutils.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_runtime_c.so \
$(WORKSPACE_action_msgs)/Release/strip/libaction_msgs__rosidl_generator_c.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_generator_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_generator_c.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
