// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef RCLCPP__EXECUTORS__NO_EXECUTOR_HPP_
#define RCLCPP__EXECUTORS__NO_EXECUTOR_HPP_

#include <rmw/rmw.h>

#include <cassert>
#include <cstdlib>
#include <memory>
#include <signal.h>
#include <vector>
#include <pthread.h>

#ifdef RTEMS
#include <time.h>
#endif

#include "rclcpp/executor.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp/memory_strategies.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/utilities.hpp"
#include "rclcpp/rate.hpp"
#include "rclcpp/sched_base.hpp"
#include "rclcpp/visibility_control.hpp"

#include "rclcpp/cond.hpp"
#include "rclcpp/stack.hpp"

namespace rclcpp
{
namespace executors
{
class NoExecutor;

enum ExecutableType {
  SUBSCRIPTION,
  SERVICE,
  CLIENT,
  WAITABLE,
  TIMER
};

struct Executable {
  ExecutableType type;
  CallbackGroup::SharedPtr callback_group;
  rclcpp::SubscriptionBase::SharedPtr subscription = nullptr;
  rclcpp::ServiceBase::SharedPtr service = nullptr;
  rclcpp::ClientBase::SharedPtr client = nullptr;
  rclcpp::Waitable::SharedPtr waitable = nullptr;
  rclcpp::TimerBase::SharedPtr timer = nullptr;
  /* Per-dispatch timing (set in assign_or_create, read in execute_executable) */
  int64_t dispatch_done_ns = 0;    /* T3: dispatch completion timestamp */
  int64_t dispatch_dur_ns = 0;     /* T3 - T0: signal entry → dispatch complete */
  int64_t assign_dur_ns = 0;       /* T3 - T1: assign_or_create body */
};

struct ThreadDataNoExec {
  syncutil::Condition is_busy;
  Executable executable;
  pthread_t pthread_id;
  pid_t pid;
};

struct PosixTimer {
  NoExecutor* executor;
  uint64_t period;
  rclcpp::TimerBase::SharedPtr timer;
  rclcpp::CallbackGroup::SharedPtr callback_group;
  timer_t timerid;
  int index;
};

/// Single-threaded executor implementation.
/**
 * This is the default executor created by rclcpp::spin.
 */
class NoExecutor : public rclcpp::Executor
{
using rclcpp::Executor::add_node;
using rclcpp::Executor::remove_node;
public:
  RCLCPP_SMART_PTR_DEFINITIONS(NoExecutor)

  /// Default constructor. See the default constructor for Executor.
  RCLCPP_PUBLIC
  explicit NoExecutor(
    const rclcpp::ExecutorOptions & options = rclcpp::ExecutorOptions());

  /// Default destructor.
  RCLCPP_PUBLIC
  virtual ~NoExecutor();

  void 
  start();

  RCLCPP_PUBLIC
  void
  spin() override;


  void
  stop();

  void
  execute_executable(Executable &executable);

  syncutil::StackAtomic<ThreadDataNoExec> idle_threads;

  RCLCPP_PUBLIC
  void
  add_node(std::shared_ptr<rclcpp::Node> node_ptr, bool notify = true) override;

  RCLCPP_PUBLIC
  void
  remove_node(std::shared_ptr<rclcpp::Node> node_ptr, bool notify = true) override;

  bool started;

  static uint64_t 
  get_period_from_timer(const rclcpp::TimerBase::SharedPtr &timer);

  void
  assign_or_create(Executable &executable);

private:
  void
  handle_subscription(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::SubscriptionBase::SharedPtr &subscription, size_t num_msgs);

  void
  handle_service(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::ServiceBase::SharedPtr &service, size_t num_msgs);

  void
  handle_client(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::ClientBase::SharedPtr &client, size_t num_msgs);

  void
  handle_waitable(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::Waitable::SharedPtr &waitable, size_t num_msgs);

  void
  create_thread(Executable executable);
  void
  thread_start(Executable executable);

public:
  std::vector<PosixTimer*> timers;
};

}  // namespace executors
}  // namespace rclcpp

#ifdef RTEMS
/* POSIX timer baseline — clock_gettime(CLOCK_MONOTONIC) right before
 * the first timer_settime() in NoExecutor::start().  User callbacks can
 * use this as the definitive "time zero" for absolute deviation measurement.
 * Defined in no_executor.cpp at file scope. */
extern volatile int64_t g_posix_timer_base_ns;

/* Per-timer dispatch completion timestamp (T3 from assign_or_create).
 * Read by user callbacks to measure jitter from dispatch done.
 * Defined in no_executor.cpp at file scope. */
#define G_NE_MAX_TIMERS 8
extern volatile int64_t g_ne_dispatch_done_ns[G_NE_MAX_TIMERS];
extern volatile int64_t g_ne_dispatch_dur_ns[G_NE_MAX_TIMERS];
extern volatile int64_t g_ne_assign_dur_ns[G_NE_MAX_TIMERS];
/* Per-callback dispatch context (set in execute_executable, read by user callback, same thread) */
extern volatile int64_t g_ne_cb_dispatch_done_ns;
extern volatile int64_t g_ne_cb_dispatch_dur_ns;
extern volatile int64_t g_ne_cb_assign_dur_ns;
#endif

#endif  // RCLCPP__EXECUTORS__SINGLE_THREADED_EXECUTOR_HPP_
