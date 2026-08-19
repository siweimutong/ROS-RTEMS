/**
 * @file rt_executor.hpp
 *
 * RTExecutor — OS-Kernel-Driven Real-Time Callback Dispatch on RTEMS
 *
 * =====================================================================
 *  Core Design Principle / 核心设计原则
 * =====================================================================
 *
 *   NO user-level scheduling.  Every callback is a standalone RTEMS
 *   real-time task; the RTEMS fixed-priority preemptive (FP) scheduler
 *   is the ONLY scheduler.  Callbacks are triggered by kernel events
 *   (hardware interrupts, RTEMS events), never by middleware queues
 *   or dispatch loops.
 *
 *   Eliminated from the previous design:
 *     - RtsemQueue     (user-level FIFO queue — replaced by kernel events)
 *     - EventManager   (user-level dispatch — replaced by direct task notify)
 *     - TimeManager    (user-level Reader/Worker — replaced by direct driver→task)
 *     - worker_loop    (user-level callback execution loop)
 *     - dispatch_callback (user-level callback enqueue)
 *
 * =====================================================================
 *  Architecture / 架构
 * =====================================================================
 *
 *   ┌───────────────────────────────────────────────────────────┐
 *   │              RTEMS FP Preemptive Scheduler                │
 *   │          (THE ONLY SCHEDULER — no middleware layer)        │
 *   └───────────────────────────────────────────────────────────┘
 *          │              │              │              │
 *          ▼              ▼              ▼              ▼
 *   ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐
 *   │ CBTask   │  │ CBTask   │  │ CBTask   │  │ CBTask   │
 *   │ pri=50   │  │ pri=60   │  │ pri=70   │  │ pri=80   │
 *   │ Timer cb │  │ Sub cb   │  │ Timer cb │  │ Sub cb   │
 *   └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘
 *        │              │              │              │
 *   HW tick         rcl_wait      HW tick        rcl_wait
 *   interrupt       detect        interrupt      detect
 *   (driver         + notify)     (driver        + notify)
 *   →direct                      →direct
 *   event)                       event)
 *
 *   Each CBTask (CallbackTask) is an independent RTEMS task:
 *     loop:  rtems_event_receive(RTEMS_EVENT_1)  →  callback()  →  repeat
 *
 * =====================================================================
 *  Trigger Paths / 触发路径
 * =====================================================================
 *
 *   Timer callbacks (register_rt_timer):
 *     HW tick → /dev/rtss_timer Ticker → rtems_event_send(CBTask)
 *     No Reader/Worker intermediary.  Driver directly notifies the task.
 *
 *   Subscription callbacks (register_rt_subscription):
 *     Publisher → intra-process → spin() detects via rcl_wait
 *              → rtems_event_send(CBTask) → CBTask executes callback
 *     spin() is a pure NOTIFICATION ROUTER — it never executes
 *     callbacks, never queues them, never makes scheduling decisions.
 *     It only identifies which subscription is ready and sends
 *     a kernel event to the corresponding CBTask.
 *
 * =====================================================================
 *  Comparison with Previous Design / 与前版设计对比
 * =====================================================================
 *
 *   Previous:  rcl_wait → dispatch_callback → RtsemQueue → worker_loop → execute
 *              (user-level queue + user-level dispatch + user-level execution)
 *
 *   Current:   rcl_wait → identify_ready → rtems_event_send → CBTask → execute
 *              (pure notification → OS kernel schedules → task executes)
 *
 *   The key difference: no user-level queue, no user-level dispatch,
 *   no user-level execution order decision.  The OS kernel decides
 *   which task runs based on priority.
 */

#ifndef RCLCPP__EXECUTORS__RT_EXECUTOR_HPP_
#define RCLCPP__EXECUTORS__RT_EXECUTOR_HPP_

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <rclcpp/executor.hpp>
#include <rclcpp/macros.hpp>
#include <rclcpp/visibility_control.hpp>
#include <rclcpp/subscription_base.hpp>
#include <rclcpp/timer.hpp>

#include <rtems.h>

