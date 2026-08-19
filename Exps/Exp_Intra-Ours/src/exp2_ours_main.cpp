/**
 * @file exp2_ours_main.cpp
 *
 * Exp2-Ours: Task Chain Latency — RTExecutor (rclcpp-rtss25)
 *
 * Same chain topology as Exp2-ST, but uses RTExecutor which dispatches
 * callbacks via independent RTEMS real-time tasks (CallbackTask) driven
 * by kernel events instead of a spin-loop executor.
 *
 * - Timer: /dev/rtss_timer ISR → rtems_event_send → CallbackTask
 * - Subscription: intra-process direct_notify → rtems_event_send → CallbackTask
 *
 * CSV columns:
 *   e2e         - first task start to last task end (callback tail)
 *   publish     - cumulative sum of all publish() call times across chain
 *   gc_trigger  - 0 (not instrumented in RTExecutor)
 *   invoke_cb   - 0 (not instrumented in RTExecutor)
 *   getnext     - 0 (not instrumented in RTExecutor)
 *   take_data   - 0 (not instrumented in RTExecutor)
 *   sched       - cumulative sum of all per-step publish-return-to-cb-entry delays
 *   dispatch    - 0 (not instrumented in RTExecutor)
 *
 * Compile with -DCHAIN_LENGTH=N (N = 1..5)
 */

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/executors/rt_executor.hpp>
#include <std_msgs/msg/int32.hpp>
#include <chrono>
#include <memory>
#include <cstdio>
#include <cinttypes>
#include <atomic>
#include <vector>

#include <rtems.h>

extern "C" {
#include "driver/rtss_timer_driver.h"
}

extern volatile int64_t rtss_isr_fire_ns[];

using namespace std::chrono_literals;

#ifndef CHAIN_LENGTH
#define CHAIN_LENGTH 2
#endif

#define TIMER_PERIOD_MS   200
#define TOTAL_ROUNDS      500
#define WARMUP_ROUNDS      10

