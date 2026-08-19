
#include "get_func_pointer_by_name.hpp"
#include "rcl_interfaces/msg/detail/log__rosidl_typesupport_fastrtps_c.h"
#include "builtin_interfaces/msg/detail/time__rosidl_typesupport_fastrtps_c.h"
#include "rmw_dds_common/msg/detail/participant_entities_info__rosidl_typesupport_fastrtps_cpp.hpp"

ObtainPointerC* ObtainPointerC::instance_ = nullptr;


#ifdef __cplusplus
extern "C"
{
#endif

const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_c__get_message_type_support_handle__rcl_interfaces__msg__Log(void);
const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_c__get_message_type_support_handle__builtin_interfaces__msg__Time(void);
const rosidl_message_type_support_t *rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo(void);
const rosidl_message_type_support_t *rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo(void);
const rosidl_message_type_support_t *rosidl_typesupport_introspection_c__get_message_type_support_handle__rcl_interfaces__msg__Log(void);


#ifdef __cplusplus
}
#endif

ObtainPointerC::ObtainPointerC(){
    // Initialization: bind target strings to function pointers
    registerMsgFunc(
        "rosidl_typesupport_fastrtps_c__get_message_type_support_handle__rcl_interfaces__msg__Log",
        rosidl_typesupport_fastrtps_c__get_message_type_support_handle__rcl_interfaces__msg__Log
    );

    registerMsgFunc(
        "rosidl_typesupport_fastrtps_c__get_message_type_support_handle__builtin_interfaces__msg__Time",
        rosidl_typesupport_fastrtps_c__get_message_type_support_handle__builtin_interfaces__msg__Time
    );

    registerMsgFunc(
        "rosidl_typesupport_introspection_c__get_message_type_support_handle__rcl_interfaces__msg__Log",
        rosidl_typesupport_introspection_c__get_message_type_support_handle__rcl_interfaces__msg__Log
    );

    registerMsgFunc(
        "rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo",
        rosidl_typesupport_fastrtps_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo
    );

    registerMsgFunc(
        "rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo",
        rosidl_typesupport_introspection_cpp__get_message_type_support_handle__rmw_dds_common__msg__ParticipantEntitiesInfo
    );
};

ObtainPointerC::~ObtainPointerC(){
    if (instance_){
        delete instance_;
    }else{
        printf("Warning: instance_ was not destroyed in its destructor; it may have been destroyed externally. Check whether the destruction location is legitimate.\r\n");
    }
}

void ObtainPointerC::registerMsgFunc(const std::string& funcName, MsgTypeSupportFunc func) {
    msgFuncMap_[funcName] = func;
}

void ObtainPointerC::registerSrvFunc(const std::string& funcName, SrvTypeSupportFunc func) {
    srvFuncMap_[funcName] = func;
}

MsgTypeSupportFunc ObtainPointerC::getMsgFunc(const std::string& funcName) {
    auto it = msgFuncMap_.find(funcName);
    return (it != msgFuncMap_.end()) ? it->second : nullptr;
}

SrvTypeSupportFunc ObtainPointerC::getSrvFunc(const std::string& funcName) {
    auto it = srvFuncMap_.find(funcName);
    return (it != srvFuncMap_.end()) ? it->second : nullptr;
}

int ObtainPointerC::isMsgOrSrv(const std::string& funcName){
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