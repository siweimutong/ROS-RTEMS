#ifndef GET_FUNC_POINTER_BY_NAME_HPP
#define GET_FUNC_POINTER_BY_NAME_HPP

/**
    This layer targets ROS's underlying use of dlopen to load dynamic libraries; however, sometimes we need to operate on static libraries, and static libraries cannot be loaded into shared memory via dlopen, so this layer is added
    to obtain the call site of a function by its name; this class is built using the singleton pattern
*/

#include <iostream>
#include <unordered_map>
#include <string>

#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rmw_dds_common/msg/participant_entities_info.hpp"
#include "rosidl_runtime_c/service_type_support_struct.h"



using MsgTypeSupportFunc = const rosidl_message_type_support_t * (*)(void);

using SrvTypeSupportFunc = const rosidl_service_type_support_t * (*)(void);


class ObtainPointerC{
private:
    ObtainPointerC();

public:
    ~ObtainPointerC();

    static ObtainPointerC* getInstance(){
        if (!instance_){
            instance_ = new ObtainPointerC();
        }
        return instance_;
    }

    ObtainPointerC(const ObtainPointerC&) = delete;
    ObtainPointerC& operator=(const ObtainPointerC&) = delete;

    // The code below is required for the business logic

    // Register function: string -> function pointer (extensible to add more functions)
    void registerMsgFunc(const std::string& funcName, MsgTypeSupportFunc func);

    void registerSrvFunc(const std::string& funcName, SrvTypeSupportFunc func);

    // Query function: get the function call site by string (core interface)
    MsgTypeSupportFunc getMsgFunc(const std::string& funcName);

    SrvTypeSupportFunc getSrvFunc(const std::string& funcName);

    int isMsgOrSrv(const std::string& funcName);

private:
    static ObtainPointerC* instance_;
    std::unordered_map<std::string, MsgTypeSupportFunc> msgFuncMap_;
    std::unordered_map<std::string, SrvTypeSupportFunc> srvFuncMap_;
};

#endif // GET_FUNC_POINTER_BY_NAME_HPP