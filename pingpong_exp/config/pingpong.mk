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
# �ļ���������: 2024 �� 07 �� 30 ��
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
LOCAL_TARGET_NAME := pingpong

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS :=  \
src/examples-0.20.2/rclcpp/executors/cbg_executor/src/examples_rclcpp_cbg_executor/ping_node.cpp \
src/examples-0.20.2/rclcpp/executors/cbg_executor/src/examples_rclcpp_cbg_executor/pong_node.cpp \
src/examples-0.20.2/rclcpp/executors/cbg_executor/src/ping_pong.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"./src/examples-0.20.2/rclcpp/executors/cbg_executor/include" \
-I"$(WORKSPACE_rclcpp)/src/rclcpp-16.0.8/rclcpp/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl/include" \
-I"$(WORKSPACE_rcutils)/src/rcutils-5.1.5/include" \
-I"$(WORKSPACE_rmw)/src/rmw-6.1.1/rmw/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_c/include" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_typesupport_interface/include" \
-I"$(WORKSPACE_rcpputils)/src/rcpputils-2.4.2/include" \
-I"$(WORKSPACE_builtin_interfaces)/src/builtin_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_rosidl)/src/rosidl/rosidl_runtime_cpp/include" \
-I"$(WORKSPACE_rcl_interfaces)/src/rcl_interfaces/rosidl_generator_cpp" \
-I"$(WORKSPACE_statistics_msgs)/src/statistics_msgs/rosidl_generator_cpp" \
-I"$(WORKSPACE_libstatistics_collector)/src/include" \
-I"$(WORKSPACE_RCL)/src/rcl-5.3.7/rcl_yaml_param_parser/include" \
-I"$(WORKSPACE_std_msgs)/src/std_msgs/rosidl_generator_cpp"

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
-lrclcpp \
-lstd_msgs__rosidl_generator_c \
-lstd_msgs__rosidl_typesupport_c \
-lstd_msgs__rosidl_typesupport_cpp \
-lstd_msgs__rosidl_typesupport_fastrtps_c \
-lstd_msgs__rosidl_typesupport_fastrtps_cpp \
-lstd_msgs__rosidl_typesupport_introspection_c \
-lstd_msgs__rosidl_typesupport_introspection_cpp
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(WORKSPACE_rclcpp)/Release/strip" \
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
LOCAL_PRE_LINK_CMD := 
LOCAL_POST_LINK_CMD := 
LOCAL_PRE_STRIP_CMD := 
LOCAL_POST_STRIP_CMD := 

#*********************************************************************************************************
# Depend target
#*********************************************************************************************************
LOCAL_DEPEND_TARGET :=  \
$(WORKSPACE_rclcpp)/Release/strip/librclcpp.so \
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
