/**
 * @file rt_executor.cpp
 *
 * RTExecutor — OS-Kernel-Driven Real-Time Callback Dispatch Implementation
 *
 * =====================================================================
 *  Design Principle
 * =====================================================================
 *
 *   ALL scheduling is triggered by the OS kernel.  NO user-level
 *   detection, NO spin loop, NO rcl_wait, NO polling, NO middleware queue.
 *
 *   Timer callbacks:
 *     HW tick → /dev/rtss_timer Ticker → rtems_event_send(CBTask)
 *             → RTEMS FP scheduler → callback()
 *     Direct driver notification.  No intermediary tasks.
 *
 *   Subscription callbacks (two paths):
 *     Intra-process:  Publisher → IntraProcessBuffer → direct_notify_callback_
 *                     → rtems_event_send(CBTask) → RTEMS FP scheduler → callback()
 *     DDS/inter-process: rmw receives msg → on_new_message_callback_ (wrapped)
 *                        → rtems_event_send(CBTask) → RTEMS FP scheduler → callback()
 *     Supports both intra_process_comms=true and intra_process_comms=false.
 *
 *   Both paths are IDENTICAL in structure: kernel event → CallbackTask.
 *   The RTEMS FP scheduler is the ONLY scheduling decision-maker.
 *
 * =====================================================================
 *  Execution Flow
 * =====================================================================
 *
 *   FLOW 1: Timer Callback (driver → CBTask)
 *   ────────────────────────────────────────
 *     HW tick interrupt
 *         │
 *         ▼
 *     /dev/rtss_timer Ticker Task (priority 90)
 *         │  rtems_event_send(ch->target_task, RTEMS_EVENT_1)
 *         ▼
 *     CallbackTask (user priority, e.g., 60)
 *         │  rtems_event_receive() returns
 *         │  callback_() executes
 *         ▼
 *     Timer callback in CallbackTask context
 *
 *   FLOW 2: Subscription Callback (intra-process / DDS → CBTask)
 *   ───────────────────────────────────────────────────────────────
 *     Path A — Intra-process (use_intra_process_comms=true):
 *       Publisher publishes (intra-process)
 *           │  IntraProcessBuffer::provide_intra_process_data()
 *           │  buffer_->add_shared(message)
 *           │  direct_notify_callback_()  ← WE SET THIS
 *           │    → rtems_event_send(CBTask, RTEMS_EVENT_1)
 *           │  (guard condition is NOT triggered — bypasses rcl_wait)
 *           ▼
 *       CallbackTask wakes, executes take + handle
 *
 *     Path B — DDS / inter-process (use_intra_process_comms=false):
 *       rmw receives DDS message
 *           │  on_new_message_callback_(count)  ← rclcpp-rtems wraps this
 *           │  our wrapper:
 *           │    → rtems_event_send(CBTask, RTEMS_EVENT_1)  ← ADDED
 *           │    → previous callback (guard condition, if any)
 *           ▼
 *       CallbackTask wakes, executes take + handle
 *       (via rcl_take or subscription->take_type_erased)
 *
 *   NO spin loop.  NO rcl_wait.  NO user-level detection.
 *   The OS kernel triggers ALL callback execution.
 *
 * =====================================================================
 *  rclcpp Modifications
 * =====================================================================
 *
 *   Four targeted modifications to rclcpp enable direct notification:
 *
 *   1. SubscriptionIntraProcessBase:
 *      - Added direct_notify_callback_ field
 *      - invoke_on_new_message() calls it if set (bypasses guard condition)
 *
 *   2. SubscriptionIntraProcessBuffer:
 *      - trigger_guard_condition() skips gc_.trigger() when
 *        direct_notify_callback_ is set
 *
 *   3. SubscriptionBase:
 *      - Added set_intra_process_direct_notify() to set the callback
 *      - Added get_on_new_message_callback() to retrieve DDS callback
 *        (rclcpp-rtems: enables wrapping for inter-process notification)
 *
 *   4. RTExecutor::register_rt_subscription():
 *      - Sets direct_notify for intra-process path
 *      - Wraps on_new_message_callback for DDS/inter-process path
 *        (rclcpp-rtems: enables kernel notification for both paths)
 */

