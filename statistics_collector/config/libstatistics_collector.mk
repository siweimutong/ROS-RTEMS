#*********************************************************************************************************
#
#                                    �й�������Դ��֯
#
#                                   Ƕ��ʽʵʱ����ϵͳ
#
#                                SylixOS(TM)  LW : long wing
#
#                               Copyright All Rights Reserved
#
#--------------�ļ���Ϣ--------------------------------------------------------------------------------
#
# ��   ��   ��: test.mk
#
# ��   ��   ��: RealEvo-IDE
#
# �ļ���������: 2024 �� 07 �� 18 ��
#
# ��        ��: ���ļ��� RealEvo-IDE ���ɣ��������� Makefile ���ܣ������ֶ��޸�
#*********************************************************************************************************

#*********************************************************************************************************
# Clear setting
#*********************************************************************************************************
include $(CLEAR_VARS_MK)

#*********************************************************************************************************
# Target
#*********************************************************************************************************
LOCAL_TARGET_NAME := liblibstatistics_collector.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/libstatistics_collector/collector/collector.cpp \
src/libstatistics_collector/collector/generate_statistics_message.cpp \
src/libstatistics_collector/moving_average_statistics/moving_average.cpp \
src/libstatistics_collector/moving_average_statistics/types.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/include" \
-I"$(WORKSPACE_rcpputils)/src/rcpputils-2.4.2/include" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_statistics_msgs)/src/statistics_msgs/rosidl_generator_cpp"

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
-lbuiltin_interfaces__rosidl_generator_c \
-lbuiltin_interfaces__rosidl_typesupport_c \
-lbuiltin_interfaces__rosidl_typesupport_cpp \
-lbuiltin_interfaces__rosidl_typesupport_fastrtps_c \
-lbuiltin_interfaces__rosidl_typesupport_fastrtps_cpp \
-lbuiltin_interfaces__rosidl_typesupport_introspection_c \
-lbuiltin_interfaces__rosidl_typesupport_introspection_cpp \
-lrcl \
-lrcpputils \
-lrmw \
-lrosidl_runtime_c \
-lstatistics_msgs__rosidl_generator_c \
-lstatistics_msgs__rosidl_typesupport_c \
-lstatistics_msgs__rosidl_typesupport_cpp \
-lstatistics_msgs__rosidl_typesupport_fastrtps_c \
-lstatistics_msgs__rosidl_typesupport_fastrtps_cpp \
-lstatistics_msgs__rosidl_typesupport_introspection_c \
-lstatistics_msgs__rosidl_typesupport_introspection_cpp
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rcpputils)/Release/strip" \
-L"$(WORKSPACE_rosidl)/Release/strip" \
-L"$(WORKSPACE_builtin_interfaces)/Release/strip" \
-L"$(WORKSPACE_statistics_msgs)/Release/strip" \
-L"$(WORKSPACE_builtin_interfaces)/Release" \
-L"$(WORKSPACE_RCL)/Release/strip" \
-L"$(WORKSPACE_rmw)/Release/strip"

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
$(WORKSPACE_rcpputils)/Release/strip/librcpputils.so \
$(WORKSPACE_rosidl)/Release/strip/librosidl_runtime_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_cpp.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_cpp.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_fastrtps_cpp.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_generator_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_c.so \
$(WORKSPACE_builtin_interfaces)/Release/libbuiltin_interfaces__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_builtin_interfaces)/Release/strip/libbuiltin_interfaces__rosidl_typesupport_introspection_cpp.so \
$(WORKSPACE_RCL)/Release/strip/librcl.so \
$(WORKSPACE_rmw)/Release/strip/librmw.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_generator_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_fastrtps_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_introspection_c.so \
$(WORKSPACE_statistics_msgs)/Release/strip/libstatistics_msgs__rosidl_typesupport_introspection_cpp.so

include $(APPLICATION_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
