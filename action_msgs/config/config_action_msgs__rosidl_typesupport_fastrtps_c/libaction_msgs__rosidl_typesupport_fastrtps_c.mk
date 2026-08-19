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
# 文   件   名: libaction_msgs__rosidl_typesupport_fastrtps_c.mk
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
LOCAL_TARGET_NAME := libaction_msgs__rosidl_typesupport_fastrtps_c.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/action_msgs/rosidl_typesupport_fastrtps_c/action_msgs/msg/detail/goal_info__type_support_c.cpp \
src/action_msgs/rosidl_typesupport_fastrtps_c/action_msgs/msg/detail/goal_status_array__type_support_c.cpp \
src/action_msgs/rosidl_typesupport_fastrtps_c/action_msgs/msg/detail/goal_status__type_support_c.cpp \
src/action_msgs/rosidl_typesupport_fastrtps_c/action_msgs/srv/detail/cancel_goal__type_support_c.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/action_msgs/rosidl_typesupport_fastrtps_c" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_cpp/include" \
-I"$(WORKSPACE_libfastrtps-master)/src/Fast-CDR/include" \
-I"$(WORKSPACE_libfastrtps-master)/src/Fast-CDR/build/include" \
-I"./src/action_msgs/rosidl_generator_c" \
-I"$(WORKSPACE_unique_identifier_msgs)/src/unique_identifier_msgs/rosidl_generator_c" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_c" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := 

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS := 
LOCAL_CXXFLAGS := -fexceptions
LOCAL_LINKFLAGS := 

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB :=  \
-lrosidl_runtime_c \
-lrosidl_typesupport_fastrtps_c \
-lrosidl_typesupport_fastrtps_cpp \
-lfastcdr \
-laction_msgs__rosidl_generator_c \
-lbuiltin_interfaces__rosidl_generator_c \
-lrmw \
-lrcutils \
-lunique_identifier_msgs__rosidl_generator_c \
-lunique_identifier_msgs__rosidl_typesupport_c \
-lunique_identifier_msgs__rosidl_typesupport_cpp \
-lunique_identifier_msgs__rosidl_typesupport_fastrtps_c \
-lunique_identifier_msgs__rosidl_typesupport_fastrtps_cpp \
-lunique_identifier_msgs__rosidl_typesupport_introspection_c \
-lunique_identifier_msgs__rosidl_typesupport_introspection_cpp
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_libfastrtps-master)/Release/strip" \
-L"$(WORKSPACE_action_msgs)/Release/strip" \
-L"$(WORKSPACE_unique_identifier_msgs)/Release/strip" \
-L"$(WORKSPACE_builtin_interfaces)/Release/strip" \
-L"$(WORKSPACE_rmw)/Release/strip" \
-L"$(WORKSPACE_rcutils)/Release/strip"

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
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_libfastrtps-master)/Release/strip/libfastcdr.so \
$(WORKSPACE_action_msgs)/Release/strip/libaction_msgs__rosidl_generator_c.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_generator_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_generator_c.so \
$(WORKSPACE_rmw)/Release/strip/librmw.so \
$(WORKSPACE_rcutils)/Release/strip/librcutils.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_unique_identifier_msgs)/Release/strip/libunique_identifier_msgs__rosidl_typesupport_introspection_cpp.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
