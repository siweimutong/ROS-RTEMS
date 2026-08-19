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
LOCAL_TARGET_NAME := libfastrtps_pre.so

#*********************************************************************************************************
# Source list
#*********************************************************************************************************
LOCAL_SRCS := \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterCompoundCondition.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterExpression.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterExpressionParser.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterFactory.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterField.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterParameter.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterPredicate.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterValue.cpp \
src/Fast-DDS/src/cpp/fastdds/log/Log.cpp \
src/Fast-DDS/src/cpp/fastdds/log/OStreamConsumer.cpp \
src/Fast-DDS/src/cpp/fastdds/log/StdoutErrConsumer.cpp \
src/Fast-DDS/src/cpp/fastdds/log/StdoutConsumer.cpp \
src/Fast-DDS/src/cpp/fastdds/log/FileConsumer.cpp \
src/Fast-DDS/src/cpp/rtps/common/Time_t.cpp \
src/Fast-DDS/src/cpp/rtps/resources/ResourceEvent.cpp \
src/Fast-DDS/src/cpp/rtps/resources/TimedEvent.cpp \
src/Fast-DDS/src/cpp/rtps/resources/TimedEventImpl.cpp \
src/Fast-DDS/src/cpp/rtps/writer/LivelinessManager.cpp \
src/Fast-DDS/src/cpp/rtps/writer/LocatorSelectorSender.cpp \
src/Fast-DDS/src/cpp/rtps/writer/RTPSWriter.cpp \
src/Fast-DDS/src/cpp/rtps/writer/StatefulWriter.cpp \
src/Fast-DDS/src/cpp/rtps/writer/ReaderProxy.cpp \
src/Fast-DDS/src/cpp/rtps/writer/StatelessWriter.cpp \
src/Fast-DDS/src/cpp/rtps/writer/ReaderLocator.cpp \
src/Fast-DDS/src/cpp/rtps/history/CacheChangePool.cpp \
src/Fast-DDS/src/cpp/rtps/history/History.cpp \
src/Fast-DDS/src/cpp/rtps/history/WriterHistory.cpp \
src/Fast-DDS/src/cpp/rtps/history/ReaderHistory.cpp \
src/Fast-DDS/src/cpp/rtps/history/TopicPayloadPool.cpp \
src/Fast-DDS/src/cpp/rtps/history/TopicPayloadPoolRegistry.cpp \
src/Fast-DDS/src/cpp/rtps/DataSharing/DataSharingPayloadPool.cpp \
src/Fast-DDS/src/cpp/rtps/DataSharing/DataSharingListener.cpp \
src/Fast-DDS/src/cpp/rtps/DataSharing/DataSharingNotification.cpp \
src/Fast-DDS/src/cpp/rtps/reader/WriterProxy.cpp \
src/Fast-DDS/src/cpp/rtps/reader/StatefulReader.cpp \
src/Fast-DDS/src/cpp/rtps/reader/StatelessReader.cpp \
src/Fast-DDS/src/cpp/rtps/reader/RTPSReader.cpp \
src/Fast-DDS/src/cpp/rtps/messages/RTPSMessageCreator.cpp \
src/Fast-DDS/src/cpp/rtps/messages/RTPSMessageGroup.cpp \
src/Fast-DDS/src/cpp/rtps/messages/RTPSGapBuilder.cpp \
src/Fast-DDS/src/cpp/rtps/messages/SendBuffersManager.cpp \
src/Fast-DDS/src/cpp/rtps/messages/MessageReceiver.cpp \
src/Fast-DDS/src/cpp/rtps/network/NetworkFactory.cpp \
src/Fast-DDS/src/cpp/rtps/network/ReceiverResource.cpp \
src/Fast-DDS/src/cpp/rtps/participant/RTPSParticipant.cpp \
src/Fast-DDS/src/cpp/rtps/participant/RTPSParticipantImpl.cpp \
src/Fast-DDS/src/cpp/rtps/RTPSDomain.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/Domain.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/participant/Participant.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/participant/ParticipantImpl.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/publisher/Publisher.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/publisher/PublisherImpl.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/publisher/PublisherHistory.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/subscriber/Subscriber.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/subscriber/SubscriberImpl.cpp \
src/Fast-DDS/src/cpp/fastrtps_deprecated/subscriber/SubscriberHistory.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/DataWriter.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/DataWriterImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/ContentFilteredTopic.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/Topic.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/TopicImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/TypeSupport.cpp \
src/Fast-DDS/src/cpp/fastdds/topic/qos/TopicQos.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/qos/DataWriterQos.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/qos/DataReaderQos.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/PublisherImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/qos/PublisherQos.cpp \
src/Fast-DDS/src/cpp/fastdds/publisher/Publisher.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/SubscriberImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/qos/SubscriberQos.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/Subscriber.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/DataReader.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/DataReaderImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/subscriber/history/DataReaderHistory.cpp \
src/Fast-DDS/src/cpp/fastdds/domain/DomainParticipantFactory.cpp \
src/Fast-DDS/src/cpp/fastdds/domain/DomainParticipantImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/domain/DomainParticipant.cpp \
src/Fast-DDS/src/cpp/fastdds/domain/qos/DomainParticipantQos.cpp \
src/Fast-DDS/src/cpp/fastdds/domain/qos/DomainParticipantFactoryQos.cpp \
src/Fast-DDS/src/cpp/fastdds/builtin/typelookup/common/TypeLookupTypes.cpp \
src/Fast-DDS/src/cpp/fastdds/builtin/common/RPCHeadersImpl.cpp \
src/Fast-DDS/src/cpp/fastdds/builtin/typelookup/TypeLookupManager.cpp \
src/Fast-DDS/src/cpp/fastdds/builtin/typelookup/TypeLookupRequestListener.cpp \
src/Fast-DDS/src/cpp/fastdds/builtin/typelookup/TypeLookupReplyListener.cpp \
src/Fast-DDS/src/cpp/rtps/transport/ChannelResource.cpp \
src/Fast-DDS/src/cpp/rtps/transport/UDPChannelResource.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPChannelResource.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPChannelResourceBasic.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPAcceptor.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPAcceptorBasic.cpp \
src/Fast-DDS/src/cpp/rtps/transport/UDPv4Transport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPTransportInterface.cpp \
src/Fast-DDS/src/cpp/rtps/transport/UDPTransportInterface.cpp \
src/Fast-DDS/src/cpp/rtps/transport/shared_mem/SharedMemTransportDescriptor.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPv4Transport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/UDPv6Transport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/TCPv6Transport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/test_UDPv4Transport.cpp \
src/Fast-DDS/src/cpp/rtps/transport/tcp/TCPControlMessage.cpp \
src/Fast-DDS/src/cpp/rtps/transport/tcp/RTCPMessageManager.cpp \
src/Fast-DDS/src/cpp/rtps/transport/ChainingTransport.cpp \
src/Fast-DDS/src/cpp/dynamic-types/AnnotationDescriptor.cpp \
src/Fast-DDS/src/cpp/dynamic-types/AnnotationParameterValue.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicData.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicDataFactory.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicType.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicPubSubType.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicTypePtr.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicDataPtr.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicTypeBuilder.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicTypeBuilderPtr.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicTypeBuilderFactory.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicTypeMember.cpp \
src/Fast-DDS/src/cpp/dynamic-types/MemberDescriptor.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeDescriptor.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeIdentifier.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeIdentifierTypes.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeObject.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeObjectHashId.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeObjectFactory.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypeNamesGenerator.cpp \
src/Fast-DDS/src/cpp/dynamic-types/TypesBase.cpp \
src/Fast-DDS/src/cpp/dynamic-types/BuiltinAnnotationsTypeObject.cpp \
src/Fast-DDS/src/cpp/dynamic-types/DynamicDataHelper.cpp \
src/Fast-DDS/src/cpp/utils/string_convert.cpp
#src/Fast-DDS/thirdparty/boost/include/boost/interprocess/mapped_region.hpp

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
-lfoonathan_memory \
-lfastcdr
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
./$(Output)/strip/libtinyxml2.so \
./$(Output)/strip/libfoonathan_memory.so \
./$(Output)/strip/libfastcdr.so

include $(LIBRARY_MK)

#*********************************************************************************************************
# End
#*********************************************************************************************************
