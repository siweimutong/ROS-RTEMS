// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifdef RTEMS
/* SingleThreadedExecutor for RTEMS.
 * Simple sequential spin: get_next_executable → execute_any_executable. */

#include "rcpputils/scope_exit.hpp"
#include "rclcpp/executors/single_threaded_executor.hpp"

using rclcpp::executors::SingleThreadedExecutor;

SingleThreadedExecutor::SingleThreadedExecutor(const rclcpp::ExecutorOptions & options)
: rclcpp::Executor(options) {}

SingleThreadedExecutor::~SingleThreadedExecutor() {}

void
SingleThreadedExecutor::spin()
{
  if (spinning.exchange(true)) {
    throw std::runtime_error("spin() called while already spinning");
  }
  RCPPUTILS_SCOPE_EXIT(this->spinning.store(false); );
  while (rclcpp::ok(this->context_) && spinning.load()) {
    spin_some_impl(std::chrono::milliseconds(100), true);
  }
}

bool
SingleThreadedExecutor::get_next_ready_executable_from_map(
  AnyExecutable & any_executable,
  const rclcpp::memory_strategy::MemoryStrategy::WeakCallbackGroupsToNodesMap &
  weak_groups_to_nodes)
{
  (void)any_executable;
  (void)weak_groups_to_nodes;
  return false;
}

#else
/* Original Linux implementation */
#include "rcpputils/scope_exit.hpp"

#include <cassert>
#include <pthread.h>
#include <stdio.h>
#include <thread>
#include <signal.h>
#include <sys/syscall.h>
#include <unordered_set>
#include <sched.h>
#include <system_error>

#include "rclcpp/callback_group.hpp"

#include "rclcpp/executors/single_threaded_executor.hpp"
#include "rclcpp/any_executable.hpp"
#include "rclcpp/sched_base.hpp"

#include "tracetools/tracetools.h"

#define UNUSED(expr) do { (void)(expr); } while (0)
#define DEFAULT_INTERVAL 500'000
#define SEC_IN_NSEC 1'000'000'000

using rclcpp::executors::SingleThreadedExecutor;

int trace_fd = -1;
int marker_fd = -1;
cpu_set_t ext_cpuset;

struct t_eventData {
    syncutil::Condition* signal_scheduler_ptr;
};

void handler(int sig, siginfo_t *si, void *uc) {
  UNUSED(sig);
  UNUSED(uc);
  t_eventData *data = (t_eventData *) si->_sifields._rt.si_sigval.sival_ptr;
  data->signal_scheduler_ptr->set_val(1, true);
}

void
SingleThreadedExecutor::thread_start_idle() {
  rclcpp::executors::ThreadData thread_data;
  thread_data.is_busy.set_val(0, false);
  pthread_t self = pthread_self();
  thread_data.pthread_id = self;
  thread_data.pid = sched::get_pid(self);
  rclcpp::sched::SchedAttr idle_sched_attr;
  idle_sched_attr.sched_policy = SCHED_FIFO;
  idle_sched_attr.sched_priority = 98;
  thread_data.sched_attr = &idle_sched_attr;
  syscall_sched_setattr(0, &idle_sched_attr);
  this->idle_threads.push(&thread_data);
  while (true) {
    thread_data.is_busy.wait_on(0);
    this->execute_executable(thread_data.any_exec, thread_data.message, thread_data.message_info);
    thread_data.is_busy.set_val(0, false);
    this->idle_threads.push(&thread_data);
    syscall_sched_setattr(0, &idle_sched_attr);
  }
}

void
SingleThreadedExecutor::thread_start(AnyExecutable any_exec, std::shared_ptr<void>& message, rclcpp::MessageInfo* message_info, rclcpp::sched::SchedAttr* sched_attr) {
  rclcpp::executors::ThreadData thread_data(std::move(any_exec));
  thread_data.message = message;
  thread_data.message_info = message_info;
  thread_data.is_busy.set_val(1, false);
  pthread_t self = pthread_self();
  thread_data.pthread_id = self;
  thread_data.pid = sched::get_pid(self);
  thread_data.sched_attr = sched_attr;
  while (true) {
    thread_data.is_busy.wait_on(0);
    this->execute_executable(thread_data.any_exec, thread_data.message, thread_data.message_info);
    thread_data.is_busy.set_val(0, false);
    this->idle_threads.push(&thread_data);
  }
}