#include "rclcpp/executors/rt_executor.hpp"

#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cstdio>

#include <rtems.h>
#include <rtems/libio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

extern "C" {
#include "driver/rtss_timer_driver.h"
}

#include "rcpputils/scope_exit.hpp"
#include "rclcpp/utilities.hpp"
#include "rclcpp/subscription_base.hpp"
#include "rclcpp/timer.hpp"


/* ================================================================
 *  RTEMS Helper Functions
 * ================================================================ */

static const char * rt_status_text(rtems_status_code sc)
{
  return rtems_status_text(sc);
}

static rtems_interval rt_ms_to_ticks(uint32_t ms)
{
  rtems_interval tps = rtems_clock_get_ticks_per_second();
  return (rtems_interval)((uint64_t)ms * tps / 1000);
}


/* ================================================================
 *  CallbackTask Implementation
 *
 *  Each CallbackTask wraps one RTEMS task that blocks on
 *  rtems_event_receive and executes a callback when notified.
 *  The RTEMS FP scheduler is the ONLY scheduler.
 * ================================================================ */

CallbackTask::CallbackTask(
  rtems_task_priority priority,
  const std::string & name)
: task_id_(RTEMS_ID_NONE),
  priority_(priority),
  name_(name),
  callback_(nullptr),
  running_(false)
{}

CallbackTask::~CallbackTask() { stop(); }

void CallbackTask::set_callback(std::function<void()> cb)
{
  callback_ = std::move(cb);
}

bool CallbackTask::start()
{
  if (running_) { return true; }

  char n1 = name_.size() > 0 ? name_[0] : 'C';
  char n2 = name_.size() > 1 ? name_[1] : 'B';
  char n3 = name_.size() > 2 ? name_[2] : '_';
  char n4 = name_.size() > 3 ? name_[3] : '_';
  rtems_name rname = rtems_build_name(n1, n2, n3, n4);

  rtems_status_code sc = rtems_task_create(
    rname, priority_,
    RTEMS_MINIMUM_STACK_SIZE * 4,
    RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES,
    &task_id_);

  if (sc != RTEMS_SUCCESSFUL) {
    std::cerr << "[CallbackTask] Create failed '" << name_
              << "': " << rt_status_text(sc) << std::endl;
    return false;
  }

  running_ = true;
  sc = rtems_task_start(task_id_, entry_point, (rtems_task_argument)this);
  if (sc != RTEMS_SUCCESSFUL) {
    running_ = false;
    rtems_task_delete(task_id_);
    task_id_ = RTEMS_ID_NONE;
    return false;
  }

  std::cout << "[CallbackTask] Started '" << name_
            << "' pri=" << static_cast<int>(priority_)
            << " tid=" << task_id_ << std::endl;
  return true;
}

void CallbackTask::stop()
{
  if (!running_) { return; }
  running_ = false;
  if (task_id_ != RTEMS_ID_NONE) {
    rtems_event_send(task_id_, RTEMS_EVENT_1);
    rtems_task_delete(task_id_);
    task_id_ = RTEMS_ID_NONE;
  }
}

void CallbackTask::notify()
{
  if (task_id_ != RTEMS_ID_NONE) {
    rtems_event_send(task_id_, RTEMS_EVENT_1);
  }
}

rtems_task CallbackTask::entry_point(rtems_task_argument arg)
{
  CallbackTask * self = reinterpret_cast<CallbackTask *>(arg);
  self->run_loop();
  rtems_task_delete(RTEMS_SELF);
}

