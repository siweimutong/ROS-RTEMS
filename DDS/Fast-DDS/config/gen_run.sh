#!/bin/bash

target_dir="./tmp"
build_modle=$1
flag=0

root_path=$2
log_path="$root_path/build/logs"
log_file_path="$log_path/compile_log_dds.txt"

if [[ ! -d "$log_path" ]]; then
    mkdir -p "$log_path"  # -pis key: creates build even if no subdirectories exist under root_path
fi

# Checkfile是nonot found,not found则Create
if [[ ! -f "$log_file_path" ]]; then
    touch "$log_file_path"  # Create empty file (if directory exists)
fi

# Key logic: -d detects whether a path is an existing directory
if [ -d "${target_dir}" ]; then
    # rm -rf Force recursive deletedirectory(Deletes files/subdirectories inside, no errors)
    if [[ "$build_modle" == "force" ]]; then
        rm -rf "${target_dir}"
        flag=1
    elif [[ "$build_modle" == "no-force" ]]; then
        flag=0
    else
        echo "build_modle arg error!!!"
        exit 1
    fi
else
    flag=1
fi

if [[ "$flag" == 1 ]]; then
    mkdir tmp
    cd tmp
    curl -s https://waf.io/waf-2.0.19 > waf
    chmod +x ./waf
    git init >> $log_file_path 2>&1
    GIT_CMD="git submodule add https://gitlab.rtems.org/rtems/tools/rtems_waf.git rtems_waf >> $log_file_path 2>&1"

    # Execute command and capture result
    if eval $GIT_CMD; then
        echo "✅ Submodule rtems_waf added successfully!" >> $log_file_path 2>&1
    else
        # CaptureFailed,OutputErrorinfo并退出
        ERROR_CODE=$?  # Get command exit code
        echo "❌ Submodule rtems_waf add failed! Exit code: ${ERROR_CODE}" >> $log_file_path 2>&1
        echo "❌ Failed原becausecan能:网络问题/仓库not found/pathalready存在/权限不足" >> $log_file_path 2>&1
        
        # Optional:Output详细Error日志(Help diagnose issues)
        echo "📝 Detailed error log: " >> $log_file_path 2>&1
        $GIT_CMD 2>&1  # Re-execute and output full error info (2>&1 redirects stderr to stdout)
        
        exit $ERROR_CODE  # Exit with non-zero code to notify caller of failure
    fi

    touch init.c
    echo "/*
    * Simple RTEMS configuration
    */

    #define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
    #define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

    #define CONFIGURE_UNLIMITED_OBJECTS
    #define CONFIGURE_UNIFIED_WORK_AREAS

    #define CONFIGURE_RTEMS_INIT_TASKS_TABLE

    #define CONFIGURE_INIT

    #include <rtems/confdefs.h>" > init.c

    # python ../mk_to_wscript_static.py ../libfastrtps-master.mk wscript

    touch wscript
    echo '

#!/usr/bin/env python
# A Waf script to build an RTEMS project with static libraries

from __future__ import print_function

rtems_version = "6"

try:
    import rtems_waf.rtems as rtems
except ImportError:
    print("error: no rtems_waf git submodule")
    import sys
    sys.exit(1)

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)
    ctx.load("compiler_cxx")  # Ensure C++ compiler is loaded

def bsp_configure(conf, arch_bsp):
    # 这里can以进Row BSP 相关的ConfigurationCheck
    pass

def options(opt):
    rtems.options(opt)

def configure(conf):
    rtems.configure(conf, bsp_configure=bsp_configure)

