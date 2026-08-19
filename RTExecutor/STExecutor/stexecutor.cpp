/**
 * @file stexecutor.cpp
 *
 * SingleThreadedExecutor Benchmark on RTEMS
 *
 * Uses the actual rclcpp::executors::SingleThreadedExecutor from the
 * ported rclcpp, NOT a custom C reimplementation.
 *
 * Faithfully replicates the standard ROS 2 execution model:
 *   - Single executor thread: wait → get_next_executable → execute loop
 *   - FIFO dispatch: callbacks dispatched in registration order
 *   - No callback priority: all callbacks share one thread priority
 *   - Software timer: checked only when executor enters rcl_wait
 *   - No preemption: running callback blocks all others
 *
 * This is the BASELINE — the standard rclcpp SingleThreadedExecutor.
 */

#include <rclcpp/rclcpp.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <iostream>
#include <cstdio>
#include <cinttypes>

#include <rtems.h>

#include "rclcpp/executors/single_threaded_executor.hpp"

using namespace std::chrono_literals;

/* ---- Nanosecond timing ---- */

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    rtems_clock_get_uptime(&ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define NS_TO_US(ns)   ((ns) / 1000U)
#define NS_TO_US_F(ns) ((double)(ns) / 1000.0)

static inline void busy_wait(uint64_t ns)
{
    uint64_t deadline = now_ns() + ns;
    while (now_ns() < deadline)
        ;
}

/* ---- Constants ---- */

#define NUM_FIRINGS   100

/* ---- Measurement record ---- */

typedef struct {
    uint64_t release_ns;
    uint64_t start_ns;
    uint64_t end_ns;
} meas_t;

/* ---- Shared measurement state ---- */

static meas_t g_timer_meas[3][NUM_FIRINGS];
static volatile int g_timer_count[3] = {0, 0, 0};
static uint64_t g_timer_start_ns[3];

static meas_t g_lat_meas[NUM_FIRINGS];
static volatile int g_lat_count = 0;

/* ---- Timer callback nodes ---- */

class TimerJitterNode : public rclcpp::Node
{
public:
    TimerJitterNode(uint32_t period_ms, int idx)
    : Node("timer_" + std::to_string(idx),
           rclcpp::NodeOptions().use_intra_process_comms(true)),
      idx_(idx)
    {
        g_timer_start_ns[idx] = now_ns();

        auto period = std::chrono::milliseconds(period_ms);
        timer_ = this->create_wall_timer(
            period,
            [this]() {
                uint64_t t = now_ns();
                int n = g_timer_count[idx_];
                if (n < NUM_FIRINGS) {
                    /* Theoretical release time for this firing */
                    uint32_t period_ms = this->timer_->get_period().count();
                    g_timer_meas[idx_][n].release_ns =
                        g_timer_start_ns[idx_] +
                        (uint64_t)n * (uint64_t)period_ms * 1000000ULL;
                    g_timer_meas[idx_][n].start_ns = t;

                    /* Simulate callback work */
                    busy_wait(1000000ULL); /* 1ms */

                    g_timer_meas[idx_][n].end_ns = now_ns();
                    g_timer_count[idx_]++;
                }
            });
    }

private:
    int idx_;
    rclcpp::TimerBase::SharedPtr timer_;
};

/* ---- Pub/Sub nodes for latency measurement ---- */

class LatencyPublisher : public rclcpp::Node
{
public:
    LatencyPublisher()
    : Node("lat_publisher",
           rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        pub_ = this->create_publisher<std_msgs::msg::Int64>("latency_topic", 10);

        timer_ = this->create_wall_timer(
            100ms,
            [this]() {
                auto msg = std::make_unique<std_msgs::msg::Int64>();
                msg->data = (int64_t)now_ns();
                pub_->publish(std::move(msg));
            });
    }

private:
    rclcpp::Publisher<std_msgs::msg::Int64>::SharedPtr pub_;
    rclcpp::TimerBase::SharedPtr timer_;
};

class LatencySubscriber : public rclcpp::Node
{
public:
    LatencySubscriber()
    : Node("lat_subscriber",
           rclcpp::NodeOptions().use_intra_process_comms(true))
    {
        sub_ = this->create_subscription<std_msgs::msg::Int64>(
            "latency_topic", 10,
            [this](std_msgs::msg::Int64::UniquePtr msg) {
                uint64_t start = now_ns();
                int n = g_lat_count;
                if (n < NUM_FIRINGS) {
                    g_lat_meas[n].release_ns = (uint64_t)msg->data;
                    g_lat_meas[n].start_ns = start;
                    busy_wait(1000000ULL); /* 1ms callback work */
                    g_lat_meas[n].end_ns = now_ns();
                    g_lat_count++;
                }
            });
    }

private:
    rclcpp::Subscription<std_msgs::msg::Int64>::SharedPtr sub_;
};

/* ---- Print helpers ---- */

static void print_jitter_stats(const char *label, meas_t *m, int n)
{
    uint64_t jmin = UINT64_MAX, jmax = 0, jsum = 0;
    for (int i = 0; i < n && i < NUM_FIRINGS; i++) {
        uint64_t jitter = m[i].start_ns - m[i].release_ns;
        if (jitter < jmin) jmin = jitter;
        if (jitter > jmax) jmax = jitter;
        jsum += jitter;
    }
    if (n == 0) jmin = 0;
    printf("  %s: n=%d, jitter min=%llu us, max=%llu us, mean=%.2f us\n",
           label, n,
           (unsigned long long)NS_TO_US(jmin),
           (unsigned long long)NS_TO_US(jmax),
           n > 0 ? NS_TO_US_F((double)jsum / n) : 0.0);
}

/* ---- Exp A: Timer Callback Jitter ---- */

static void run_exp_a(void)
{
    printf("\n=== Exp A: Timer Callback Jitter (SingleThreadedExecutor) ===\n");

    rclcpp::init(0, nullptr);

    auto node0 = std::make_shared<TimerJitterNode>(200, 0);
    auto node1 = std::make_shared<TimerJitterNode>(500, 1);
    auto node2 = std::make_shared<TimerJitterNode>(1000, 2);

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node0);
    executor.add_node(node1);
    executor.add_node(node2);

    /* Spin for 30 seconds */
    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < end_time) {
        executor.spin_some(std::chrono::milliseconds(100));
    }

    const char *labels[] = {"200ms", "500ms", "1000ms"};
    for (int i = 0; i < 3; i++)
        print_jitter_stats(labels[i], g_timer_meas[i], g_timer_count[i]);

    /* CSV */
    uint32_t periods[] = {200, 500, 1000};
    printf("\nCSV,stexec,A,period_ms,jitter_us\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < g_timer_count[i] && j < NUM_FIRINGS; j++) {
            uint64_t jitter = g_timer_meas[i][j].start_ns - g_timer_meas[i][j].release_ns;
            printf("CSV,stexec,A,%d,%llu\n", periods[i],
                   (unsigned long long)NS_TO_US(jitter));
        }
    }

    rclcpp::shutdown();
}

