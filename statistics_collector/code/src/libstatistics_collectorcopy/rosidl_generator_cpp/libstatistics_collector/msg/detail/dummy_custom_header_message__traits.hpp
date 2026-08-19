// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from libstatistics_collector:msg/DummyCustomHeaderMessage.idl
// generated code does not contain a copyright notice

#ifndef LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_CUSTOM_HEADER_MESSAGE__TRAITS_HPP_
#define LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_CUSTOM_HEADER_MESSAGE__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "libstatistics_collector/msg/detail/dummy_custom_header_message__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

// Include directives for member types
// Member 'header'
#include "std_msgs/msg/detail/color_rgba__traits.hpp"

namespace libstatistics_collector
{

namespace msg
{

inline void to_flow_style_yaml(
  const DummyCustomHeaderMessage & msg,
  std::ostream & out)
{
  out << "{";
  // member: header
  {
    out << "header: ";
    to_flow_style_yaml(msg.header, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const DummyCustomHeaderMessage & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: header
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "header:\n";
    to_block_style_yaml(msg.header, out, indentation + 2);
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const DummyCustomHeaderMessage & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace libstatistics_collector

namespace rosidl_generator_traits
{

[[deprecated("use libstatistics_collector::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const libstatistics_collector::msg::DummyCustomHeaderMessage & msg,
  std::ostream & out, size_t indentation = 0)
{
  libstatistics_collector::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use libstatistics_collector::msg::to_yaml() instead")]]
inline std::string to_yaml(const libstatistics_collector::msg::DummyCustomHeaderMessage & msg)
{
  return libstatistics_collector::msg::to_yaml(msg);
}

template<>
inline const char * data_type<libstatistics_collector::msg::DummyCustomHeaderMessage>()
{
  return "libstatistics_collector::msg::DummyCustomHeaderMessage";
}

template<>
inline const char * name<libstatistics_collector::msg::DummyCustomHeaderMessage>()
{
  return "libstatistics_collector/msg/DummyCustomHeaderMessage";
}

template<>
struct has_fixed_size<libstatistics_collector::msg::DummyCustomHeaderMessage>
  : std::integral_constant<bool, has_fixed_size<std_msgs::msg::ColorRGBA>::value> {};

template<>
struct has_bounded_size<libstatistics_collector::msg::DummyCustomHeaderMessage>
  : std::integral_constant<bool, has_bounded_size<std_msgs::msg::ColorRGBA>::value> {};

template<>
struct is_message<libstatistics_collector::msg::DummyCustomHeaderMessage>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // LIBSTATISTICS_COLLECTOR__MSG__DETAIL__DUMMY_CUSTOM_HEADER_MESSAGE__TRAITS_HPP_
