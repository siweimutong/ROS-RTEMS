#ifndef RCLCPP__NODE_INTERFACES__GET_NODE_TIMERS_INTERFACE_HPP_
#define RCLCPP__NODE_INTERFACES__GET_NODE_TIMERS_INTERFACE_HPP_

#include "rclcpp/node_interfaces/node_timers_interface.hpp"
#include "rclcpp/macros.hpp"

namespace rclcpp
{
namespace node_interfaces
{

template<typename NodeT>
rclcpp::node_interfaces::NodeTimersInterface::SharedPtr
get_node_timers_interface(NodeT && node)
{
  return node->get_node_timers_interface();
}

inline
rclcpp::node_interfaces::NodeTimersInterface::SharedPtr
get_node_timers_interface(rclcpp::node_interfaces::NodeTimersInterface::SharedPtr & node_timers)
{
  return node_timers;
}

}  // namespace node_interfaces
}  // namespace rclcpp

#endif  // RCLCPP__NODE_INTERFACES__GET_NODE_TIMERS_INTERFACE_HPP_
