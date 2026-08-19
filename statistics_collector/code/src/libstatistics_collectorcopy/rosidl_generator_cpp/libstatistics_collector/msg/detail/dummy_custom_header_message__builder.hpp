// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from libstatistics_collector:msg/DummyCustomHeaderMessage.idl
// generated code does not contain a copyright notice

#ifndef LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_CUSTOM_HEADER_MESSAGE__BUILDER_HPP_
#define LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_CUSTOM_HEADER_MESSAGE__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "libstatistics_collector/msg/detail/dummy_custom_header_message__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace libstatistics_collector
{

namespace msg
{

namespace builder
{

class Init_DummyCustomHeaderMessage_header
{
public:
  Init_DummyCustomHeaderMessage_header()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::libstatistics_collector::msg::DummyCustomHeaderMessage header(::libstatistics_collector::msg::DummyCustomHeaderMessage::_header_type arg)
  {
    msg_.header = std::move(arg);
    return std::move(msg_);
  }

private:
  ::libstatistics_collector::msg::DummyCustomHeaderMessage msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::libstatistics_collector::msg::DummyCustomHeaderMessage>()
{
  return libstatistics_collector::msg::builder::Init_DummyCustomHeaderMessage_header();
}

}  // namespace libstatistics_collector

#endif  // LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_CUSTOM_HEADER_MESSAGE__BUILDER_HPP_
