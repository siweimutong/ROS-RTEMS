#ifndef RCLCPP__NODE_INTERFACES__GET_NODE_TOPICS_INTERFACE_HPP_
#define RCLCPP__NODE_INTERFACES__GET_NODE_TOPICS_INTERFACE_HPP_

#include "rclcpp/node_interfaces/node_topics_interface.hpp"
#include "rclcpp/macros.hpp"

namespace rclcpp
{
namespace node_interfaces
{

template<typename NodeT>
rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr
get_node_topics_interface(NodeT && node)
{
  return node.get_node_topics_interface();
}

inline
rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr
get_node_topics_interface(rclcpp::node_interfaces::NodeTopicsInterface::SharedPtr & node_topics)
{
  return node_topics;
}

}  // namespace node_interfaces
}  // namespace rclcpp

#endif  // RCLCPP__NODE_INTERFACES__GET_NODE_TOPICS_INTERFACE_HPP_