void SingleThreadedExecutor::execute_executable(AnyExecutable any_exec, std::shared_ptr<void>& message, rclcpp::MessageInfo* message_info) {
  if (any_exec.callback_group->type() == CallbackGroupType::MutuallyExclusive) {
    any_exec.callback_group->callback_group_mutex.lock();
    if (any_exec.subscription == nullptr) {
      execute_any_executable(any_exec);
    } else {
      if (strcmp("/parameter_events", any_exec.subscription->get_topic_name())) {
        assert(message);
        assert(message_info);
      }
      any_exec.subscription->handle_message(message, *message_info);
      any_exec.subscription->return_message(message);
      delete message_info;
    }
    any_exec.callback_group->callback_group_mutex.unlock();
    any_exec.callback_group.reset();
    return;
  }
  if (any_exec.subscription == nullptr) {
    execute_any_executable(any_exec);
  } else {
    assert(message);
    assert(message_info);
    any_exec.subscription->handle_message(message, *message_info);
    any_exec.subscription->return_message(message);
    delete message_info;
  }
  any_exec.callback_group.reset();
}

bool SingleThreadedExecutor::get_next_ready_executable_from_map(
  AnyExecutable & any_executable,
  const rclcpp::memory_strategy::MemoryStrategy::WeakCallbackGroupsToNodesMap &
  weak_groups_to_nodes)
{
  bool success = false;
  std::lock_guard<std::mutex> guard{mutex_};
  memory_strategy_->get_next_timer(any_executable, weak_groups_to_nodes);
  if (any_executable.timer) { success = true; }
  if (!success) {
    memory_strategy_->get_next_subscription(any_executable, weak_groups_to_nodes);
    if (any_executable.subscription) { success = true; }
  }
  if (!success) {
    memory_strategy_->get_next_service(any_executable, weak_groups_to_nodes);
    if (any_executable.service) { success = true; }
  }
  if (!success) {
    memory_strategy_->get_next_client(any_executable, weak_groups_to_nodes);
    if (any_executable.client) { success = true; }
  }
  if (!success) {
    memory_strategy_->get_next_waitable(any_executable, weak_groups_to_nodes);
    if (any_executable.waitable) {
      any_executable.data = any_executable.waitable->take_data();
      success = true;
    }
  }
  if (success) {
    rclcpp::CallbackGroup::WeakPtr weak_group_ptr = any_executable.callback_group;
    auto iter = weak_groups_to_nodes.find(weak_group_ptr);
    if (iter == weak_groups_to_nodes.end()) {
      std::cout << "Couldn't find the callback group" << std::endl;
      success = false;
    }
  }
  return success;
}

inline rclcpp::sched::SchedAttr* SingleThreadedExecutor::get_sched_attr(const AnyExecutable& any_exec) {
  if (any_exec.subscription != nullptr) return &(any_exec.subscription->sched_attr);
  if (any_exec.timer != nullptr) return &(any_exec.timer->sched_attr);
  if (any_exec.service != nullptr) return &(any_exec.service->sched_attr);
  if (any_exec.client != nullptr) return &(any_exec.client->sched_attr);
  if (any_exec.waitable != nullptr) return &(any_exec.waitable->sched_attr);
  assert(false);
  return nullptr;
}

inline rclcpp::sched::edf_sched_entity* SingleThreadedExecutor::get_sched_entity(const AnyExecutable& any_exec) {
  if (any_exec.subscription != nullptr) return &(any_exec.subscription->sched_entity);
  if (any_exec.timer != nullptr) return &(any_exec.timer->sched_entity);
  if (any_exec.service != nullptr) return &(any_exec.service->sched_entity);
  if (any_exec.client != nullptr) return &(any_exec.client->sched_entity);
  if (any_exec.waitable != nullptr) return &(any_exec.waitable->sched_entity);
  assert(false);
  return nullptr;
}

static bool take_message(rclcpp::AnyExecutable& any_exec, std::shared_ptr<void>& message, rclcpp::MessageInfo** message_info_ptr)
{
  rclcpp::MessageInfo* message_info = new rclcpp::MessageInfo;
  message_info->get_rmw_message_info().from_intra_process = false;
  message = any_exec.subscription->create_message();
  assert(message);
  assert(message.get() != nullptr);
  bool taken = false;
  try {
    taken = any_exec.subscription->take_type_erased(message.get(), *message_info);
  } catch (const rclcpp::exceptions::RCLError & rcl_error) {
    (void)rcl_error;
  }
  if (!taken) {
    any_exec.subscription->return_message(message);
    delete message_info;
    message_info = nullptr;
  }
  if (taken) {
    assert(message_info);
    assert(message.get() != nullptr);
  }
  *message_info_ptr = message_info;
  return taken;
}

inline void SingleThreadedExecutor::create_idle_thread() {
  std::thread new_thread(std::bind(&SingleThreadedExecutor::thread_start_idle, this));
  new_thread.detach();
}

