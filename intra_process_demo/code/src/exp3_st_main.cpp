/**
 * @file exp3_st_main.cpp
 *
 * Exp3-ST: Task Chain Latency — SingleThreadedExecutor
 *
 * A chain of n tasks using intra-process pub/sub:
 *   Node 0: Timer (time-triggered) -> publishes chain_start_ns
 *   Node 1: Subscriber -> does work -> publishes chain_start_ns
 *   ...
 *   Node n-1: Subscriber (last) -> does work -> records e2e latency
 *
 * Compile with -DCHAIN_LENGTH=N (N = 1..5)
 */

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int64.hpp>
#include <chrono>
#include <memory>
#include <cstdio>
#include <cinttypes>
#include <atomic>
#include <vector>

#include <rclcpp/executors/single_threaded_executor.hpp>

using namespace std::chrono_literals;

#ifndef CHAIN_LENGTH
#define CHAIN_LENGTH 2
#endif

#define TIMER_PERIOD_MS   200
#define TOTAL_ROUNDS      50

static int64_t rcl_clock_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

struct E2EStats
{
  int64_t  min_ns;
  int64_t  max_ns;
  int64_t  sum_ns;
  uint32_t count;

  E2EStats()
  : min_ns(INT64_MAX), max_ns(INT64_MIN), sum_ns(0), count(0) {}

  void update(int64_t val)
  {
    if (val < min_ns) min_ns = val;
    if (val > max_ns) max_ns = val;
    sum_ns += val;
    count++;
  }

  int64_t avg() const
  {
    return count > 0 ? sum_ns / (int64_t)count : 0;
  }
};

static uint32_t g_round = 0;
static std::atomic<bool> g_done{false};
static E2EStats g_e2e_stats;

/* ---- Node 0: Timer Producer ---- */

struct TimerNode : public rclcpp::Node
{
  TimerNode()
  : Node("chain_0", rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(TIMER_PERIOD_MS),
      [this]() { this->on_timer(); }
    );

    if (CHAIN_LENGTH > 1) {
      pub_ = this->create_publisher<std_msgs::msg::Int64>("chain_0", 10);
    }
  }

  void on_timer()
  {
    if (g_done.load() || g_round >= TOTAL_ROUNDS) { return; }

    int64_t start_ns = rcl_clock_ns();

    volatile int dummy = 0;
    for (int j = 0; j < 100; j++) { dummy += j; }
    (void)dummy;

    if (CHAIN_LENGTH == 1) {
      int64_t e2e = rcl_clock_ns() - start_ns;
      g_e2e_stats.update(e2e);
      printf("[E3ST-CSV] %d,%u,%lld\n",
             CHAIN_LENGTH, g_round, (long long)e2e);
      g_round++;
      if (g_round >= TOTAL_ROUNDS && !g_done.exchange(true)) {
        printf("[E3ST-SUMMARY] chain_len=%d e2e_min=%lldns e2e_max=%lldns e2e_avg=%lldns\n",
               CHAIN_LENGTH,
               (long long)g_e2e_stats.min_ns,
               (long long)g_e2e_stats.max_ns,
               (long long)g_e2e_stats.avg());
        rclcpp::shutdown();
      }
      return;
    }

    auto msg = std::make_unique<std_msgs::msg::Int64>();
    msg->data = start_ns;
    pub_->publish(std::move(msg));
  }

  rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

/* ---- Node i (1..n-2): Relay Subscriber ---- */

struct RelayNode : public rclcpp::Node
{
  RelayNode(int idx)
  : Node("chain_" + std::to_string(idx),
         rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    std::string in_topic = "chain_" + std::to_string(idx);
    std::string out_topic = "chain_" + std::to_string(idx + 1);

    pub_ = this->create_publisher<std_msgs::msg::Int64>(out_topic, 10);
    auto captured_pub = std::weak_ptr<rclcpp::Publisher<std_msgs::msg::Int64>>(pub_);

    sub_ = this->create_subscription<std_msgs::msg::Int64>(
      in_topic, 10,
      [captured_pub](std_msgs::msg::Int64::UniquePtr msg) {
        if (g_done.load()) { return; }

        volatile int dummy = 0;
        for (int j = 0; j < 100; j++) { dummy += j; }
        (void)dummy;

        auto pub_ptr = captured_pub.lock();
        if (!pub_ptr) { return; }
        pub_ptr->publish(std::move(msg));
      });
  }

  rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr pub_;
  rclcpp::Subscription<std_msgs::msg::Int64>::SharedPtr sub_;
};

/* ---- Node n-1: Final Subscriber ---- */

struct FinalNode : public rclcpp::Node
{
  FinalNode(int idx)
  : Node("chain_" + std::to_string(idx),
         rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    std::string in_topic = "chain_" + std::to_string(idx);

    sub_ = this->create_subscription<std_msgs::msg::Int64>(
      in_topic, 10,
      [](std_msgs::msg::Int64::UniquePtr msg) {
        if (g_done.load()) { return; }

        int64_t chain_start_ns = msg->data;

        volatile int dummy = 0;
        for (int j = 0; j < 100; j++) { dummy += j; }
        (void)dummy;

        int64_t e2e = rcl_clock_ns() - chain_start_ns;
        g_e2e_stats.update(e2e);

        printf("[E3ST-CSV] %d,%u,%lld\n",
               CHAIN_LENGTH, g_round, (long long)e2e);

        g_round++;
        if (g_round >= TOTAL_ROUNDS && !g_done.exchange(true)) {
          printf("[E3ST-SUMMARY] chain_len=%d e2e_min=%lldns e2e_max=%lldns e2e_avg=%lldns\n",
                 CHAIN_LENGTH,
                 (long long)g_e2e_stats.min_ns,
                 (long long)g_e2e_stats.max_ns,
                 (long long)g_e2e_stats.avg());
          rclcpp::shutdown();
        }
      });
  }

  rclcpp::Subscription<std_msgs::msg::Int64>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  printf("=== Exp3-ST: Task Chain (n=%d) SingleThreadedExecutor ===\n", CHAIN_LENGTH);
  printf("Period: %dms, Rounds: %u\n", TIMER_PERIOD_MS, TOTAL_ROUNDS);
  printf("[E3ST-CSV] chain_len,round,e2e_ns\n");

  rclcpp::init(argc, argv);

  rclcpp::executors::SingleThreadedExecutor executor;

  auto timer_node = std::make_shared<TimerNode>();
  executor.add_node(timer_node);

  std::vector<std::shared_ptr<RelayNode>> relay_nodes;
  for (int i = 1; i < CHAIN_LENGTH - 1; i++) {
    auto node = std::make_shared<RelayNode>(i);
    relay_nodes.push_back(node);
    executor.add_node(node);
  }

  std::shared_ptr<FinalNode> final_node;
  if (CHAIN_LENGTH >= 2) {
    final_node = std::make_shared<FinalNode>(CHAIN_LENGTH - 1);
    executor.add_node(final_node);
  }

  executor.spin();
  rclcpp::shutdown();

  return 0;
}
