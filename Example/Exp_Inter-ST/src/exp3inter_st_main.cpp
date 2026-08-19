/**
 * @file exp3inter_st_main.cpp
 *
 * Exp3inter-ST: Task Chain Latency — SingleThreadedExecutor
 *
 * CSV columns:
 *   e2e         - first task start to last task end (callback tail)
 *   publish     - cumulative sum of all publish() call times across chain
 *   gc_trigger  - guard condition trigger time (subset of publish)
 *   invoke_cb   - invoke_on_new_message time (subset of publish)
 *   getnext     - get_next_ready_executable time (includes take_data)
 *   take_data   - take_data time (subset of getnext)
 *   sched       - cumulative sum of all per-step publish-return-to-cb-entry delays
 *
 * Compile with -DCHAIN_LENGTH=N (N = 1..5)
 */

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/int32.hpp>
#include <chrono>
#include <memory>
#include <cstdio>
#include <cinttypes>
#include <atomic>
#include <vector>

#include <rclcpp/executors/single_threaded_executor.hpp>

using namespace std::chrono_literals;

extern volatile int g_intra_process_published;
extern volatile int64_t g_rclcpp_gc_trigger_ns;
extern volatile int64_t g_rclcpp_invoke_cb_ns;
extern volatile int64_t g_rclcpp_getnext_ns;
extern volatile int64_t g_rclcpp_take_data_ns;
extern volatile int64_t g_rclcpp_execute_start_ns;

#ifndef CHAIN_LENGTH
#define CHAIN_LENGTH 2
#endif

#define TIMER_PERIOD_MS   200
#define TOTAL_ROUNDS      500

static int64_t rcl_clock_ns()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
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
static E2EStats g_gc_trigger_stats;
static E2EStats g_invoke_cb_stats;
static E2EStats g_getnext_stats;
static E2EStats g_take_data_stats;
static E2EStats g_sched_stats;
static E2EStats g_dispatch_stats;
static volatile int64_t g_last_publish_ns = 0;
static volatile int64_t g_last_publish_end_ns = 0;
static volatile int64_t g_round_publish_sum_ns = 0;  // cumulative publish time per round
static volatile int64_t g_round_sched_sum_ns = 0;    // cumulative sched time per round
static volatile int64_t g_round_gc_trigger_sum_ns = 0;
static volatile int64_t g_round_invoke_cb_sum_ns = 0;
static volatile int64_t g_round_getnext_sum_ns = 0;
static volatile int64_t g_round_takedata_sum_ns = 0;
static volatile int64_t g_round_dispatch_sum_ns = 0;

/* ---- Node 0: Timer Producer ---- */

struct TimerNode : public rclcpp::Node
{
  TimerNode()
  : Node("chain_0", rclcpp::NodeOptions().use_intra_process_comms(false))
  {
    timer_ = this->create_wall_timer(
      std::chrono::milliseconds(TIMER_PERIOD_MS),
      [this]() { this->on_timer(); }
    );

    if (CHAIN_LENGTH > 1) {
      pub_ = this->create_publisher<std_msgs::msg::Int32>("chain_0", 10);
    }
  }

  void on_timer()
  {
    if (g_done.load() || g_round >= TOTAL_ROUNDS) { return; }

    // Reset per-round cumulative counters
    g_round_publish_sum_ns = 0;
    g_round_sched_sum_ns = 0;
    g_round_gc_trigger_sum_ns = 0;
    g_round_invoke_cb_sum_ns = 0;
    g_round_getnext_sum_ns = 0;
    g_round_takedata_sum_ns = 0;
    g_round_dispatch_sum_ns = 0;

    int64_t start_ns = rcl_clock_ns();

    if (CHAIN_LENGTH == 1) {
      int64_t e2e = rcl_clock_ns() - start_ns;
      g_e2e_stats.update(e2e);
      printf("[E3IST-CSV] %d,%u,%lld,0,0,0,0,0,0\n",
             CHAIN_LENGTH, g_round, (long long)e2e);
      g_round++;
      if (g_round >= TOTAL_ROUNDS && !g_done.exchange(true)) {
        rclcpp::shutdown();
      }
      return;
    }

    auto msg = std::make_unique<std_msgs::msg::Int32>();
    msg->data = ns_to_trunc(start_ns);
    int64_t pub_t0 = rcl_clock_ns();
    pub_->publish(std::move(msg));
    int64_t pub_t1 = rcl_clock_ns();
    g_intra_process_published = 1;
    int64_t pub_ns = pub_t1 - pub_t0;
    g_last_publish_ns = pub_ns;
    g_last_publish_end_ns = pub_t1;
    g_round_publish_sum_ns += pub_ns;
  }

  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

/* ---- Node i (1..n-2): Relay Subscriber ---- */

struct RelayNode : public rclcpp::Node
{
  RelayNode(int idx)
  : Node("chain_" + std::to_string(idx),
         rclcpp::NodeOptions().use_intra_process_comms(false))
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
        g_round_dispatch_sum_ns += (cb_entry_ns - g_rclcpp_execute_start_ns);
        g_round_gc_trigger_sum_ns += g_rclcpp_gc_trigger_ns;
        g_round_invoke_cb_sum_ns += g_rclcpp_invoke_cb_ns;
        g_round_getnext_sum_ns += g_rclcpp_getnext_ns;
        g_round_takedata_sum_ns += g_rclcpp_take_data_ns;