/* ---- Exp B: Subscription E2E Latency ---- */

static void run_exp_b(void)
{
    printf("\n=== Exp B: Subscription E2E Latency (SingleThreadedExecutor) ===\n");

    rclcpp::init(0, nullptr);

    auto pub = std::make_shared<LatencyPublisher>();
    auto sub = std::make_shared<LatencySubscriber>();

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(pub);
    executor.add_node(sub);

    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < end_time) {
        executor.spin_some(std::chrono::milliseconds(100));
    }

    int n = g_lat_count;
    uint64_t lmin = UINT64_MAX, lmax = 0, lsum = 0;
    for (int i = 0; i < n; i++) {
        uint64_t lat = g_lat_meas[i].end_ns - g_lat_meas[i].release_ns;
        if (lat < lmin) lmin = lat;
        if (lat > lmax) lmax = lat;
        lsum += lat;
    }
    if (n == 0) lmin = 0;
    printf("  sub_e2e: n=%d, latency min=%llu us, max=%llu us, mean=%.2f us\n",
           n, (unsigned long long)NS_TO_US(lmin),
           (unsigned long long)NS_TO_US(lmax),
           n > 0 ? NS_TO_US_F((double)lsum / n) : 0.0);

    rclcpp::shutdown();
}

/* ---- Exp C: Priority Preemption ---- */

static void run_exp_c(void)
{
    printf("\n=== Exp C: Priority Preemption (SingleThreadedExecutor) ===\n");
    printf("  NOTE: SingleThreadedExecutor has NO priority preemption.\n");
    printf("  All callbacks share one executor thread (FIFO dispatch).\n");
    printf("  A running callback blocks ALL others until it completes.\n");
    printf("  Preemption success rate: 0%% (by design)\n");
}