namespace rclcpp
{
namespace executors
{

/* ================================================================
 *  RtPriority — Callback Priority Defaults
 *  回调优先级默认值
 *
 *  Named default priority levels.  Users can pass any
 *  rtems_task_priority value; these are only for defaults.
 *  Lower value = higher priority (RTEMS FP scheduler).
 *
 *  Default convention:
 *    Timer callbacks       → HIGH   (hardware-driven, time-critical)
 *    Subscription callbacks → LOW    (event-driven, less time-critical)
 * ================================================================ */

enum RtPriority : rtems_task_priority {
  RT_PRIORITY_HIGH   = 60,  // Timer default — hardware-driven, time-critical
  RT_PRIORITY_NORMAL = 70,
  RT_PRIORITY_LOW    = 80,  // Subscription default — event-driven
};

/* ================================================================
 *  CallbackTask — One RTEMS Task per Callback
 *  回调任务：每个回调对应一个独立的 RTEMS 任务
 *
 *  Each CallbackTask wraps a single RTEMS real-time task that
 *  executes one callback function.  The task blocks on
 *  rtems_event_receive(RTEMS_EVENT_1) and wakes up when notified.
 *
 *  Lifecycle:
 *    Construction → set_callback() → start() → [notify() …] → stop()
 *
 *  The OS kernel (RTEMS FP scheduler) decides WHEN the task runs
 *  based on its priority.  No user-level queue or dispatch involved.
 *
 *  ┌─────────────────────────────────────┐
 *  │  CallbackTask RTEMS task            │
 *  │                                     │
 *  │  while (running) {                  │
 *  │    rtems_event_receive(EVENT_1);    │  ← OS kernel blocks task
 *  │    callback_();                     │  ← OS kernel schedules task
 *  │  }                                  │
 *  └─────────────────────────────────────┘
 *         ▲
 *         │  rtems_event_send(task_id, EVENT_1)
 *         │  (from driver / spin loop / external trigger)
 * ================================================================ */
class CallbackTask
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(CallbackTask)

  /**
   * @brief Construct a CallbackTask
   * @param priority   RTEMS task priority (lower = higher priority)
   * @param name       4-character task name (e.g., "TM0", "SUB1")
   */
  CallbackTask(
    rtems_task_priority priority,
    const std::string & name = "CB");

  ~CallbackTask();

  /**
   * @brief Set the callback function
   *
   * The callback is executed each time the task is notified.
   * It should include all necessary logic (take + handle for
   * subscriptions, execute_callback for timers, etc.).
   *
   * @param cb  The callback function
   */
  void set_callback(std::function<void()> cb);

  /**
   * @brief Create and start the RTEMS task
   *
   * After this call, the task is alive and blocking on
   * rtems_event_receive.  Call notify() to wake it up.
   */
  bool start();

  /**
   * @brief Stop and delete the RTEMS task
   */
  void stop();

  /**
   * @brief Notify the task to execute its callback
   *
   * Sends RTEMS_EVENT_1 to the task.  The RTEMS kernel will
   * schedule the task based on its priority.  If a higher-priority
   * task is running, this task will wait until it becomes the
   * highest-priority ready task.
   *
   * This is the ONLY way to trigger callback execution.
   * No queue, no dispatch, no middleware involvement.
   */
  void notify();

  /// Get the RTEMS task ID
  rtems_id task_id() const { return task_id_; }

  /// Get the task priority
  rtems_task_priority priority() const { return priority_; }

  /// Check if the task is running
  bool is_running() const { return running_; }

private:
  /// Static entry point for the RTEMS task
  static rtems_task entry_point(rtems_task_argument arg);

  /// Main loop: block on event → execute callback → repeat
  void run_loop();

  rtems_id task_id_;
  rtems_task_priority priority_;
  std::string name_;
  std::function<void()> callback_;
  volatile bool running_;
};


/* ================================================================
 *  RTExecutor — RTEMS Real-Time Executor
 *  RTEMS 实时执行器
 *
 *  Replaces the standard rclcpp::Executor's middleware scheduling
 *  with OS-kernel-driven dispatch via CallbackTasks.
 *
 *  Key APIs:
 *    register_rt_timer():       Hardware-interrupt-driven timer callback
 *    register_rt_subscription(): Subscription callback with dedicated task
 *    spin():                    Start CallbackTasks + block until cancel()
 *
 *  Timer callbacks are triggered DIRECTLY by /dev/rtss_timer driver
 *  via rtems_event_send — no spin loop involvement.
 *
 *  Subscription callbacks are triggered DIRECTLY by intra-process
 *  direct_notify_callback_ — no spin loop, no rcl_wait.
 *
 *  spin() does NO polling, NO detection, NO dispatch.
 *  It starts the CallbackTasks, then blocks on rtems_event_receive
 *  until cancel() sends the SHUTDOWN_EVENT.
 * ================================================================ */
class RTExecutor : public rclcpp::Executor
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(RTExecutor)

  /**
   * @brief Construct the RTExecutor
   *
   * @param options  Executor options
   */
  explicit RTExecutor(
    const rclcpp::ExecutorOptions & options = rclcpp::ExecutorOptions());

  virtual ~RTExecutor();

