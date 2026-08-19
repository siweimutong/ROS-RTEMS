#ifndef RCLCPP__NODE_INTERFACES__GET_NODE_PARAMETERS_INTERFACE_HPP_
#define RCLCPP__NODE_INTERFACES__GET_NODE_PARAMETERS_INTERFACE_HPP_

#include "rclcpp/node_interfaces/node_parameters_interface.hpp"
#include "rclcpp/macros.hpp"

namespace rclcpp
{
namespace node_interfaces
{

template<typename NodeT>
rclcpp::node_interfaces::NodeParametersInterface::SharedPtr
get_node_parameters_interface(NodeT && node)
{
  return node.get_node_parameters_interface();
}

inline
rclcpp::node_interfaces::NodeParametersInterface::SharedPtr
get_node_parameters_interface(rclcpp::node_interfaces::NodeParametersInterface::SharedPtr & node_parameters)
{
  return node_parameters;
}

}  // namespace node_interfaces
}  // namespace rclcpp

#endif  // RCLCPP__NODE_INTERFACES__GET_NODE_PARAMETERS_INTERFACE_HPP_
