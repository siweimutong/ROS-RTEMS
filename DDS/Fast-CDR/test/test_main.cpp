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

// Namespace alias (to simplify code writing)
namespace fastcdr = eprosima::fastcdr;

// 1. Define a custom struct that emulates a DDS message
struct DDSTestMsg {
    uint32_t msg_id;
    uint64_t timestamp;
    float    temperature;
    bool     is_valid;
    std::string device_name;
    char        data_buf[16]; // Fixed-length character array

    // Constructor: initialize default values
    DDSTestMsg() : msg_id(0), timestamp(0), temperature(0.0f), is_valid(false) {
        memset(data_buf, 0, sizeof(data_buf));
    }

    // Print function: verify the encode/decode result
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

// 2. Serialization function: FastCdr + serializeArray (compatible with your version)
bool serialize_dds_msg(const DDSTestMsg& msg, fastcdr::FastBuffer& buffer) {
    // Create a FastCdr object (the core encode/decode class in your version, not the newer Cdr class)
    fastcdr::FastCdr serializer(buffer);

    try {
        // Basic types: serialize directly with the << operator (same usage as before)
        serializer << msg.msg_id;
        serializer << msg.timestamp;
        serializer << msg.temperature;
        serializer << msg.is_valid;
        serializer << msg.device_name;

        // Character array: serialize in bulk with serializeArray (the core bulk write API in your version)
        serializer.serializeArray(msg.data_buf, sizeof(msg.data_buf));

        std::cout << "Serialization succeeded! CDR byte stream length: " << serializer.getSerializedDataLength() << " bytes" << std::endl;
        return true;
    } catch (const eprosima::fastcdr::exception::NotEnoughMemoryException& e) {
        std::cerr << "Serialization failed: " << e.what() << std::endl;
        return false;
    }
}

// 3. Deserialization function: FastCdr + deserializeArray (compatible with your version)
bool deserialize_dds_msg(DDSTestMsg& msg, const fastcdr::FastBuffer& buffer) {
    // Create a FastCdr object bound to the existing buffer
    fastcdr::FastCdr deserializer(const_cast<fastcdr::FastBuffer&>(buffer));

    try {
        // Basic types: deserialize directly with the >> operator (same usage as before)
        deserializer >> msg.msg_id;
        deserializer >> msg.timestamp;
        deserializer >> msg.temperature;
        deserializer >> msg.is_valid;
        deserializer >> msg.device_name;

        // Character array: deserialize in bulk with deserializeArray (the core bulk read API in your version)
        deserializer.deserializeArray(msg.data_buf, sizeof(msg.data_buf));

        std::cout << "Deserialization succeeded!" << std::endl;
        return true;
    } catch (const eprosima::fastcdr::exception::NotEnoughMemoryException& e) {
        std::cerr << "Deserialization failed: " << e.what() << std::endl;
        return false;
    }
}

extern "C" rtems_task Init(
  rtems_task_argument ignored
)
{
    // Step 1: initialize the DDS message to be serialized
    DDSTestMsg send_msg;
    send_msg.msg_id = 10086;
    send_msg.timestamp = 1735689600000;
    send_msg.temperature = 25.6f;
    send_msg.is_valid = true;
    send_msg.device_name = "Sensor_001";
    strcpy(send_msg.data_buf, "Hello_FAST_CDR");
    send_msg.print("Original DDS message");

    // Step 2: create a dynamic FAST CDR buffer
    fastcdr::FastBuffer cdr_buffer;

    // Step 3: perform serialization
    if (!serialize_dds_msg(send_msg, cdr_buffer)) {
        exit(-1);
    }

    // Step 4: initialize the receiving message struct
    DDSTestMsg recv_msg;

    // Step 5: perform deserialization
    if (!deserialize_dds_msg(recv_msg, cdr_buffer)) {
        exit(-1);
    }

    // Step 6: print the deserialized result and verify consistency
    recv_msg.print("Deserialized DDS message");

    bool is_consistent = (send_msg.msg_id == recv_msg.msg_id) &&
                         (send_msg.timestamp == recv_msg.timestamp) &&
                         (abs(send_msg.temperature - recv_msg.temperature) < 1e-6) &&
                         (send_msg.is_valid == recv_msg.is_valid) &&
                         (send_msg.device_name == recv_msg.device_name) &&
                         (strcmp(send_msg.data_buf, recv_msg.data_buf) == 0);

    if (is_consistent) {
        std::cout << "✅ Encode/decode consistency check passed! FAST CDR works correctly." << std::endl;
    } else {
        std::cerr << "❌ Encode/decode consistency check failed!" << std::endl;
        exit(-1);
    }

}