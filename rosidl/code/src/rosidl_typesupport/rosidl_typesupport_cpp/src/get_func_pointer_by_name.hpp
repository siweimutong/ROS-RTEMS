#ifndef GET_FUNC_POINTER_BY_NAME_HPP
#define GET_FUNC_POINTER_BY_NAME_HPP

/**
    这一层是针对ros底层自带的使用dlopen打开动态库，但是有时候需要基于静态库操作，然而静态库并不能使用dlopen加载到共享内存上，所以需要添加这一层的结构，
    用于通过func的name获取func的调用点，这个类的构建需要使用到单例模式
*/

#include <iostream>
#include <unordered_map>
#include <string>

#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rmw_dds_common/msg/participant_entities_info.hpp"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

using MsgTypeSupportFunc = const rosidl_message_type_support_t * (*)(void);

using SrvTypeSupportFunc = const rosidl_service_type_support_t * (*)(void);

class ObtainPointerCpp{
private:
    ObtainPointerCpp();

public:
    ~ObtainPointerCpp();

    static ObtainPointerCpp* getInstance(){
        if (!instance_){
            instance_ = new ObtainPointerCpp();
        }
        return instance_;
    }

    ObtainPointerCpp(const ObtainPointerCpp&) = delete;
    ObtainPointerCpp& operator=(const ObtainPointerCpp&) = delete;

    // 一下都是些业务需要的代码

    // 注册函数：字符串 → 函数指针（可扩展添加更多函数）
    void registerMsgFunc(const std::string& funcName, MsgTypeSupportFunc func);

    void registerSrvFunc(const std::string& funcName, SrvTypeSupportFunc func);

    // 查询函数：通过字符串获取函数调用点（核心接口）
    MsgTypeSupportFunc getMsgFunc(const std::string& funcName);

    SrvTypeSupportFunc getSrvFunc(const std::string& funcName);

    // 判断当前传入的字符串是什么类型的，如果是messager那就返回0，如果是service返回1
    int isMsgOrSrv(const std::string& funcName);



private:
    static ObtainPointerCpp* instance_;
    std::unordered_map<std::string, MsgTypeSupportFunc> msgFuncMap_;
    std::unordered_map<std::string, SrvTypeSupportFunc> srvFuncMap_;
};

#endif // GET_FUNC_POINTER_BY_NAME_HPP