static int64_t rcl_clock_ns()
{
  struct timespec ts;
  rtems_clock_get_uptime(&ts);
  return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

static int32_t ns_to_trunc(int64_t ns)
{
  return static_cast<int32_t>(ns);
}

static int64_t trunc_e2e(int32_t start_trunc, int64_t end_ns)
{
  int32_t end_trunc = ns_to_trunc(end_ns);
  return static_cast<int64_t>(end_trunc - start_trunc);
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
static E2EStats g_publish_stats;
static E2EStats g_sched_stats;
static volatile int64_t g_last_publish_end_ns = 0;
static volatile int64_t g_round_publish_sum_ns = 0;
static volatile int64_t g_round_sched_sum_ns = 0;

/* Global pointer so FinalNode can cancel the executor (rclcpp::shutdown()
 * does NOT automatically call RTExecutor::cancel()). */
static rclcpp::executors::RTExecutor * g_executor_ptr = nullptr;

/* ---- Node 0: Timer Producer (standard ROS2 timer + publisher) ---- */

struct TimerNode : public rclcpp::Node
{
  TimerNode()
  : Node("chain_0", rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    if (CHAIN_LENGTH > 1) {
      pub_ = this->create_publisher<std_msgs::msg::Int32>("chain_0", 10);
    }
  }

  // Called by RTExecutor timer CallbackTask
  void on_timer(int channel)
  {
    if (g_done.load() || g_round >= TOTAL_ROUNDS + WARMUP_ROUNDS) { return; }

    // Reset per-round cumulative counters
    g_round_publish_sum_ns = 0;
    g_round_sched_sum_ns = 0;

    int64_t start_ns = rcl_clock_ns();

    if (CHAIN_LENGTH == 1) {
      int64_t e2e = rcl_clock_ns() - start_ns;
      g_e2e_stats.update(e2e);
      printf("[E2OURS-CSV] %d,%u,%lld,0,0,0,0,0,0,0\n",
             CHAIN_LENGTH, g_round, (long long)e2e);
      g_round++;
      if (g_round >= TOTAL_ROUNDS + WARMUP_ROUNDS && !g_done.exchange(true)) {
        rclcpp::shutdown();
      }
      return;
    }

    auto msg = std::make_unique<std_msgs::msg::Int32>();
    msg->data = ns_to_trunc(start_ns);
    int64_t pub_t0 = rcl_clock_ns();
    pub_->publish(std::move(msg));
    int64_t pub_t1 = rcl_clock_ns();
    int64_t pub_ns = pub_t1 - pub_t0;
    g_last_publish_end_ns = pub_t1;
    g_round_publish_sum_ns += pub_ns;
  }

  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;  // kept for registration
};

/* ---- Node i (1..n-2): Relay Subscriber ---- */

struct RelayNode : public rclcpp::Node
{
  RelayNode(int idx)
  : Node("chain_" + std::to_string(idx),
         rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    std::string in_topic = "chain_" + std::to_string(idx - 1);
    std::string out_topic = "chain_" + std::to_string(idx);

    pub_ = this->create_publisher<std_msgs::msg::Int32>(out_topic, 10);
    auto captured_pub = std::weak_ptr<rclcpp::Publisher<std_msgs::msg::Int32>>(pub_);

    sub_ = this->create_subscription<std_msgs::msg::Int32>(
      in_topic, 10,
      [captured_pub](std_msgs::msg::Int32::UniquePtr msg) {
        if (g_done.load()) { return; }

        int64_t cb_entry_ns = rcl_clock_ns();

        // Accumulate this step's scheduling metrics
        g_round_sched_sum_ns += (cb_entry_ns - g_last_publish_end_ns);

        auto pub_ptr = captured_pub.lock();
        if (!pub_ptr) { return; }
        int64_t pub_t0 = rcl_clock_ns();
        pub_ptr->publish(std::move(msg));
        int64_t pub_t1 = rcl_clock_ns();
        int64_t pub_ns = pub_t1 - pub_t0;
        g_last_publish_end_ns = pub_t1;
        g_round_publish_sum_ns += pub_ns;
      });
  }

  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

/* ---- Node n-1: Final Subscriber ---- */

struct FinalNode : public rclcpp::Node
{
  FinalNode(int idx)
  : Node("chain_" + std::to_string(idx),
         rclcpp::NodeOptions().use_intra_process_comms(true))
  {
    std::string in_topic = "chain_" + std::to_string(idx - 1);

    sub_ = this->create_subscription<std_msgs::msg::Int32>(
      in_topic, 10,
      [](std_msgs::msg::Int32::UniquePtr msg) {
        if (g_done.load()) { return; }

        // Skip warmup rounds
        if (g_round < WARMUP_ROUNDS) {
          g_round++;
          return;
        }

        int64_t cb_entry_ns = rcl_clock_ns();

        // Accumulate final step's scheduling metrics
        g_round_sched_sum_ns += (cb_entry_ns - g_last_publish_end_ns);

        int32_t chain_start_trunc = msg->data;
        int64_t e2e = trunc_e2e(chain_start_trunc, cb_entry_ns);
        int64_t pub = g_round_publish_sum_ns;
        int64_t sched = g_round_sched_sum_ns;

        g_e2e_stats.update(e2e);
        g_publish_stats.update(pub);
        g_sched_stats.update(sched);

        printf("[E2OURS-CSV] %d,%u,%lld,%lld,0,0,0,0,%lld,0\n",
               CHAIN_LENGTH, g_round,
               (long long)e2e, (long long)pub, (long long)sched);

        g_round++;
        if (g_round >= TOTAL_ROUNDS + WARMUP_ROUNDS && !g_done.exchange(true)) {
          printf("[E2OURS-SUMMARY] chain_len=%d "
                 "e2e_avg=%lld publish_avg=%lld sched_avg=%lld\n",
                 CHAIN_LENGTH,
                 (long long)g_e2e_stats.avg(),
                 (long long)g_publish_stats.avg(),
                 (long long)g_sched_stats.avg());
          /* Cancel executor directly — rclcpp::shutdown() does NOT
           * unblock RTExecutor::spin() (it waits on SHUTDOWN_EVENT). */
          if (g_executor_ptr) {
            g_executor_ptr->cancel();
          }
          rclcpp::shutdown();
        }
      });
  }

  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  printf("=== Exp2-Ours: Task Chain (n=%d) RTExecutor ===\n", CHAIN_LENGTH);
  printf("Period: %dms, Rounds: %u\n", TIMER_PERIOD_MS, TOTAL_ROUNDS);
  printf("[E2OURS-CSV] chain_len,round,e2e_ns,publish_ns,gc_trigger_ns,invoke_cb_ns,getnext_ns,take_data_ns,sched_ns,dispatch_ns\n");

  rclcpp::init(argc, argv);

  rclcpp::executors::RTExecutor executor;

  // Create nodes
  auto timer_node = std::make_shared<TimerNode>();

  std::vector<std::shared_ptr<RelayNode>> relay_nodes;
  for (int i = 1; i < CHAIN_LENGTH - 1; i++) {
    relay_nodes.push_back(std::make_shared<RelayNode>(i));
  }

  std::shared_ptr<FinalNode> final_node;
  if (CHAIN_LENGTH >= 2) {
    final_node = std::make_shared<FinalNode>(CHAIN_LENGTH - 1);
  }

  // Register timer callback via RTExecutor (hardware-driven)
  int timer_ch = executor.register_rt_timer(
    [timer_node]() {
      timer_node->on_timer(0);
    },
    TIMER_PERIOD_MS,
    rclcpp::executors::RT_PRIORITY_HIGH);

  // Register subscription callbacks via RTExecutor (direct notify)
  for (int i = 0; i < (int)relay_nodes.size(); i++) {
    executor.register_rt_subscription(
      relay_nodes[i]->sub_,
      rclcpp::executors::RT_PRIORITY_LOW,
      "chain_" + std::to_string(i + 1));
  }

  if (CHAIN_LENGTH >= 2) {
    executor.register_rt_subscription(
      final_node->sub_,
      rclcpp::executors::RT_PRIORITY_LOW,
      "chain_" + std::to_string(CHAIN_LENGTH - 1));
  }

  g_executor_ptr = &executor;
  executor.spin();
  rclcpp::shutdown();

  /* Cleanly exit QEMU via ARM semihosting SYS_EXIT (0x18)
   *
   * Thumb: r0=0x18, r1=0 (exit code, direct value)
   * ARM:   r0=0x18, r1=pointer to {0} (exit code)
   */
  printf("[E2OURS-EXIT] Reached semihosting exit\n");
  {
#if defined(__thumb__)
    /* Thumb-2: use movw to load 0x20026 (ADP_Stopped_ApplicationExit)
     * Fall back to exit code 0 if movw/movt also problematic */
    __asm__ volatile (
        "mov r0, #0x18\n"
        "movs r1, #0\n"
        "svc 0xAB\n"
        : : : "r0", "r1", "memory"
    );
#else
    volatile int exit_code = 0;
    __asm__ volatile (
        "mov r0, #0x18\n"
        "mov r1, %[param]\n"
        "svc 0x123456\n"
        : : [param] "r"(&exit_code) : "r0", "r1", "memory"
    );
#endif
  }
  return 0;
}