void CallbackTask::run_loop()
{
  while (running_) {
    rtems_event_set events;
    rtems_status_code sc = rtems_event_receive(
      RTEMS_EVENT_1,
      RTEMS_EVENT_ANY | RTEMS_WAIT,
      RTEMS_NO_TIMEOUT,
      &events);

    if (sc != RTEMS_SUCCESSFUL || !running_) { break; }

    if (callback_) {
      try {
        callback_();
      } catch (const std::exception & e) {
        std::cerr << "[CallbackTask] Exception in '" << name_
                  << "': " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "[CallbackTask] Unknown exception in '"
                  << name_ << "'" << std::endl;
      }
    }
  }
}


/* ================================================================
 *  RTExecutor Implementation
 * ================================================================ */

RTExecutor::RTExecutor(
  const rclcpp::ExecutorOptions & options)
: rclcpp::Executor(options),
  next_channel_(0),
  dev_fd_(-1),
  spin_task_id_(RTEMS_ID_NONE)
{
  std::cout << "[RTExecutor] Constructed (kernel-driven, no spin)" << std::endl;
}

RTExecutor::~RTExecutor()
{
  stop_all_tasks();
  teardown_timer_driver();
  std::cout << "[RTExecutor] Destroyed" << std::endl;
}

int RTExecutor::register_rt_timer(
  std::function<void()> callback,
  uint32_t period_ms,
  rtems_task_priority priority)
{
  if (next_channel_ >= RTSS_TIMER_MAX_CHANNELS) {
    std::cerr << "[RTExecutor] Max timer channels reached" << std::endl;
    return -1;
  }

  int ch = next_channel_++;
  rtems_interval period_ticks = rt_ms_to_ticks(period_ms);

  auto task = std::make_unique<CallbackTask>(priority, "TM" + std::to_string(ch));
  task->set_callback(std::move(callback));

  TimerReg reg;
  reg.channel = ch;
  reg.period_ticks = period_ticks;
  reg.task = std::move(task);
  timer_regs_.push_back(std::move(reg));

  std::cout << "[RTExecutor] Registered timer ch=" << ch
            << " period=" << period_ms << "ms pri="
            << static_cast<int>(priority) << std::endl;
  return ch;
}

/**
 * @brief Register a subscription with a dedicated RTEMS CallbackTask
 *
 * Creates a CallbackTask at the given priority and sets the
 * direct_notify_callback_ on the subscription's intra-process buffer.
 * When a message is published intra-process, the publishing path
 * DIRECTLY calls rtems_event_send to wake the CallbackTask.
 *
 * No spin loop.  No rcl_wait.  No user-level detection.
 * The OS kernel triggers callback execution via direct notification.
 */
