
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
    // 初始化：绑定目标字符串与函数指针
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
        printf("警告：instance_实例的销毁不是在其析构函数中进行的，可能是在外部进行的，注意实例销毁的位置是否合法\r\n");
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
    // 1. 定义目标子串
    const std::string msg_substr = "get_message_type_support";
    const std::string srv_substr = "get_service_type_support";

    // 2. 使用 std::string::find 查找子串
    // find 返回子串首次出现的索引，std::string::npos 表示未找到
    size_t pos = funcName.find(msg_substr);

    if(pos != std::string::npos) return 0;

    pos = funcName.find(srv_substr);

    if(pos != std::string::npos) return 1;

    // 两个要查找的字符串都不在被查找的字符串中，目前看来暂时是异常情况
    throw std::runtime_error(
        "ObtainPointerCpp::isMsgOrSrv 错误：函数名既不包含 msg 也不包含 srv 子串！funcName = " + funcName
    );
}