        auto pub_ptr = captured_pub.lock();
        if (!pub_ptr) { return; }
        int64_t pub_t0 = rcl_clock_ns();
        pub_ptr->publish(std::move(msg));
        int64_t pub_t1 = rcl_clock_ns();
        g_intra_process_published = 1;
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
         rclcpp::NodeOptions().use_intra_process_comms(false))
  {
    std::string in_topic = "chain_" + std::to_string(idx - 1);

    sub_ = this->create_subscription<std_msgs::msg::Int32>(
      in_topic, 10,
      [](std_msgs::msg::Int32::UniquePtr msg) {
        if (g_done.load()) { return; }

        int64_t cb_entry_ns = rcl_clock_ns();

        // Accumulate final step's scheduling metrics
        g_round_sched_sum_ns += (cb_entry_ns - g_last_publish_end_ns);
        g_round_dispatch_sum_ns += (cb_entry_ns - g_rclcpp_execute_start_ns);
        g_round_gc_trigger_sum_ns += g_rclcpp_gc_trigger_ns;
        g_round_invoke_cb_sum_ns += g_rclcpp_invoke_cb_ns;
        g_round_getnext_sum_ns += g_rclcpp_getnext_ns;
        g_round_takedata_sum_ns += g_rclcpp_take_data_ns;

        int32_t chain_start_trunc = msg->data;
        int64_t e2e = trunc_e2e(chain_start_trunc, cb_entry_ns);
        int64_t pub = g_round_publish_sum_ns;
        int64_t gc = g_round_gc_trigger_sum_ns;
        int64_t ic = g_round_invoke_cb_sum_ns;
        int64_t gn = g_round_getnext_sum_ns;
        int64_t td = g_round_takedata_sum_ns;

        g_e2e_stats.update(e2e);
        g_publish_stats.update(pub);
        g_gc_trigger_stats.update(gc);
        g_invoke_cb_stats.update(ic);
        g_getnext_stats.update(gn);
        g_take_data_stats.update(td);
        g_sched_stats.update(g_round_sched_sum_ns);
        g_dispatch_stats.update(g_round_dispatch_sum_ns);

        printf("[E3IST-CSV] %d,%u,%lld,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
               CHAIN_LENGTH, g_round,
               (long long)e2e, (long long)pub, (long long)gc, (long long)ic,
               (long long)gn, (long long)td, (long long)g_round_sched_sum_ns,
               (long long)g_round_dispatch_sum_ns);

        g_round++;
        if (g_round >= TOTAL_ROUNDS && !g_done.exchange(true)) {
          printf("[E3IST-SUMMARY] chain_len=%d "
                 "e2e_avg=%lld publish_avg=%lld gc_trigger_avg=%lld invoke_cb_avg=%lld "
                 "getnext_avg=%lld take_data_avg=%lld sched_avg=%lld dispatch_avg=%lld\n",
                 CHAIN_LENGTH,
                 (long long)g_e2e_stats.avg(),
                 (long long)g_publish_stats.avg(),
                 (long long)g_gc_trigger_stats.avg(),
                 (long long)g_invoke_cb_stats.avg(),
                 (long long)g_getnext_stats.avg(),
                 (long long)g_take_data_stats.avg(),
                 (long long)g_sched_stats.avg(),
                 (long long)g_dispatch_stats.avg());
          rclcpp::shutdown();
        }
      });
  }

  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

int main(int argc, char * argv[])
{
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  printf("=== Exp3inter-ST: Task Chain (n=%d) SingleThreadedExecutor ===\n", CHAIN_LENGTH);
  printf("Period: %dms, Rounds: %u\n", TIMER_PERIOD_MS, TOTAL_ROUNDS);
  printf("[E3IST-CSV] chain_len,round,e2e_ns,publish_ns,gc_trigger_ns,invoke_cb_ns,getnext_ns,take_data_ns,sched_ns,dispatch_ns\n");

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

  /* Cleanly exit QEMU via ARM semihosting SYS_EXIT (ARM or Thumb) */
#if defined(__thumb__)
  __asm__ volatile (
      "mov r0, #0x18\n"
      "mov r1, #0\n"
      "svc 0xAB\n"
      : : : "r0", "r1", "memory"
  );
#else
  __asm__ volatile (
      "mov r0, #0x18\n"
      "mov r1, #0\n"
      "svc 0x123456\n"
      : : : "r0", "r1", "memory"
  );
#endif
  return 0;
}
