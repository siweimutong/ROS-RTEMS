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
# 文   件   名: librcl.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2024 年 05 月 03 日
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
LOCAL_TARGET_NAME := librcl.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/rcl-5.3.7/rcl/src/rcl/arguments.c \
src/rcl-5.3.7/rcl/src/rcl/client.c \
src/rcl-5.3.7/rcl/src/rcl/common.c \
src/rcl-5.3.7/rcl/src/rcl/context.c \
src/rcl-5.3.7/rcl/src/rcl/domain_id.c \
src/rcl-5.3.7/rcl/src/rcl/event.c \
src/rcl-5.3.7/rcl/src/rcl/expand_topic_name.c \
src/rcl-5.3.7/rcl/src/rcl/graph.c \
src/rcl-5.3.7/rcl/src/rcl/guard_condition.c \
src/rcl-5.3.7/rcl/src/rcl/init.c \
src/rcl-5.3.7/rcl/src/rcl/init_options.c \
src/rcl-5.3.7/rcl/src/rcl/lexer.c \
src/rcl-5.3.7/rcl/src/rcl/lexer_lookahead.c \
src/rcl-5.3.7/rcl/src/rcl/localhost.c \
src/rcl-5.3.7/rcl/src/rcl/logging.c \
src/rcl-5.3.7/rcl/src/rcl/logging_rosout.c \
src/rcl-5.3.7/rcl/src/rcl/log_level.c \
src/rcl-5.3.7/rcl/src/rcl/network_flow_endpoints.c \
src/rcl-5.3.7/rcl/src/rcl/node.c \
src/rcl-5.3.7/rcl/src/rcl/node_options.c \
src/rcl-5.3.7/rcl/src/rcl/node_resolve_name.c \
src/rcl-5.3.7/rcl/src/rcl/publisher.c \
src/rcl-5.3.7/rcl/src/rcl/remap.c \
src/rcl-5.3.7/rcl/src/rcl/rmw_implementation_identifier_check.c \
src/rcl-5.3.7/rcl/src/rcl/security.c \
src/rcl-5.3.7/rcl/src/rcl/service.c \
src/rcl-5.3.7/rcl/src/rcl/subscription.c \
src/rcl-5.3.7/rcl/src/rcl/time.c \
src/rcl-5.3.7/rcl/src/rcl/timer.c \
src/rcl-5.3.7/rcl/src/rcl/validate_enclave_name.c \
src/rcl-5.3.7/rcl/src/rcl/validate_topic_name.c \
src/rcl-5.3.7/rcl/src/rcl/wait.c

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/rcl-5.3.7/rcl/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"./src/rcl-5.3.7/rcl_yaml_param_parser/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_rcl_interfaces)/src/rcl_interfaces/rosidl_generator_c" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_c" \
-I"$(WORKSPACE_logging)/src/rcl_logging-2.3.1/rcl_logging_interface/include"

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
-lrcl_interfaces__rosidl_generator_c \
-lrcl_interfaces__rosidl_typesupport_c \
-lrcl_interfaces__rosidl_typesupport_cpp \
-lrcl_interfaces__rosidl_typesupport_fastrtps_c \
-lrcl_interfaces__rosidl_typesupport_fastrtps_cpp \
-lrcl_interfaces__rosidl_typesupport_introspection_c \
-lrcl_interfaces__rosidl_typesupport_introspection_cpp \
-lrcl_logging_noop \
-lrcl_logging_spdlog \
-lrcl_logging_interface \
-lrcl-yaml-param-parser \
-lrcutils \
-lrmw \
-lrmw_implementation \
-lrosidl_runtime_c
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rcutils)/Release" \
-L"$(WORKSPACE_rmw)/Release/strip" \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_rcutils)/Release/strip" \
-L"$(WORKSPACE_rmw_implementation)/Release/strip" \
-L"$(WORKSPACE_rcl_interfaces)/Release/strip" \
-L"$(WORKSPACE_builtin_interfaces)/Release/strip" \
-L"$(WORKSPACE_logging)/Release/strip" \
-L"$(WORKSPACE_RCL)/Release/strip"

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
$(WORKSPACE_rcutils)/Release/strip/librcutils.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_runtime_c.so \
$(WORKSPACE_rmw_implementation)/Release/strip/librmw_implementation.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_generator_c.so \
$(WORKSPACE_logging)/Release/strip/librcl_logging_interface.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_cpp.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_rcl_interfaces)/Release/strip/librcl_interfaces__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_RCL)/Release/strip/librcl-yaml-param-parser.so \
$(WORKSPACE_logging)/Release/strip/librcl_logging_noop.so \
$(WORKSPACE_logging)/Release/strip/librcl_logging_spdlog.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