/* ---- Exp D & E: Concurrency & Overhead ---- */
/* These follow the same pattern as Exp A — use SingleThreadedExecutor
 * with timer nodes and measure jitter/overhead. */

static void run_exp_d(void)
{
    printf("\n=== Exp D: Callback Concurrency (SingleThreadedExecutor) ===\n");

    rclcpp::init(0, nullptr);

    auto node0 = std::make_shared<TimerJitterNode>(200, 0);
    auto node1 = std::make_shared<TimerJitterNode>(500, 1);

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node0);
    executor.add_node(node1);

    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < end_time) {
        executor.spin_some(std::chrono::milliseconds(100));
    }

    print_jitter_stats("cb0(200ms)", g_timer_meas[0], g_timer_count[0]);
    print_jitter_stats("cb1(500ms)", g_timer_meas[1], g_timer_count[1]);

    rclcpp::shutdown();
}

/* ---- Exp E: Dispatch Path Overhead ---- */

static meas_t g_overhead_meas[NUM_FIRINGS];
static volatile int g_overhead_count = 0;
static uint64_t g_overhead_start_ns;

class OverheadNode : public rclcpp::Node
{
public:
    OverheadNode()
    : Node("overhead_node")
    {
        g_overhead_start_ns = now_ns();

        timer_ = this->create_wall_timer(
            200ms,
            [this]() {
                int n = g_overhead_count;
                if (n < NUM_FIRINGS) {
                    g_overhead_meas[n].release_ns =
                        g_overhead_start_ns + (uint64_t)n * 200ULL * 1000000ULL;
                    g_overhead_meas[n].start_ns = now_ns();
                    /* No work — measure pure dispatch overhead */
                    g_overhead_meas[n].end_ns = now_ns();
                    g_overhead_count++;
                }
            });
    }

private:
    rclcpp::TimerBase::SharedPtr timer_;
};

static void run_exp_e(void)
{
    printf("\n=== Exp E: Dispatch Path Overhead (SingleThreadedExecutor) ===\n");

    rclcpp::init(0, nullptr);

    auto node = std::make_shared<OverheadNode>();

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node);

    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < end_time) {
        executor.spin_some(std::chrono::milliseconds(100));
    }

    if (g_overhead_count > 0) {
        uint64_t osum = 0, omin = UINT64_MAX, omax = 0;
        for (int i = 0; i < g_overhead_count && i < NUM_FIRINGS; i++) {
            uint64_t overhead = g_overhead_meas[i].start_ns - g_overhead_meas[i].release_ns;
            if (overhead < omin) omin = overhead;
            if (overhead > omax) omax = overhead;
            osum += overhead;
        }
        printf("  Dispatch overhead: min=%llu us, max=%llu us, mean=%.2f us\n",
               (unsigned long long)NS_TO_US(omin),
               (unsigned long long)NS_TO_US(omax),
               NS_TO_US_F((double)osum / g_overhead_count));
    }

    rclcpp::shutdown();
}

/* ---- RTEMS Init ---- */

extern "C" {

void rtems_task_init(rtems_task_argument arg);

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_NULL_DRIVER
#define CONFIGURE_MICROSECONDS_PER_TICK    1000

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_MAXIMUM_TASKS            30
#define CONFIGURE_MAXIMUM_SEMAPHORES       30
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES   10
#define CONFIGURE_MAXIMUM_TIMERS           10

#define CONFIGURE_INIT_TASK_NAME           rtems_build_name('M', 'A', 'I', 'N')
#define CONFIGURE_INIT_TASK_PRIORITY       50
#define CONFIGURE_INIT_TASK_INITIAL_MODES  RTEMS_DEFAULT_MODES
#define CONFIGURE_INIT_TASK_ARGUMENTS      0
#define CONFIGURE_INIT_TASK_STACK_SIZE     (256 * 1024)
#define CONFIGURE_INIT_TASK_ENTRY_POINT    rtems_task_init

#include <rtems/confdefs.h>

}

void rtems_task_init(rtems_task_argument arg)
{
    (void)arg;
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    printf("SingleThreadedExecutor Benchmark on RTEMS\n");
    printf("==========================================\n");
    printf("Tick: 1ms | Timing: ns (rtems_clock_get_uptime)\n");
    printf("Executor: rclcpp::executors::SingleThreadedExecutor\n");

    run_exp_a();
    run_exp_b();
    run_exp_c();
    run_exp_d();
    run_exp_e();

    printf("\nBenchmark complete.\n");
    rtems_task_delete(RTEMS_SELF);
}
