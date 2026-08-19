/*
 * intra.cpp
 *
 *  Created on: 2024Äê7ÔÂ23ÈÕ
 *      Author: 14609
 */






#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
using std::placeholders::_1;
using namespace std::chrono_literals;

class MyNode final : public rclcpp::Node {
public:
  MyNode()
      : Node("helloworld_cpp_intra", rclcpp::NodeOptions().use_intra_process_comms(true)), qos(10),
        count_(0) {
    options.intra_process_buffer_type = rclcpp::IntraProcessBufferType::SharedPtr;
    auto options2 = rclcpp::SubscriptionOptions();
    options2.intra_process_buffer_type = rclcpp::IntraProcessBufferType::SharedPtr;
    auto group1 = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    auto group2 = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    auto group3 = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    auto group4 = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    auto group5 = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    options.callback_group = group1;
    // options2.callback_group = group2;
    publisher_1 = create_publisher<std_msgs::msg::String>("topic", qos, options_pub);
    // publisher_2 = create_publisher<Student>("topic", 2);
    subscription_1 = create_subscription<std_msgs::msg::String>(
        "topic", 10, std::bind(&MyNode::topic_callback_1, this, _1), options);
    options.callback_group = group3;
    subscription_3 = create_subscription<std_msgs::msg::String>(
        "topic", 10, std::bind(&MyNode::topic_callback_3, this, _1), options);
    options.callback_group = group4;
    subscription_4 = create_subscription<std_msgs::msg::String>(
        "topic", 10, std::bind(&MyNode::topic_callback_4, this, _1), options);
    options.callback_group = group2;
    subscription_2 = create_subscription<std_msgs::msg::String>(
        "topic", 10, std::bind(&MyNode::topic_callback_2, this, _1), options);
    options.callback_group = group5;
    subscription_5 = create_subscription<std_msgs::msg::String>(
        "topic", 10, std::bind(&MyNode::topic_callback_5, this, _1), options);
    timer_ = create_wall_timer(500ms, std::bind(&MyNode::timer_callback, this));
  }

private:
  void timer_callback() {
    auto message = std_msgs::msg::String::UniquePtr(new std_msgs::msg::String());
    message->data = std::to_string(count_++);
    std::cout << "Publishing: " << reinterpret_cast<std::uintptr_t>(message.get()) << std::endl;
    publisher_1->publish(std::move(message));
  }
  void topic_callback_1(std_msgs::msg::String::ConstSharedPtr msg) {
    std::cout << "I heard: 1 message = " << msg->data.c_str() << " "
              << reinterpret_cast<std::uintptr_t>(msg.get()) << " " << msg.use_count() << std::endl;
    // if (msg->data == "15") {
    //   subscription_6 =
    //       create_subscription<Student>("topic", qos,
    //       std::bind(&MyNode::topic_callback_6, this, _1), options);
    // }
  }
  void topic_callback_3(std_msgs::msg::String::ConstSharedPtr msg) {
    std::cout << "I heard: 3 message = " << msg->data.c_str() << " "
              << reinterpret_cast<std::uintptr_t>(msg.get()) << " " << msg.use_count() << std::endl;
  }
  void topic_callback_2(std_msgs::msg::String::ConstSharedPtr msg) {
    std::cout << "I heard: 2 message = " << msg->data.c_str() << " "
              << reinterpret_cast<std::uintptr_t>(msg.get()) << " " << msg.use_count() << std::endl;
  }
  void topic_callback_4(std_msgs::msg::String::ConstSharedPtr msg) {
    std::cout << "I heard: 4 message = " << msg->data.c_str() << " "
              << reinterpret_cast<std::uintptr_t>(msg.get()) << " " << msg.use_count() << std::endl;
  }
  void topic_callback_5(std_msgs::msg::String::ConstSharedPtr msg) {
    std::cout << "I heard: 5 message = " << msg->data.c_str() << " "
              << reinterpret_cast<std::uintptr_t>(msg.get()) << " " << msg.use_count() << std::endl;
  }
  void topic_callback_6(std_msgs::msg::String::ConstSharedPtr msg) {
    std::cout << "I heard: 6 message = " << msg->data.c_str() << " "
              << reinterpret_cast<std::uintptr_t>(msg.get()) << " " << msg.use_count() << std::endl;
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::SubscriptionOptions options;
  rclcpp::PublisherOptions options_pub;
  rclcpp::QoS qos;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_1;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_2;
  rclcpp::Subscription<std_msgs::msg::String>::ConstSharedPtr subscription_1;
  rclcpp::Subscription<std_msgs::msg::String>::ConstSharedPtr subscription_3;
  rclcpp::Subscription<std_msgs::msg::String>::ConstSharedPtr subscription_4;
  rclcpp::Subscription<std_msgs::msg::String>::ConstSharedPtr subscription_2;
  rclcpp::Subscription<std_msgs::msg::String>::ConstSharedPtr subscription_5;
  rclcpp::Subscription<std_msgs::msg::String>::ConstSharedPtr subscription_6;
  size_t count_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor executor;
  auto node = std::make_shared<MyNode>();
  executor.add_node(node);
  executor.spin();

  // auto p = std::make_shared<Student>();
  // auto t = test();
  // sleep(2);
  return 0;
}

