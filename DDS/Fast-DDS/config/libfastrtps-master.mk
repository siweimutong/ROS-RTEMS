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
# ��   ��   ��: libfastrtps.mk
#
# ��   ��   ��: RealEvo-IDE
#
# �ļ���������: 2021 �� 08 �� 31 ��
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
LOCAL_TARGET_NAME := libfastrtps-master.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
src/Fast-DDS/src/cpp/fastrtps_deprecated/attributes/TopicAttributes.cpp \
src/Fast-DDS/src/cpp/fastdds/core/Entity.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/Condition.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/ConditionNotifier.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/GuardCondition.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/StatusCondition.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/StatusConditionImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/WaitSet.cpp \
src/Fast-DDS/src/cpp/fastdds/core/condition/WaitSetImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/core/policy/ParameterList.cpp \
src/Fast-DDS/src/cpp/fastdds/core/policy/QosPolicyUtils.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/qos/WriterQos.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/qos/ReaderQos.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/BuiltinProtocols.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/DirectMessageSender.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/PDP.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/PDPSimple.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/PDPListener.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDP.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDPSimple.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDPSimpleListeners.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDPStatic.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/liveliness/WLP.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/liveliness/WLPListener.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/data/ParticipantProxyData.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/data/WriterProxyData.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/data/ReaderProxyData.cpp \
src/Fast-DDS/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp \
src/Fast-DDS/src/cpp/rtps/flowcontrol/FlowControllerConsts.cpp \
src/Fast-DDS/src/cpp/rtps/flowcontrol/FlowControllerFactory.cpp \
src/Fast-DDS/src/cpp/rtps/exceptions/Exception.cpp \
src/Fast-DDS/src/cpp/rtps/attributes/PropertyPolicy.cpp \
src/Fast-DDS/src/cpp/rtps/common/Token.cpp \
src/Fast-DDS/src/cpp/rtps/xmlparser/XMLParserCommon.cpp \
src/Fast-DDS/src/cpp/rtps/xmlparser/XMLElementParser.cpp \
src/Fast-DDS/src/cpp/rtps/xmlparser/XMLDynamicParser.cpp \
src/Fast-DDS/src/cpp/rtps/xmlparser/XMLEndpointParser.cpp \
src/Fast-DDS/src/cpp/rtps/xmlparser/XMLParser.cpp \
src/Fast-DDS/src/cpp/rtps/xmlparser/XMLProfileManager.cpp \
src/Fast-DDS/src/cpp/rtps/writer/PersistentWriter.cpp \
src/Fast-DDS/src/cpp/rtps/writer/StatelessPersistentWriter.cpp \
src/Fast-DDS/src/cpp/rtps/writer/StatefulPersistentWriter.cpp \
src/Fast-DDS/src/cpp/rtps/reader/StatelessPersistentReader.cpp \
src/Fast-DDS/src/cpp/rtps/reader/StatefulPersistentReader.cpp \
src/Fast-DDS/src/cpp/rtps/persistence/PersistenceFactory.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/database/backup/SharedBackupFunctions.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDPClient.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDPServer.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/endpoint/EDPServerListeners.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/database/DiscoveryDataBase.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/database/DiscoveryParticipantInfo.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/database/DiscoveryParticipantsAckStatus.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/database/DiscoverySharedInfo.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/PDPClient.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/PDPServer.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/PDPServerListener.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/timedevent/DSClientEvent.cpp \
src/Fast-DDS/src/cpp/rtps/builtin/discovery/participant/timedevent/DServerEvent.cpp \
src/Fast-DDS/src/cpp/utils/md5.cpp \
src/Fast-DDS/src/cpp/utils/StringMatching.cpp \
src/Fast-DDS/src/cpp/utils/IPLocator.cpp \
src/Fast-DDS/src/cpp/utils/System.cpp \
src/Fast-DDS/src/cpp/utils/SystemInfo.cpp \
src/Fast-DDS/src/cpp/utils/TimedConditionVariable.cpp \
src/Fast-DDS/src/cpp/utils/string_convert.cpp \
src/Fast-DDS/src/cpp/dds/core/types.cpp \
src/Fast-DDS/src/cpp/dds/core/Exception.cpp \
src/Fast-DDS/src/cpp/dds/domain/DomainParticipant.cpp \
src/Fast-DDS/src/cpp/dds/pub/Publisher.cpp \
src/Fast-DDS/src/cpp/dds/pub/AnyDataWriter.cpp \
src/Fast-DDS/src/cpp/dds/pub/DataWriter.cpp \
src/Fast-DDS/src/cpp/dds/sub/Subscriber.cpp \
src/Fast-DDS/src/cpp/dds/sub/DataReader.cpp \
src/Fast-DDS/src/cpp/dds/topic/Topic.cpp \
src/Fast-DDS/src/cpp/statistics/fastdds/domain/DomainParticipant.cpp \
src/Fast-DDS/src/cpp/statistics/fastdds/publisher/qos/DataWriterQos.cpp \
src/Fast-DDS/src/cpp/statistics/fastdds/subscriber/qos/DataReaderQos.cpp \
src/Fast-DDS/src/cpp/rtps/transport/shared_mem/test_SharedMemTransport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/shared_mem/SharedMemTransport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPChannelResourceSecure.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPAcceptorSecure.cpp \
src/Fast-DDS/src/cpp/rtps/persistence/SQLite3PersistenceService.cpp \
src/Fast-DDS/src/cpp/rtps/persistence/sqlite3.c
# src/Fast-DDS/src/cpp/utils/IPFinder.cpp