inline void SingleThreadedExecutor::create_thread(AnyExecutable any_exec, std::shared_ptr<void>& message, rclcpp::MessageInfo* message_info) {
  auto sched_entity = get_sched_entity(any_exec);
  auto attr = get_sched_attr(any_exec);
  const std::string nodeName = any_exec.node_base->get_name();
  if (any_exec.subscription) {
    assert(message.get() != nullptr);
    assert(message_info != nullptr);
  }
  try {
    std::thread new_thread(std::bind(&SingleThreadedExecutor::thread_start, this, std::move(any_exec), message, message_info, attr));
    if (sched_setaffinity(sched::get_pid(new_thread.native_handle()), sizeof(cpu_set_t), &ext_cpuset) == -1) {
      std::cerr << "Error setting CPU affinity: " << strerror(errno) << std::endl;
    }
    if (sched_entity->edf_attr) {
      if (sched_entity->is_source) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        sched_entity->edf_attr->abs_deadline = (uint64_t) now.tv_sec * SEC_IN_NSEC + now.tv_nsec + sched_entity->relative_deadline;
      }
      sched::update_deadline(new_thread.native_handle(), sched_entity->edf_attr);
    } else {
      sched::syscall_sched_setattr(sched::get_pid(new_thread.native_handle()), attr);
    }
    new_thread.detach();
  } catch(const std::system_error& e) {
    std::cout << "Caught system_error with code [" << e.code() << "] meaning [" << e.what() << "]\n";
    std::cout << "Failed to create thread for node " << nodeName << std::endl;
  }
}

void SingleThreadedExecutor::assign_or_create(AnyExecutable any_exec) {
  std::shared_ptr<void> message(nullptr);
  rclcpp::MessageInfo* message_info = nullptr;
  if (any_exec.subscription) {
    bool taken = take_message(any_exec, message, &message_info);
    assert(taken);
    if (!taken) return;
    assert(message.get() != nullptr);
    assert(message_info != nullptr);
  }
  auto idle_thread = idle_threads.pop();
  if (idle_thread == nullptr) {
    create_thread(std::move(any_exec), message, message_info);
    return;
  }
  auto sched_entity = get_sched_entity(any_exec);
  auto attr = get_sched_attr(any_exec);
  assert(attr != nullptr);
  idle_thread->any_exec = std::move(any_exec);
  idle_thread->message = message;
  idle_thread->message_info = message_info;
  idle_thread->sched_attr = attr;
  int res = 0;
  if (sched_entity->edf_attr) {
    if (sched_entity->is_source) {
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);
      sched_entity->edf_attr->abs_deadline = (uint64_t) now.tv_sec * SEC_IN_NSEC + now.tv_nsec + sched_entity->relative_deadline;
    }
    res = (sched::update_deadline(idle_thread->pthread_id, sched_entity->edf_attr) == false);
  } else {
    res = sched::syscall_sched_setattr(idle_thread->pid, attr);
  }
  if (res != 0) {
    perror("Error while setting idle thread's sched_attr: ");
    RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Error: Tried setting idle thread (pid=%d) with sched_attr, but got return code %d", idle_thread->pid, res);
  }
  assert(res == 0);
  idle_thread->is_busy.set_val(1, true);
}

SingleThreadedExecutor::SingleThreadedExecutor(const rclcpp::ExecutorOptions & options)
: rclcpp::Executor(options) {}

SingleThreadedExecutor::~SingleThreadedExecutor() {}

void
SingleThreadedExecutor::spin() {
  printf("Spinning\n");
  if (spinning.exchange(true)) {
    throw std::runtime_error("spin() called while already spinning");
  }
  CPU_ZERO(&ext_cpuset);
  CPU_SET(10, &ext_cpuset);
  CPU_SET(11, &ext_cpuset);
  CPU_SET(12, &ext_cpuset);
  CPU_SET(13, &ext_cpuset);
  char* core_count = getenv("ROS_CORE_COUNT");
  (void)core_count;
  RCPPUTILS_SCOPE_EXIT(this->spinning.store(false); );
  int period_ns;
  char* method = getenv("ROS_SCHED_METHOD");
  char* period_str = getenv("ROS_SCHED_PERIOD");
  for (int i = 0; i < 50; i++) {
    printf("Warming up, iter: %d\n", i);
    AnyExecutable executable;
    bool success = get_next_executable(executable, std::chrono::nanoseconds(500000));
    if (success) {
      printf("success at iter: %d\n", i);
      execute_any_executable(executable);
    }
  }
  std::cout << "Warmup complete" << std::endl;
  if (period_str == nullptr) {
    period_ns = DEFAULT_INTERVAL;
  } else {
    period_ns = atoi(period_str);
    assert(period_ns < SEC_IN_NSEC);
  }
  if (method == nullptr || strcmp(method, "DEADLINE") == 0) {
    spin_deadline(period_ns);
  } else if (strcmp(method, "SLEEP") == 0) {
    spin_sleep(period_ns);
  } else if (strcmp(method, "TIMER") == 0) {
    spin_timer(period_ns);
  } else if (strcmp(method, "SPIN") == 0) {
    spin_forever();
  } else {
    assert(false);
  }
}

