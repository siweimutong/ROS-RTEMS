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
# 文   件   名: librmw_fastrtps_dynamic_cpp.mk
#
# 创   建   人: RealEvo-IDE
#
# 文件创建日期: 2024 年 07 月 22 日
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
LOCAL_TARGET_NAME := librmw_fastrtps_dynamic_cpp.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/rmw_fastrtps_dynamic_cpp/src/client_service_common.cpp \
src/rmw_fastrtps_dynamic_cpp/src/get_client.cpp \
src/rmw_fastrtps_dynamic_cpp/src/get_participant.cpp \
src/rmw_fastrtps_dynamic_cpp/src/get_publisher.cpp \
src/rmw_fastrtps_dynamic_cpp/src/get_service.cpp \
src/rmw_fastrtps_dynamic_cpp/src/get_subscriber.cpp \
src/rmw_fastrtps_dynamic_cpp/src/identifier.cpp \
src/rmw_fastrtps_dynamic_cpp/src/init_rmw_context_impl.cpp \
src/rmw_fastrtps_dynamic_cpp/src/publisher.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_client.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_compare_gids_equal.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_count.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_event.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_features.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_get_endpoint_network_flow.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_get_gid_for_publisher.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_get_implementation_identifier.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_get_serialization_format.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_get_topic_endpoint_info.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_guard_condition.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_init.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_logging.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_node.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_node_info_and_types.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_node_names.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_publish.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_publisher.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_qos.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_request.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_response.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_serialize.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_service.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_service_names_and_types.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_service_server_is_available.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_subscription.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_take.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_topic_names_and_types.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_trigger_guard_condition.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_wait.cpp \
src/rmw_fastrtps_dynamic_cpp/src/rmw_wait_set.cpp \
src/rmw_fastrtps_dynamic_cpp/src/serialization_format.cpp \
src/rmw_fastrtps_dynamic_cpp/src/subscription.cpp \
src/rmw_fastrtps_dynamic_cpp/src/type_support_common.cpp \
src/rmw_fastrtps_dynamic_cpp/src/type_support_proxy.cpp \
src/rmw_fastrtps_dynamic_cpp/src/type_support_registry.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/rmw_fastrtps_dynamic_cpp/include" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rcpputils)/src/rcpputils-2.4.2/include" \
-I"./src/rmw_fastrtps_shared_cpp/include" \
-I"$(WORKSPACE_libfastrtps-master)/src/Fast-DDS/include" \
-I"$(WORKSPACE_libfastrtps-master)/src/Fast-DDS/build/include" \
-I"$(WORKSPACE_libfastrtps-master)/src/Fast-CDR/include" \
-I"$(WORKSPACE_libfastrtps-master)/src/Fast-CDR/build/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_introspection_cpp/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_introspection_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl_typesupport_fastrtps/rosidl_typesupport_fastrtps_cpp/include" \
-I"$(WORKSPACE_rmw_dds_common)/src/rmw_dds_common_src/rmw_dds_common/include" \
-I"$(WORKSPACE_rmw_dds_common)/src/rmw_dds_common/rosidl_generator_cpp"

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
-lrcpputils \
-lrcutils \
-lrmw_dds_common__rosidl_generator_c \
-lrmw_dds_common__rosidl_typesupport_c \
-lrmw_dds_common__rosidl_typesupport_cpp \
-lrmw_dds_common__rosidl_typesupport_fastrtps_c \
-lrmw_dds_common__rosidl_typesupport_fastrtps_cpp \
-lrmw_dds_common__rosidl_typesupport_introspection_c \
-lrmw_dds_common__rosidl_typesupport_introspection_cpp \
-lrmw_dds_common \
-lrmw_fastrtps_shared_cpp \
-lfastcdr \
-lfastrtps-master \
-lrmw \
-lrosidl_runtime_c \
-lrosidl_typesupport_fastrtps_c \
-lrosidl_typesupport_fastrtps_cpp \
-lrosidl_typesupport_introspection_c \
-lrosidl_typesupport_introspection_cpp
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rmw)/Release/strip" \
-L"$(WORKSPACE_rcutils)/Release/strip" \
-L"$(WORKSPACE_rcpputils)/Release/strip" \
-L"$(WORKSPACE_rmw_fastrtps)/Release/strip" \
-L"$(WORKSPACE_libfastrtps-master)/Release/strip" \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_rmw_dds_common)/Release/strip"

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
$(WORKSPACE_rcpputils)/Release/strip/librcpputils.so \
$(WORKSPACE_rmw_fastrtps)/Release/strip/librmw_fastrtps_shared_cpp.so \
$(WORKSPACE_libfastrtps-master)/Release/strip/libfastrtps-master.so \
$(WORKSPACE_libfastrtps-master)/Release/strip/libfastcdr.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_runtime_c.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_introspection_c.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_generator_c.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_typesupport_c.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_typesupport_cpp.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_rmw_dds_common)/Release/strip/librmw_dds_common__rosidl_typesupport_introspection_cpp.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