int RTExecutor::register_rt_subscription(
  rclcpp::SubscriptionBase::SharedPtr subscription,
  rtems_task_priority priority,
  const std::string & topic_name)
{
  if (!subscription) { return -1; }

  int idx = static_cast<int>(sub_regs_.size());

  auto task = std::make_unique<CallbackTask>(priority, "SUB" + std::to_string(idx));

  /* Build the callback: take + handle + reset callback_group.
   * Captures shared_ptrs by value for lifetime safety. */
  auto sub_ptr = subscription;
  auto callback_group = subscription->get_callback_group();

  task->set_callback([sub_ptr, callback_group]() {
    rclcpp::MessageInfo message_info;
    message_info.get_rmw_message_info().from_intra_process = false;

    if (sub_ptr->is_serialized()) {
      std::shared_ptr<SerializedMessage> serialized_msg =
        sub_ptr->create_serialized_message();
      bool taken = false;
      try {
        taken = sub_ptr->take_serialized(*serialized_msg.get(), message_info);
      } catch (const rclcpp::exceptions::RCLError &) {}
      if (taken) {
        sub_ptr->handle_serialized_message(serialized_msg, message_info);
      }
      sub_ptr->return_serialized_message(serialized_msg);
    } else if (sub_ptr->can_loan_messages()) {
      void * loaned_msg = nullptr;
      bool taken = false;
      try {
        rcl_ret_t ret = rcl_take_loaned_message(
          sub_ptr->get_subscription_handle().get(),
          &loaned_msg,
          &message_info.get_rmw_message_info(),
          nullptr);
        taken = (RCL_RET_OK == ret);
      } catch (...) { taken = false; }
      if (taken && loaned_msg) {
        sub_ptr->handle_loaned_message(loaned_msg, message_info);
        rcl_return_loaned_message_from_subscription(
          sub_ptr->get_subscription_handle().get(), loaned_msg);
      }
    } else {
      std::shared_ptr<void> message = sub_ptr->create_message();
      bool taken = false;
      try {
        taken = sub_ptr->take_type_erased(message.get(), message_info);
      } catch (const rclcpp::exceptions::RCLError &) {}
      if (taken) {
        sub_ptr->handle_message(message, message_info);
      }
      sub_ptr->return_message(message);
    }

    if (callback_group &&
        callback_group->type() == CallbackGroupType::MutuallyExclusive)
    {
      callback_group->can_be_taken_from().store(true);
    }
  });

  /* Start the CallbackTask NOW so its task_id is valid */
  if (!task->start()) {
    std::cerr << "[RTExecutor] Failed to start subscription CallbackTask" << std::endl;
    return -1;
  }

  /* ---- KEY: Set direct notification on the intra-process buffer ----
   *
   * When a publisher publishes intra-process, the publishing path calls:
   *   IntraProcessBuffer::provide_intra_process_data()
   *     → buffer_->add_shared(message)
   *     → trigger_guard_condition()   ← SKIPPED (direct_notify set)
   *     → invoke_on_new_message()     ← calls direct_notify_callback_
   *
   * The direct_notify_callback_ sends rtems_event_send to the
   * CallbackTask.  No spin loop, no rcl_wait, no user-level
   * detection.  The OS kernel directly wakes the CallbackTask.
   */
  rtems_id tid = task->task_id();

  /* ---- Intra-process direct notify ----
   * When publisher uses intra_process_comms=true:
   *   Publisher → IntraProcessBuffer → direct_notify_callback_
   *   → rtems_event_send(CallbackTask)
   */
  subscription->set_intra_process_direct_notify([tid]() {
    rtems_event_send(tid, RTEMS_EVENT_1);
  });

  /* ---- DDS / inter-process notification (rclcpp-rtems addition) ----
   * When intra_process_comms=false or the message arrives via DDS:
   *   rmw receives message → on_new_message_callback_(count)
   *   → our wrapper sends rtems_event_send → CallbackTask wakes up
   *
   * We wrap the existing DDS callback (which triggers guard condition
   * for standard executors) with an additional rtems_event_send.
   * The original callback is preserved so guard conditions still work
   * for any co-existing standard executor logic.
   */
  {
    auto previous_dds_cb = subscription->get_on_new_message_callback();
    subscription->set_on_new_message_callback(
      [tid, previous_dds_cb](size_t count) {
        /* Wake the CallbackTask — kernel-driven, no spin loop */
        rtems_event_send(tid, RTEMS_EVENT_1);
        /* Forward to original callback if any */
        if (previous_dds_cb) {
          previous_dds_cb(count);
        }
      });
  }

  CallbackTask * task_ptr = task.get();
  sub_task_map_[subscription.get()] = task_ptr;

  SubReg reg;
  reg.subscription = subscription;
  reg.task = std::move(task);
  reg.topic_name = topic_name;
  sub_regs_.push_back(std::move(reg));

  std::cout << "[RTExecutor] Registered subscription '" << topic_name
            << "' pri=" << static_cast<int>(priority)
            << " direct_notify=ON" << std::endl;
  return idx;
}

