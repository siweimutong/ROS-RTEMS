#ifndef RCLCPP__NODE_INTERFACES__GET_NODE_BASE_INTERFACE_HPP_
#define RCLCPP__NODE_INTERFACES__GET_NODE_BASE_INTERFACE_HPP_

#include "rclcpp/node_interfaces/node_base_interface.hpp"
#include "rclcpp/macros.hpp"

namespace rclcpp
{
namespace node_interfaces
{

template<typename NodeT>
rclcpp::node_interfaces::NodeBaseInterface::SharedPtr
get_node_base_interface(NodeT && node)
{
  return node->get_node_base_interface();
}

}  // namespace node_interfaces
}  // namespace rclcpp

#endif  // RCLCPP__NODE_INTERFACES__GET_NODE_BASE_INTERFACE_HPP_