def build(bld):
    rtems.build(bld)

    # fastrtps-master target (static library)
    # Compile and link static library
    bld.stlib(
        target="fastrtps",
        source=[
            "init.c",
            "../../code/thirdparty/boost/include/stub/rtems_flock_stub.cpp",
            "../../code/thirdparty/boost/include/stub/rtems_posix_stub.cpp",
            "../../code/thirdparty/boost/include/stub/rtems_shm_stub.cpp",
            "../../code/src/cpp/fastrtps_deprecated/attributes/TopicAttributes.cpp",
            "../../code/src/cpp/fastdds/core/Entity.cpp",
            "../../code/src/cpp/fastdds/core/condition/Condition.cpp",
            "../../code/src/cpp/fastdds/core/condition/ConditionNotifier.cpp",
            "../../code/src/cpp/fastdds/core/condition/GuardCondition.cpp",
            "../../code/src/cpp/fastdds/core/condition/StatusCondition.cpp",
            "../../code/src/cpp/fastdds/core/condition/StatusConditionImpl.cpp",
            "../../code/src/cpp/fastdds/core/condition/WaitSet.cpp",
            "../../code/src/cpp/fastdds/core/condition/WaitSetImpl.cpp",
            "../../code/src/cpp/fastdds/core/policy/ParameterList.cpp",
            "../../code/src/cpp/fastdds/core/policy/QosPolicyUtils.cpp",
            "../../code/src/cpp/fastdds/publisher/qos/WriterQos.cpp",
            "../../code/src/cpp/fastdds/subscriber/qos/ReaderQos.cpp",
            "../../code/src/cpp/rtps/builtin/BuiltinProtocols.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/DirectMessageSender.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/PDP.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/PDPSimple.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/PDPListener.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDP.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDPSimple.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDPSimpleListeners.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDPStatic.cpp",
            "../../code/src/cpp/rtps/builtin/liveliness/WLP.cpp",
            "../../code/src/cpp/rtps/builtin/liveliness/WLPListener.cpp",
            "../../code/src/cpp/rtps/builtin/data/ParticipantProxyData.cpp",
            "../../code/src/cpp/rtps/builtin/data/WriterProxyData.cpp",
            "../../code/src/cpp/rtps/builtin/data/ReaderProxyData.cpp",
            "../../code/src/cpp/rtps/flowcontrol/ThroughputControllerDescriptor.cpp",
            "../../code/src/cpp/rtps/flowcontrol/FlowControllerConsts.cpp",
            "../../code/src/cpp/rtps/flowcontrol/FlowControllerFactory.cpp",
            "../../code/src/cpp/rtps/exceptions/Exception.cpp",
            "../../code/src/cpp/rtps/attributes/PropertyPolicy.cpp",
            "../../code/src/cpp/rtps/common/Token.cpp",
            "../../code/src/cpp/rtps/xmlparser/XMLParserCommon.cpp",
            "../../code/src/cpp/rtps/xmlparser/XMLElementParser.cpp",
            "../../code/src/cpp/rtps/xmlparser/XMLDynamicParser.cpp",
            "../../code/src/cpp/rtps/xmlparser/XMLEndpointParser.cpp",
            "../../code/src/cpp/rtps/xmlparser/XMLParser.cpp",
            "../../code/src/cpp/rtps/xmlparser/XMLProfileManager.cpp",
            "../../code/src/cpp/rtps/writer/PersistentWriter.cpp",
            "../../code/src/cpp/rtps/writer/StatelessPersistentWriter.cpp",
            "../../code/src/cpp/rtps/writer/StatefulPersistentWriter.cpp",
            "../../code/src/cpp/rtps/reader/StatelessPersistentReader.cpp",
            "../../code/src/cpp/rtps/reader/StatefulPersistentReader.cpp",
            "../../code/src/cpp/rtps/persistence/PersistenceFactory.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/database/backup/SharedBackupFunctions.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDPClient.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDPServer.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/endpoint/EDPServerListeners.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/database/DiscoveryDataBase.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/database/DiscoveryParticipantInfo.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/database/DiscoveryParticipantsAckStatus.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/database/DiscoverySharedInfo.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/PDPClient.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/PDPServer.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/PDPServerListener.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/timedevent/DSClientEvent.cpp",
            "../../code/src/cpp/rtps/builtin/discovery/participant/timedevent/DServerEvent.cpp",
            "../../code/src/cpp/utils/md5.cpp",
            "../../code/src/cpp/utils/StringMatching.cpp",
            "../../code/src/cpp/utils/IPLocator.cpp",
            "../../code/src/cpp/utils/System.cpp",
            "../../code/src/cpp/utils/SystemInfo.cpp",
            "../../code/src/cpp/utils/TimedConditionVariable.cpp",
            "../../code/src/cpp/utils/string_convert.cpp",
            "../../code/src/cpp/dds/core/types.cpp",
            "../../code/src/cpp/dds/core/Exception.cpp",
            "../../code/src/cpp/dds/domain/DomainParticipant.cpp",
            "../../code/src/cpp/dds/pub/Publisher.cpp",
            "../../code/src/cpp/dds/pub/AnyDataWriter.cpp",
            "../../code/src/cpp/dds/pub/DataWriter.cpp",
            "../../code/src/cpp/dds/sub/Subscriber.cpp",
            "../../code/src/cpp/dds/sub/DataReader.cpp",
            "../../code/src/cpp/dds/topic/Topic.cpp",
            "../../code/src/cpp/statistics/fastdds/domain/DomainParticipant.cpp",
            "../../code/src/cpp/statistics/fastdds/publisher/qos/DataWriterQos.cpp",
            "../../code/src/cpp/statistics/fastdds/subscriber/qos/DataReaderQos.cpp",
            "../../code/src/cpp/rtps/transport/shared_mem/test_SharedMemTransport.cpp",
            "../../code/src/cpp/rtps/transport/shared_mem/SharedMemTransport.cpp",
            "../../code/src/cpp/rtps/transport/TCPChannelResourceSecure.cpp",
            "../../code/src/cpp/rtps/transport/TCPAcceptorSecure.cpp",
            "../../code/src/cpp/rtps/persistence/SQLite3PersistenceService.cpp",
            "../../code/src/cpp/rtps/persistence/sqlite3.c",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterCompoundCondition.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterExpression.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterExpressionParser.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterFactory.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterField.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterParameter.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterPredicate.cpp",
            "../../code/src/cpp/fastdds/topic/DDSSQLFilter/DDSFilterValue.cpp",
            "../../code/src/cpp/fastdds/log/Log.cpp",
            "../../code/src/cpp/fastdds/log/OStreamConsumer.cpp",
            "../../code/src/cpp/fastdds/log/StdoutErrConsumer.cpp",
            "../../code/src/cpp/fastdds/log/StdoutConsumer.cpp",
            "../../code/src/cpp/fastdds/log/FileConsumer.cpp",
            "../../code/src/cpp/rtps/common/Time_t.cpp",
            "../../code/src/cpp/rtps/resources/ResourceEvent.cpp",
            "../../code/src/cpp/rtps/resources/TimedEvent.cpp",
            "../../code/src/cpp/rtps/resources/TimedEventImpl.cpp",
            "../../code/src/cpp/rtps/writer/LivelinessManager.cpp",
            "../../code/src/cpp/rtps/writer/LocatorSelectorSender.cpp",
            "../../code/src/cpp/rtps/writer/RTPSWriter.cpp",
            "../../code/src/cpp/rtps/writer/StatefulWriter.cpp",
            "../../code/src/cpp/rtps/writer/ReaderProxy.cpp",
            "../../code/src/cpp/rtps/writer/StatelessWriter.cpp",
            "../../code/src/cpp/rtps/writer/ReaderLocator.cpp",
            "../../code/src/cpp/rtps/history/CacheChangePool.cpp",
            "../../code/src/cpp/rtps/history/History.cpp",
            "../../code/src/cpp/rtps/history/WriterHistory.cpp",
            "../../code/src/cpp/rtps/history/ReaderHistory.cpp",
            "../../code/src/cpp/rtps/history/TopicPayloadPool.cpp",
            "../../code/src/cpp/rtps/history/TopicPayloadPoolRegistry.cpp",
            "../../code/src/cpp/rtps/DataSharing/DataSharingPayloadPool.cpp",
            "../../code/src/cpp/rtps/DataSharing/DataSharingListener.cpp",
            "../../code/src/cpp/rtps/DataSharing/DataSharingNotification.cpp",
            "../../code/src/cpp/rtps/reader/WriterProxy.cpp",
            "../../code/src/cpp/rtps/reader/StatefulReader.cpp",
            "../../code/src/cpp/rtps/reader/StatelessReader.cpp",
            "../../code/src/cpp/rtps/reader/RTPSReader.cpp",
            "../../code/src/cpp/rtps/messages/RTPSMessageCreator.cpp",
            "../../code/src/cpp/rtps/messages/RTPSMessageGroup.cpp",
            "../../code/src/cpp/rtps/messages/RTPSGapBuilder.cpp",
            "../../code/src/cpp/rtps/messages/SendBuffersManager.cpp",
            "../../code/src/cpp/rtps/messages/MessageReceiver.cpp",
            "../../code/src/cpp/rtps/network/NetworkFactory.cpp",
            "../../code/src/cpp/rtps/network/ReceiverResource.cpp",
            "../../code/src/cpp/rtps/participant/RTPSParticipant.cpp",
            "../../code/src/cpp/rtps/participant/RTPSParticipantImpl.cpp",
            "../../code/src/cpp/rtps/RTPSDomain.cpp",
            "../../code/src/cpp/fastrtps_deprecated/Domain.cpp",
            "../../code/src/cpp/fastrtps_deprecated/participant/Participant.cpp",
            "../../code/src/cpp/fastrtps_deprecated/participant/ParticipantImpl.cpp",
            "../../code/src/cpp/fastrtps_deprecated/publisher/Publisher.cpp",
            "../../code/src/cpp/fastrtps_deprecated/publisher/PublisherImpl.cpp",
            "../../code/src/cpp/fastrtps_deprecated/publisher/PublisherHistory.cpp",
            "../../code/src/cpp/fastrtps_deprecated/subscriber/Subscriber.cpp",
            "../../code/src/cpp/fastrtps_deprecated/subscriber/SubscriberImpl.cpp",
            "../../code/src/cpp/fastrtps_deprecated/subscriber/SubscriberHistory.cpp",
            "../../code/src/cpp/fastdds/publisher/DataWriter.cpp",
            "../../code/src/cpp/fastdds/publisher/DataWriterImpl.cpp",
            "../../code/src/cpp/fastdds/topic/ContentFilteredTopic.cpp",
            "../../code/src/cpp/fastdds/topic/Topic.cpp",
            "../../code/src/cpp/fastdds/topic/TopicImpl.cpp",
            "../../code/src/cpp/fastdds/topic/TypeSupport.cpp",
            "../../code/src/cpp/fastdds/topic/qos/TopicQos.cpp",
            "../../code/src/cpp/fastdds/publisher/qos/DataWriterQos.cpp",
            "../../code/src/cpp/fastdds/subscriber/qos/DataReaderQos.cpp",
            "../../code/src/cpp/fastdds/publisher/PublisherImpl.cpp",
            "../../code/src/cpp/fastdds/publisher/qos/PublisherQos.cpp",
            "../../code/src/cpp/fastdds/publisher/Publisher.cpp",
            "../../code/src/cpp/fastdds/subscriber/SubscriberImpl.cpp",
            "../../code/src/cpp/fastdds/subscriber/qos/SubscriberQos.cpp",
            "../../code/src/cpp/fastdds/subscriber/Subscriber.cpp",
            "../../code/src/cpp/fastdds/subscriber/DataReader.cpp",
            "../../code/src/cpp/fastdds/subscriber/DataReaderImpl.cpp",
            "../../code/src/cpp/fastdds/subscriber/history/DataReaderHistory.cpp",
            "../../code/src/cpp/fastdds/domain/DomainParticipantFactory.cpp",
            "../../code/src/cpp/fastdds/domain/DomainParticipantImpl.cpp",
            "../../code/src/cpp/fastdds/domain/DomainParticipant.cpp",
            "../../code/src/cpp/fastdds/domain/qos/DomainParticipantQos.cpp",
            "../../code/src/cpp/fastdds/domain/qos/DomainParticipantFactoryQos.cpp",
            "../../code/src/cpp/fastdds/builtin/typelookup/common/TypeLookupTypes.cpp",
            "../../code/src/cpp/fastdds/builtin/common/RPCHeadersImpl.cpp",
            "../../code/src/cpp/fastdds/builtin/typelookup/TypeLookupManager.cpp",
            "../../code/src/cpp/fastdds/builtin/typelookup/TypeLookupRequestListener.cpp",
            "../../code/src/cpp/fastdds/builtin/typelookup/TypeLookupReplyListener.cpp",
            "../../code/src/cpp/rtps/transport/ChannelResource.cpp",
            "../../code/src/cpp/rtps/transport/UDPChannelResource.cpp",
            "../../code/src/cpp/rtps/transport/TCPChannelResource.cpp",
            "../../code/src/cpp/rtps/transport/TCPChannelResourceBasic.cpp",
            "../../code/src/cpp/rtps/transport/TCPAcceptor.cpp",
            "../../code/src/cpp/rtps/transport/TCPAcceptorBasic.cpp",
            "../../code/src/cpp/rtps/transport/UDPv4Transport.cpp",
            "../../code/src/cpp/rtps/transport/TCPTransportInterface.cpp",
            "../../code/src/cpp/rtps/transport/UDPTransportInterface.cpp",
            "../../code/src/cpp/rtps/transport/shared_mem/SharedMemTransportDescriptor.cpp",
            "../../code/src/cpp/rtps/transport/TCPv4Transport.cpp",
            "../../code/src/cpp/rtps/transport/UDPv6Transport.cpp",
            "../../code/src/cpp/rtps/transport/TCPv6Transport.cpp",
            "../../code/src/cpp/rtps/transport/test_UDPv4Transport.cpp",
            "../../code/src/cpp/rtps/transport/tcp/TCPControlMessage.cpp",
            "../../code/src/cpp/rtps/transport/tcp/RTCPMessageManager.cpp",
            "../../code/src/cpp/rtps/transport/ChainingTransport.cpp",
            "../../code/src/cpp/dynamic-types/AnnotationDescriptor.cpp",
            "../../code/src/cpp/dynamic-types/AnnotationParameterValue.cpp",
            "../../code/src/cpp/dynamic-types/DynamicData.cpp",
            "../../code/src/cpp/dynamic-types/DynamicDataFactory.cpp",
            "../../code/src/cpp/dynamic-types/DynamicType.cpp",
            "../../code/src/cpp/dynamic-types/DynamicPubSubType.cpp",
            "../../code/src/cpp/dynamic-types/DynamicTypePtr.cpp",
            "../../code/src/cpp/dynamic-types/DynamicDataPtr.cpp",
            "../../code/src/cpp/dynamic-types/DynamicTypeBuilder.cpp",
            "../../code/src/cpp/dynamic-types/DynamicTypeBuilderPtr.cpp",
            "../../code/src/cpp/dynamic-types/DynamicTypeBuilderFactory.cpp",
            "../../code/src/cpp/dynamic-types/DynamicTypeMember.cpp",
            "../../code/src/cpp/dynamic-types/MemberDescriptor.cpp",
            "../../code/src/cpp/dynamic-types/TypeDescriptor.cpp",
            "../../code/src/cpp/dynamic-types/TypeIdentifier.cpp",
            "../../code/src/cpp/dynamic-types/TypeIdentifierTypes.cpp",
            "../../code/src/cpp/dynamic-types/TypeObject.cpp",
            "../../code/src/cpp/dynamic-types/TypeObjectHashId.cpp",
            "../../code/src/cpp/dynamic-types/TypeObjectFactory.cpp",
            "../../code/src/cpp/dynamic-types/TypeNamesGenerator.cpp",
            "../../code/src/cpp/dynamic-types/TypesBase.cpp",
            "../../code/src/cpp/dynamic-types/BuiltinAnnotationsTypeObject.cpp",
            "../../code/src/cpp/dynamic-types/DynamicDataHelper.cpp",
            "../../code/src/cpp/utils/string_convert.cpp",
            "../../code/src/cpp/utils/IPFinder.cpp"
        ],
        includes=[
            "../../../../../../src/bsd/rtems-libbsd-5.1/freebsd/include",
            "../../../../../../rtems/6/arm-rtems6/include",
            "../../code/include/",
            "../../code/src/cpp/",
            "../../code/build/include/",
            "../../code/test/mock/rtps/RTPSParticipantImpl/",
            "../../code/thirdparty/boost/include/",
            "../../code/thirdparty/nlohmann-json/",
            "../../code/thirdparty/taocpp-pegtl/",
            "../../../Fast-CDR/code/include/",
            "../../../Fast-CDR/code/build/include/",
            "../../../memory-main/code/include/",
            "../../../memory-main/code/build/src/",
            "../../../memory-main/code/build/src",
            "../../code/asio/include/",
            "../../../tinyxml2/code/include/"],
        defines=[
            "__RTEMS_BSD_API=1",
            "_DEFAULT_SOURCE",
            "__BSD_VISIBLE=5",
            "__INSIDE_RTEMS_BSD_TCPIP_STACK__",
            "__FreeBSD__ ",
            "RTEMS",
            "TLS_FOUND=0",
            "IFF_RUNNING=64",
            "SO_NOSIGPIPE=4130",
            "SO_REUSEPORT=15",
            "ASIO_NO_SSL=1",
            "ASIO_DISABLE_SSL=1",
            "IP_MULTICAST_LOOP=34",
            "IP_ADD_MEMBERSHIP=35",
            "ESHUTDOWN=108",
            "IP_MULTICAST_IF=32",
            "IP_MULTICAST_TTL=33",
            "ASIO_DISABLE_SIGNAL_SET=1",
            "PF_UNSPEC=0",
            "PF_INET=2",
            "PF_INET6=10",
            "SIOCGIFHWADDR=35111",
            "ARPHRD_ETHER=1",
            "ASIO_STANDALONE",
            "ASIO_IPV4_ONLY=1",
            "ASIO_HAS_NO_EXCEPTIONS=1",
            "ASIO_DISABLE_THREADS",
            "ASIO_DISABLE_POSIX_SIGNAL_BLOCKS=1",
            "AF_INET6=0",
            "INET6_ADDRSTRLEN=46",
            "IPV6_MULTICAST_IF=0",
            "IPV6_MULTICAST_HOPS=0",
            "IPV6_MULTICAST_LOOP=0",
            "IPV6_JOIN_GROUP=0",
            "IPV6_LEAVE_GROUP=0",
            "IPV6_UNICAST_HOPS=0",
            "IPV6_V6ONLY=0",
            "ASIO_DISABLE_IPV6=1",
            "ASIO_HAS_IPV6=0",
            "ASIO_NO_IPV6=1",
            "ASIO_DISABLE_IPV6_ITERATOR=1",
            "ASIO_IPV6_ADDRESS_DISABLED=1",
            "ASIO_IPV6_ENDPOINT_DISABLED=1",
            "ASIO_DETAIL_SOCKADDR_IN6_TYPE=void",
            "BOOST_INTERPROCESS_DISABLE_POSIX_FILE_LOCKS=1",
            "BOOST_INTERPROCESS_DISABLE_FILESYSTEM=1",
            "BOOST_INTERPROCESS_POSIX_API=0",
            "ASIO_DISABLE_IPV6=1",
            "ASIO_HAS_IPV6=0",
            "ASIO_HAS_IP_MULTICAST=1",
            "IPPROTO_IPV6=41",
            "IPPROTO_ICMPV6=58",
            "IP_TTL=2",
            "IP_DROP_MEMBERSHIP=36"
        ],
        cxxflags="-Wno-psabi -Wno-return-type -fpermissive -fexceptions -frtti -std=c++17 -Wno-register -g -Wno-unknown-pragmas -fPIC",
        linkflags="-static",
        lib=["bsd", "tinyxml2", "foonathan_memory", "fastcdr", "stdc++", "rtemscpu", "rtemsbsp", "rtemsdefaultconfig", "m", "gcc", "c"],
    )
    ' > wscript
fi

exit 0