  /**
   * @brief Start the executor and block until cancel()
   *
   * Starts all CallbackTasks and the timer driver, then blocks on
   * rtems_event_receive(SHUTDOWN_EVENT).  No polling, no detection
   * loop, no rcl_wait.  All callback execution is driven entirely
   * by OS kernel events (ISR for timers, direct_notify for subscriptions).
   *
   * Call cancel() to unblock and exit.
   */
  void spin() override;

  /**
   * @brief Stop the executor
   *
   * Sets spinning=false and sends SHUTDOWN_EVENT to wake spin().
   */
  void cancel();

  /**
   * @brief Register a hardware-timer-driven callback
   *
   * Creates a CallbackTask at the given priority.  The /dev/rtss_timer
   * driver ISR directly sends rtems_event_send to this task on each
   * timer expiration.  No intermediary, no queue.
   *
   * Trigger path:
   *   HW tick ISR → rtems_event_send(CBTask)
   *              → RTEMS kernel schedules CBTask → callback()
   *
   * @param callback  The callback function
   * @param period_ms Timer period in milliseconds
   * @param priority  RTEMS task priority (lower = higher priority).
   *                  Defaults to RT_PRIORITY_HIGH.  Accepts any numeric value.
   * @return Channel number (0-7), or -1 on failure
   */
  int register_rt_timer(
    std::function<void()> callback,
    uint32_t period_ms,
    rtems_task_priority priority = RT_PRIORITY_HIGH);

  /**
   * @brief Register a subscription callback with a dedicated RTEMS task
   *
   * Creates a CallbackTask at the given priority.  When a message arrives
   * via intra-process, direct_notify_callback_ sends rtems_event_send
   * directly to the CallbackTask — no spin loop, no rcl_wait.
   *
   * Trigger path:
   *   Publisher → intra-process → direct_notify_callback_
   *            → rtems_event_send(CBTask)
   *            → RTEMS kernel schedules CBTask → take() + handle_message()
   *
   * @param subscription  The subscription to register
   * @param priority      RTEMS task priority.  Defaults to RT_PRIORITY_LOW.
   *                      Accepts any numeric value.
   * @param topic_name    Topic name (for debugging)
   * @return Index of the registration, or -1 on failure
   */
  int register_rt_subscription(
    rclcpp::SubscriptionBase::SharedPtr subscription,
    rtems_task_priority priority = RT_PRIORITY_LOW,
    const std::string & topic_name = "");

private:
  RCLCPP_DISABLE_COPY(RTExecutor)

  static constexpr rtems_event_set SHUTDOWN_EVENT = RTEMS_EVENT_0;

  /* Timer registration record */
  struct TimerReg {
    int channel;                         ///< /dev/rtss_timer channel
    rtems_interval period_ticks;         ///< Timer period in RTEMS ticks
    std::unique_ptr<CallbackTask> task;  ///< Dedicated RTEMS task
  };

  /* Subscription registration record */
  struct SubReg {
    rclcpp::SubscriptionBase::SharedPtr subscription;  ///< ROS2 subscription
    std::unique_ptr<CallbackTask> task;                 ///< Dedicated RTEMS task
    std::string topic_name;                             ///< For debugging
  };

  /* Timer registrations */
  std::vector<TimerReg> timer_regs_;
  int next_channel_;

  /* Subscription registrations */
  std::vector<SubReg> sub_regs_;

  /* Map: subscription pointer → CallbackTask */
  std::unordered_map<rclcpp::SubscriptionBase *, CallbackTask *> sub_task_map_;

  /* Map: timer pointer → CallbackTask (for standard ROS2 timers) */
  std::unordered_map<rclcpp::TimerBase *, CallbackTask *> timer_task_map_;

  /* /dev/rtss_timer file descriptor */
  int dev_fd_;

  /* Task ID of the thread calling spin() — used by cancel() to wake it */
  rtems_id spin_task_id_;

  /**
   * @brief Open /dev/rtss_timer and configure channels for timer CallbackTasks
   *
   * For each registered timer, sets the channel's target_task to the
   * CallbackTask's RTEMS task ID, so the driver sends events directly.
   */
  bool setup_timer_driver();

  /**
   * @brief Stop all timer channels and close the driver
   */
  void teardown_timer_driver();

  /**
   * @brief Start all registered CallbackTasks
   */
  void start_all_tasks();

  /**
   * @brief Stop all registered CallbackTasks
   */
  void stop_all_tasks();
};

}  // namespace executors
}  // namespace rclcpp

#endif  // RCLCPP__EXECUTORS__RT_EXECUTOR_HPP_