bool RTExecutor::setup_timer_driver()
{
  if (timer_regs_.empty()) { return true; }

  dev_fd_ = open(RTSS_DEVICE_NAME, O_RDWR);
  if (dev_fd_ < 0) {
    std::cerr << "[RTExecutor] Cannot open " << RTSS_DEVICE_NAME
              << " (" << strerror(errno) << ")" << std::endl;
    return false;
  }

  rtss_timer_mode_config mode_cfg;
  mode_cfg.mode = RTSS_MODE_EVENT;
  if (ioctl(dev_fd_, RTSS_TIMER_SET_MODE, &mode_cfg) < 0) {
    std::cerr << "[RTExecutor] SET_MODE failed" << std::endl;
    return false;
  }

  for (auto & reg : timer_regs_) {
    if (!reg.task->is_running()) {
      if (!reg.task->start()) { continue; }
    }

    rtss_timer_config cfg;
    cfg.channel = reg.channel;
    cfg.period_ticks = reg.period_ticks;
    if (ioctl(dev_fd_, RTSS_TIMER_SET_PERIOD, &cfg) < 0) { continue; }

    rtss_timer_task_config task_cfg;
    task_cfg.channel = reg.channel;
    task_cfg.target_task = reg.task->task_id();
    if (ioctl(dev_fd_, RTSS_TIMER_SET_TASK, &task_cfg) < 0) {
      std::cerr << "[RTExecutor] SET_TASK failed ch=" << reg.channel << std::endl;
    }

    if (ioctl(dev_fd_, RTSS_TIMER_START, &cfg) < 0) { continue; }

    std::cout << "[RTExecutor] Timer ch=" << reg.channel
              << " period=" << reg.period_ticks << "t"
              << " target=" << reg.task->task_id() << std::endl;
  }

  return true;
}

void RTExecutor::teardown_timer_driver()
{
  if (dev_fd_ < 0) { return; }
  for (auto & reg : timer_regs_) {
    rtss_timer_config cfg;
    cfg.channel = reg.channel;
    cfg.period_ticks = 0;
    ioctl(dev_fd_, RTSS_TIMER_STOP, &cfg);
  }
  close(dev_fd_);
  dev_fd_ = -1;
}

void RTExecutor::start_all_tasks()
{
  for (auto & reg : timer_regs_) {
    if (!reg.task->is_running()) { reg.task->start(); }
  }
  /* Subscription CallbackTasks are already started in register_rt_subscription() */
}

void RTExecutor::stop_all_tasks()
{
  for (auto & reg : timer_regs_) { reg.task->stop(); }
  for (auto & reg : sub_regs_) { reg.task->stop(); }
}

/**
 * @brief Start the executor — NO spin loop, NO rcl_wait, NO polling
 *
 * All callbacks are triggered by OS kernel events:
 *   Timer:       HW tick ISR → rtems_event_send(CBTask)
 *   Subscription: IntraProcess publish → direct_notify_callback_
 *              → rtems_event_send(CBTask)
 *
 * spin() starts the CallbackTasks and the timer driver,
 * then blocks on rtems_event_receive(SHUTDOWN_EVENT).
 * No polling.  No detection loop.  No user logic.
 */
void RTExecutor::spin()
{
  if (spinning.exchange(true)) {
    throw std::runtime_error("spin() called while already spinning");
  }
  RCPPUTILS_SCOPE_EXIT(this->spinning.store(false); );

  start_all_tasks();
  setup_timer_driver();

  spin_task_id_ = rtems_task_self();

  std::cout << "[RTExecutor] Running — ALL callbacks kernel-driven, "
            << "NO polling" << std::endl;

  /* Block until cancel() sends SHUTDOWN_EVENT.
   * All callback execution is driven by OS kernel events. */
  rtems_event_set events;
  rtems_event_receive(
    SHUTDOWN_EVENT,
    RTEMS_EVENT_ANY | RTEMS_WAIT,
    RTEMS_NO_TIMEOUT,
    &events);

  std::cout << "[RTExecutor] Exited" << std::endl;
}

void RTExecutor::cancel()
{
  spinning.store(false);
  if (spin_task_id_ != RTEMS_ID_NONE) {
    rtems_event_send(spin_task_id_, SHUTDOWN_EVENT);
  }
}
