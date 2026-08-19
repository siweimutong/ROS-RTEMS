
#include "get_func_pointer_by_name.hpp"
#include "rmw_dds_common/msg/detail/participant_entities_info__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/msg/detail/parameter_event__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/msg/detail/parameter_event__rosidl_typesupport_fastrtps_cpp.hpp"
#include "std_msgs/msg/detail/int32__rosidl_typesupport_fastrtps_cpp.hpp"
#include "std_msgs/msg/detail/int32__rosidl_typesupport_introspection_cpp.hpp"
#include "std_msgs/msg/detail/string__rosidl_typesupport_fastrtps_cpp.hpp"
#include "std_msgs/msg/detail/string__rosidl_typesupport_introspection_cpp.hpp"

#include "rcl_interfaces/srv/detail/get_parameters__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/srv/detail/get_parameter_types__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/srv/detail/set_parameters__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/srv/detail/set_parameters_atomically__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/srv/detail/describe_parameters__rosidl_typesupport_fastrtps_cpp.hpp"
#include "rcl_interfaces/srv/detail/list_parameters__rosidl_typesupport_fastrtps_cpp.hpp"

ObtainPointerCpp* ObtainPointerCpp::instance_ = nullptr;


#ifdef __cplusplus
extern "C"
{
#endif

const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo(void);
const rosidl_message_type_support_t *rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo(void);
const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rcl_interfaces__msg__ParameterEvent(void);
const rosidl_message_type_support_t *rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rcl_interfaces__msg__ParameterEvent(void);
const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__std_msgs__msg__Int32(void);
const rosidl_message_type_support_t *rosidl_typesupport_introspection_cpp__get_message_type_support_handle__std_msgs__msg__Int32(void);
const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__std_msgs__msg__String(void);
const rosidl_message_type_support_t *rosidl_typesupport_introspection_cpp__get_message_type_support_handle__std_msgs__msg__String(void);


const rosidl_service_type_support_t *rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__GetParameters(void);
const rosidl_service_type_support_t *rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__GetParameterTypes(void);
const rosidl_service_type_support_t *rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__SetParameters(void);
const rosidl_service_type_support_t *rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__SetParametersAtomically(void);
const rosidl_service_type_support_t *rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__DescribeParameters(void);
const rosidl_service_type_support_t *rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__ListParameters(void);

#ifdef __cplusplus
}
#endif

ObtainPointerCpp::ObtainPointerCpp(){
    // Initialization: bind target strings to function pointers
    registerMsgFunc(
        "rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo",
        rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo
    );

    registerMsgFunc(
        "rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo",
        rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo
    );

    registerMsgFunc(
        "rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rcl_interfaces__msg__ParameterEvent",
        rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rcl_interfaces__msg__ParameterEvent
    );

    registerMsgFunc(
        "rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rcl_interfaces__msg__ParameterEvent",
        rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rcl_interfaces__msg__ParameterEvent
    );

    registerMsgFunc(
        "rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__std_msgs__msg__Int32",
        rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__std_msgs__msg__Int32
    );

    registerMsgFunc(
        "rosidl_typesupport_introspection_cpp__get_message_type_support_handle__std_msgs__msg__Int32",
        rosidl_typesupport_introspection_cpp__get_message_type_support_handle__std_msgs__msg__Int32
    );

    registerMsgFunc(
        "rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__std_msgs__msg__String",
        rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__std_msgs__msg__String
    );

    registerMsgFunc(
        "rosidl_typesupport_introspection_cpp__get_message_type_support_handle__std_msgs__msg__String",
        rosidl_typesupport_introspection_cpp__get_message_type_support_handle__std_msgs__msg__String
    );





    registerSrvFunc(
        "rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__GetParameters",
        rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__GetParameters
    );

    registerSrvFunc(
        "rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__GetParameterTypes",
        rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__GetParameterTypes
    );

    registerSrvFunc(
        "rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__SetParameters",
        rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__SetParameters
    );

    registerSrvFunc(
        "rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__SetParametersAtomically",
        rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__SetParametersAtomically
    );

    registerSrvFunc(
        "rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__DescribeParameters",
        rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__DescribeParameters
    );

    registerSrvFunc(
        "rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__ListParameters",
        rosidl_typesupport_fastrtps_cpp__get_service_type_support_handle__rcl_interfaces__srv__ListParameters
    );
};

ObtainPointerCpp::~ObtainPointerCpp(){
    if (instance_){
        delete instance_;
    }else{
        printf("Warning: instance_ was not destroyed in its destructor; it may have been destroyed externally. Check whether the destruction location is legitimate.\r\n");
    }
}

void ObtainPointerCpp::registerMsgFunc(const std::string& funcName, MsgTypeSupportFunc func) {
    msgFuncMap_[funcName] = func;
}

void ObtainPointerCpp::registerSrvFunc(const std::string& funcName, SrvTypeSupportFunc func) {
    srvFuncMap_[funcName] = func;
}

MsgTypeSupportFunc ObtainPointerCpp::getMsgFunc(const std::string& funcName) {
    auto it = msgFuncMap_.find(funcName);
    return (it != msgFuncMap_.end()) ? it->second : nullptr;
}

SrvTypeSupportFunc ObtainPointerCpp::getSrvFunc(const std::string& funcName) {
    auto it = srvFuncMap_.find(funcName);
    return (it != srvFuncMap_.end()) ? it->second : nullptr;
}


int ObtainPointerCpp::isMsgOrSrv(const std::string& funcName){
    // 1. Define the target substrings
    const std::string msg_substr = "get_message_type_support";
    const std::string srv_substr = "get_service_type_support";

    // 2. Search for the substrings using std::string::find
    // find returns the index of the first occurrence; std::string::npos means not found
    size_t pos = funcName.find(msg_substr);

    if(pos != std::string::npos) return 0;

    pos = funcName.find(srv_substr);

    if(pos != std::string::npos) return 1;

    // Neither target substring was found in the string; this is currently treated as an abnormal case
    throw std::runtime_error(
        "ObtainPointerCpp::isMsgOrSrv error: the function name contains neither the msg nor the srv substring! funcName = " + funcName
    );
}