void
SingleThreadedExecutor::spin_timer(int period_ns) {
  pid_t cur_tid = gettid();
  timer_t timerId = 0;
  t_eventData eventData = {&signal_scheduler};
  union sigval sigv;
  sigv.sival_ptr = &eventData;
  struct sigevent sev = {};
  sev.sigev_notify = SIGEV_THREAD_ID;
  sev.sigev_signo = SIGRTMIN;
  sev.sigev_value = sigv;
  sev._sigev_un._tid = cur_tid;
  struct sigaction sa = {};
  struct itimerspec its = {};
  struct timespec it_interval = {};
  struct timespec it_value = {};
  it_interval.tv_nsec = period_ns;
  it_value.tv_nsec = period_ns;
  its.it_interval = it_interval;
  its.it_value = it_value;
  printf("Signal Interrupt Timer - thread-id: %d\n", gettid());
  if (timer_create(CLOCK_MONOTONIC, &sev, &timerId)) return;
  sa.sa_flags = (SA_SIGINFO | SA_RESTART);
  sa.sa_sigaction = handler;
  sigemptyset(&sa.sa_mask);
  printf("Establishing handler for signal %d\n", SIGRTMIN);
  if (sigaction(SIGRTMIN, &sa, NULL)) return;
  printf("starting timer\n");
  if(timer_settime(timerId, 0, &its, NULL)) return;
  while (rclcpp::ok(this->context_) && spinning.load()) {
    this->schedule();
  }
}

void inc_period(struct timespec& period_time, int period_ns) {
  period_time.tv_nsec += period_ns;
  while (period_time.tv_nsec >= 1000000000) {
    period_time.tv_sec++;
    period_time.tv_nsec -= 1000000000;
  }
}

void
SingleThreadedExecutor::spin_sleep(int period_ns) {
  sched::SchedAttr attr;
  attr.sched_policy = SCHED_FIFO;
  attr.sched_priority = 99;
  assert(sched::syscall_sched_setattr(gettid(), &attr) == 0);
  struct timespec period_point;
  int flags = TIMER_ABSTIME;
  int err = 0;
  assert(clock_gettime(CLOCK_MONOTONIC, &period_point) == 0);
  inc_period(period_point, period_ns);
  struct timespec wake_time_actual;
  wake_time_actual.tv_sec = 9;
  wake_time_actual.tv_nsec = 9;
  while (rclcpp::ok(this->context_) && spinning.load()) {
    do {
      err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &period_point, NULL);
    } while (err != 0 && errno == EINTR);
    assert(err == 0);
    int r = clock_gettime(CLOCK_MONOTONIC, &wake_time_actual);
    assert(r == 0);
    this->schedule();
    r = clock_gettime(CLOCK_MONOTONIC, &period_point);
    assert(r == 0);
    inc_period(period_point, period_ns);
  }
  (void)flags;
}

void
SingleThreadedExecutor::spin_deadline(int period_ns) {
  std::cout << "In spin_deadline" << std::endl;
  for (int i = 0; i < 500; i++) {
    create_idle_thread();
  }
  sched::SchedAttr attr;
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_priority = 0;
  attr.sched_period = period_ns;
  attr.sched_runtime = period_ns;
  attr.sched_deadline = period_ns;
  attr.sched_flags |= 0x04;
  sched::syscall_sched_setattr(gettid(), &attr);
  while (rclcpp::ok(this->context_) && spinning.load()) {
    this->schedule();
    sched_yield();
  }
}

void SingleThreadedExecutor::schedule() {
  int num_cb_dispatched = 0;
  rclcpp::AnyExecutable executable;
  if (!get_next_executable(executable, std::chrono::nanoseconds::zero())) {
    return;
  }
  assign_or_create(std::move(executable));
  num_cb_dispatched++;
  while (true) {
    rclcpp::AnyExecutable ready_executable;
    if (!get_next_ready_executable(ready_executable)) {
      return;
    }
    assign_or_create(std::move(ready_executable));
    num_cb_dispatched++;
  }
}

void
SingleThreadedExecutor::spin_forever() {
  sched::SchedAttr attr;
  attr.sched_policy = SCHED_FIFO;
  attr.sched_priority = 99;
  assert(sched::syscall_sched_setattr(gettid(), &attr) == 0);
  rclcpp::AnyExecutable executable;
  while (rclcpp::ok(this->context_) && spinning.load()) {
    if (!get_next_executable(executable, std::chrono::nanoseconds(-1))) {
      continue;
    }
    assign_or_create(std::move(executable));
  }
}

#endif
