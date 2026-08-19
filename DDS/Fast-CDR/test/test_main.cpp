/*
 * Hello world example
 */
#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>

#include "tinyxml2.h"

#include <fastcdr/Cdr.h>
#include <fastcdr/FastCdr.h>
#include <iostream>
#include <cstring>
#include <string>

using namespace tinyxml2;
using namespace std;

// 命名空间别名（简化代码书写）
namespace fastcdr = eprosima::fastcdr;

// 1. 定义模拟 DDS 消息的自定义结构体
struct DDSTestMsg {
    uint32_t msg_id;
    uint64_t timestamp;
    float    temperature;
    bool     is_valid;
    std::string device_name;
    char        data_buf[16]; // 固定长度字符数组

    // 构造函数：初始化默认值
    DDSTestMsg() : msg_id(0), timestamp(0), temperature(0.0f), is_valid(false) {
        memset(data_buf, 0, sizeof(data_buf));
    }

    // 打印函数：验证编解码结果
    void print(const std::string& tip) const {
        std::cout << "===== " << tip << " =====" << std::endl;
        std::cout << "msg_id: " << msg_id << std::endl;
        std::cout << "timestamp: " << timestamp << std::endl;
        std::cout << "temperature: " << temperature << std::endl;
        std::cout << "is_valid: " << (is_valid ? "true" : "false") << std::endl;
        std::cout << "device_name: " << device_name << std::endl;
        std::cout << "data_buf: " << data_buf << std::endl;
        std::cout << "==========================" << std::endl << std::endl;
    }
};

// 2. 序列化函数：使用 FastCdr + serializeArray（兼容你的版本）
bool serialize_dds_msg(const DDSTestMsg& msg, fastcdr::FastBuffer& buffer) {
    // 创建 FastCdr 对象（你当前版本的核心编解码类，非高版本 Cdr 类）
    fastcdr::FastCdr serializer(buffer);

    try {
        // 基础类型：直接使用 << 运算符序列化（与之前用法一致）
        serializer << msg.msg_id;
        serializer << msg.timestamp;
        serializer << msg.temperature;
        serializer << msg.is_valid;
        serializer << msg.device_name;

        // 字符数组：使用 serializeArray 批量序列化（你版本的核心批量写入接口）
        serializer.serializeArray(msg.data_buf, sizeof(msg.data_buf));

        std::cout << "序列化成功！CDR 字节流长度：" << serializer.getSerializedDataLength() << " 字节" << std::endl;
        return true;
    } catch (const eprosima::fastcdr::exception::NotEnoughMemoryException& e) {
        std::cerr << "序列化失败：" << e.what() << std::endl;
        return false;
    }
}

// 3. 反序列化函数：使用 FastCdr + deserializeArray（兼容你的版本）
bool deserialize_dds_msg(DDSTestMsg& msg, const fastcdr::FastBuffer& buffer) {
    // 创建 FastCdr 对象，绑定已有缓冲区
    fastcdr::FastCdr deserializer(const_cast<fastcdr::FastBuffer&>(buffer));

    try {
        // 基础类型：直接使用 >> 运算符反序列化（与之前用法一致）
        deserializer >> msg.msg_id;
        deserializer >> msg.timestamp;
        deserializer >> msg.temperature;
        deserializer >> msg.is_valid;
        deserializer >> msg.device_name;

        // 字符数组：使用 deserializeArray 批量反序列化（你版本的核心批量读取接口）
        deserializer.deserializeArray(msg.data_buf, sizeof(msg.data_buf));

        std::cout << "反序列化成功！" << std::endl;
        return true;
    } catch (const eprosima::fastcdr::exception::NotEnoughMemoryException& e) {
        std::cerr << "反序列化失败：" << e.what() << std::endl;
        return false;
    }
}

extern "C" rtems_task Init(
  rtems_task_argument ignored
)
{
    // 步骤1：初始化待序列化的 DDS 消息
    DDSTestMsg send_msg;
    send_msg.msg_id = 10086;
    send_msg.timestamp = 1735689600000;
    send_msg.temperature = 25.6f;
    send_msg.is_valid = true;
    send_msg.device_name = "Sensor_001";
    strcpy(send_msg.data_buf, "Hello_FAST_CDR");
    send_msg.print("原始 DDS 消息");

    // 步骤2：创建 FAST CDR 动态缓冲区
    fastcdr::FastBuffer cdr_buffer;

    // 步骤3：执行序列化操作
    if (!serialize_dds_msg(send_msg, cdr_buffer)) {
        exit(-1);
    }

    // 步骤4：初始化接收消息结构体
    DDSTestMsg recv_msg;

    // 步骤5：执行反序列化操作
    if (!deserialize_dds_msg(recv_msg, cdr_buffer)) {
        exit(-1);
    }

    // 步骤6：打印反序列化结果并校验一致性
    recv_msg.print("反序列化后 DDS 消息");

    bool is_consistent = (send_msg.msg_id == recv_msg.msg_id) &&
                         (send_msg.timestamp == recv_msg.timestamp) &&
                         (abs(send_msg.temperature - recv_msg.temperature) < 1e-6) &&
                         (send_msg.is_valid == recv_msg.is_valid) &&
                         (send_msg.device_name == recv_msg.device_name) &&
                         (strcmp(send_msg.data_buf, recv_msg.data_buf) == 0);

    if (is_consistent) {
        std::cout << "✅ 编解码一致性校验通过！FAST CDR 功能正常。" << std::endl;
    } else {
        std::cerr << "❌ 编解码一致性校验失败！" << std::endl;
        exit(-1);
    }

}