#*********************************************************************************************************
# Header file search path (eg. LOCAL_INC_PATH := -I"Your header files search path")
#*********************************************************************************************************
LOCAL_INC_PATH :=  \
-I"$(SYLIXOS_BASE_PATH)/libgtest/googletest/googlemock/include/" \
-I"$(SYLIXOS_BASE_PATH)/libgtest/googletest/googletest/include/" \
-I"$(SYLIXOS_BASE_PATH)/openssl/openssl/include/" \
-I"src/Fast-DDS/include/" \
-I"src/Fast-DDS/src/cpp/" \
-I"src/Fast-DDS/build/include/" \
-I"src/Fast-DDS/test/mock/rtps/RTPSParticipantImpl/" \
-I"src/Fast-DDS/thirdparty/boost/include/" \
-I"src/Fast-DDS/thirdparty/nlohmann-json/" \
-I"src/Fast-DDS/thirdparty/taocpp-pegtl/" \
-I"src/Fast-CDR/include/" \
-I"src/Fast-CDR/build/include/" \
-I"src/memory-main/include/" \
-I"src/memory-main/build/src/" \
-I"src/memory-main/build/src" \
-I"src/asio/include/" \
-I"src/tinyxml2/"

#*********************************************************************************************************
# Pre-defined macro (eg. -DYOUR_MARCO=1)
#*********************************************************************************************************
LOCAL_DSYMBOL := \
-D_GLIBCXX_USE_C99 \
-D__linux__ \
-DASIO_DISABLE_CSTDINT \
-DBOOST_ASIO_STANDALONE \
-DASIO_STANDALONE \
-DSHM_TRANSPORT_BUILTIN

#*********************************************************************************************************
# Compiler flags
#*********************************************************************************************************
LOCAL_CFLAGS   := 
LOCAL_CXXFLAGS := -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g -Wno-unknown-pragmas
LOCAL_LINKFLAGS :=

#*********************************************************************************************************
# Depend library (eg. LOCAL_DEPEND_LIB := -la LOCAL_DEPEND_LIB_PATH := -L"Your library search path")
#*********************************************************************************************************
LOCAL_DEPEND_LIB :=  \
-lssl \
-ltinyxml2 \
-lfastcdr \
-lfoonathan_memory \
-lfastrtps_pre
LOCAL_DEPEND_LIB_PATH :=  \
-L"$(SYLIXOS_BASE_PATH)/openssl/$(Output)/strip/" \
-L"$(Output)/strip/"

#*********************************************************************************************************
# Linker specific
#*********************************************************************************************************
LOCAL_NO_UNDEF_SYM := no

#*********************************************************************************************************
# C++ config
#*********************************************************************************************************
LOCAL_USE_CXX := yes
LOCAL_USE_CXX_EXCEPT := yes

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
./$(Output)/strip/libfoonathan_memory.so \
./$(Output)/strip/libfastcdr.so \
./$(Output)/strip/libfastrtps_pre.so

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
