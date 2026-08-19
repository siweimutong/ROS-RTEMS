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

#include "rcpputils/scope_exit.hpp"

#include <cassert>
#include <pthread.h>
#include <stdio.h>
#include <thread>
#include <signal.h>
#ifndef RTEMS
#include <sys/syscall.h>
#endif
#include <unordered_set>
#ifndef RTEMS
#include <sched.h>
#endif
#include <system_error>
#include <atomic>

#include "rclcpp/callback_group.hpp"

#include "rclcpp/executors/no_executor.hpp"
#include "rclcpp/sched_base.hpp"

//#include "tracetools/tracetools.h"

#ifdef RTEMS
#include <rtems.h>
/* Exp2.2 instrumentation: NoExecutor dispatch overhead
 *
 * Measurement points along the dispatch path:
 *
 *   T0: handle_timer() signal handler entry
 *       │
 *       ├─ construct Executable
 *       ├─ assign_or_create() entry ───── T1
 *       │     ├─ idle_threads.pop()
 *       │     ├─ get_sched_base()
 *       │     ├─ sched_setattr / EDF
 *       │     └─ is_busy.set_val(1) ──── T3  (user dispatch done)
 *       │
 *   T4: thread_start() wait_on(0) returns  (thread woke up)
 *       │
 *   T5: execute_executable() entry          (callback about to run)
 *
 * Breakdown:
 *   t_dispatch = T3 - T0  (signal entry → user dispatch complete)
 *   t_assign   = T3 - T1  (assign_or_create body)
 *   t_wakeup   = T4 - T3  (futex wakeup latency)
 *   t_preexec  = T5 - T4  (thread woke → callback entry)
 *   t_total    = T5 - T0  (signal → callback execution)
 */
volatile uint64_t g_ne_dispatch_ns = 0;
volatile int      g_ne_dispatch_calls = 0;
volatile uint64_t g_ne_dispatch_min_ns = (uint64_t)-1;
volatile uint64_t g_ne_dispatch_max_ns = 0;
volatile uint64_t g_ne_assign_ns = 0;
volatile int      g_ne_assign_calls = 0;
volatile uint64_t g_ne_wakeup_ns = 0;
volatile int      g_ne_wakeup_calls = 0;
volatile uint64_t g_ne_preexec_ns = 0;
volatile int      g_ne_preexec_calls = 0;
volatile int      g_ne_raw_enabled = 0;

/* Per-timer dispatch completion timestamp (T3).
 * Set in assign_or_create() on the spin thread, read by
 * user callbacks on worker threads.
 * g_ne_dispatch_done_ns[ptimer->index] = T3 timestamp in ns. */
#define G_NE_MAX_TIMERS 8
volatile int64_t  g_ne_dispatch_done_ns[G_NE_MAX_TIMERS];
/* Per-timer dispatch and assign durations (set in assign_or_create, read by callbacks) */
volatile int64_t  g_ne_dispatch_dur_ns[G_NE_MAX_TIMERS];
volatile int64_t  g_ne_assign_dur_ns[G_NE_MAX_TIMERS];
/* Per-callback dispatch context (set in execute_executable from Executable struct,
 * read by user callbacks on the same worker thread — no race condition) */
volatile int64_t  g_ne_cb_dispatch_done_ns = 0;
volatile int64_t  g_ne_cb_dispatch_dur_ns = 0;
volatile int64_t  g_ne_cb_assign_dur_ns = 0;

/* Global pointer for RTEMS signal handler workaround (si_value bug) */
rclcpp::executors::NoExecutor *g_active_executor = nullptr;

static inline uint64_t rtems_uptime_ns_fast() {
  struct timespec ts;
  rtems_clock_get_uptime(&ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Thread-local timestamps: set in one function, read in another
 * on the same thread (signal handler and assign_or_create run on
 * the spin() thread; thread_start runs on worker threads). */
static __thread uint64_t tl_t0 = 0;   /* handle_timer entry */
static __thread uint64_t tl_t1 = 0;   /* assign_or_create entry */
static __thread uint64_t tl_t3 = 0;   /* assign_or_create done */
static __thread uint64_t tl_t4 = 0;   /* wait_on returns (worker thread) */
/* Per-dispatch timing read by user callback (set from Executable struct in execute_executable) */
static __thread int64_t tl_dispatch_done_ns = 0;
static __thread int64_t tl_dispatch_dur_ns = 0;
static __thread int64_t tl_assign_dur_ns = 0;
#endif

#define UNUSED(expr) do { (void)(expr); } while (0)
#define SEC_IN_NSEC 1'000'000'000

using std::placeholders::_1;
using rclcpp::executors::NoExecutor;
using rclcpp::executors::Executable;
using rclcpp::executors::ExecutableType;
using rclcpp::executors::PosixTimer;

struct itimerspec unset_timer = {};
void
handle_timer(int sig, siginfo_t *si, void *uc);

NoExecutor::NoExecutor(const rclcpp::ExecutorOptions & options)
: rclcpp::Executor(options) {
  started = false;
}

NoExecutor::~NoExecutor() {
  stop();
}

void
NoExecutor::start() {
#ifdef RTEMS
  g_active_executor = this;
  for (PosixTimer *timer: timers) {
    timer_t timerId = 0;
    union sigval sigv;
    sigv.sival_ptr = (void *) timer;
    struct sigevent sev = {};
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMAX;
    sev.sigev_value = sigv;
    assert(timer_create(CLOCK_MONOTONIC, &sev, &timerId) == 0);
    timer->timerid = timerId;
  }
#else
  pid_t cur_tid = gettid();
  for (PosixTimer *timer: timers) {
    timer_t timerId = 0;
    union sigval sigv;
    sigv.sival_ptr = (void *) timer;
    struct sigevent sev = {};
    sev.sigev_notify = SIGEV_THREAD_ID;
    sev.sigev_signo = SIGRTMAX;
    sev.sigev_value = sigv;
    sev._sigev_un._tid = cur_tid;
    assert(timer_create(CLOCK_MONOTONIC, &sev, &timerId) == 0);
    timer->timerid = timerId;
  }
#endif

  struct sigaction sa = {};
#ifdef RTEMS
  sa.sa_flags = SA_SIGINFO;
#else
  sa.sa_flags = (SA_SIGINFO | SA_NODEFER | SA_RESTART);
#endif
  sa.sa_sigaction = handle_timer;
  sigemptyset(&sa.sa_mask);
  assert(sigaction(SIGRTMAX, &sa, NULL) == 0);

  started = true;
  for (PosixTimer *timer: timers) {
    struct itimerspec its = {};
    struct timespec it_interval = {};
    struct timespec it_value = {};
    it_interval.tv_nsec = timer->period % SEC_IN_NSEC;
    it_interval.tv_sec = timer->period / SEC_IN_NSEC;
    it_value.tv_nsec = 50;
    it_value.tv_sec = 0;
    its.it_interval = it_interval;
    its.it_value = it_value;
    assert(timer_settime(timer->timerid, 0, &its, NULL) == 0);
  }
}

void
NoExecutor::stop() {
  started = false;
  for (PosixTimer *timer: timers) {
    assert(timer_settime(timer->timerid, 0, &unset_timer, NULL) == 0);
  }
}

void
NoExecutor::execute_executable(Executable &executable) {
#ifdef RTEMS
  /* T5: callback about to execute */
  uint64_t t5 = rtems_uptime_ns_fast();
  uint64_t preexec_dur = 0;
  if (tl_t4 > 0) {
    preexec_dur = t5 - tl_t4;
    g_ne_preexec_ns += preexec_dur;
    g_ne_preexec_calls++;
  }
  if (tl_t0 > 0 && tl_t3 > 0) {
    uint64_t total_dur = t5 - tl_t0;
    if (g_ne_raw_enabled) {
      printf("[E2.2-DETAIL] t_dispatch=%.3f t_wakeup=%.3f t_preexec=%.3f t_total=%.3f\n",
             (double)(tl_t3 - tl_t0) / 1000.0,
             tl_t4 > tl_t3 ? (double)(tl_t4 - tl_t3) / 1000.0 : 0.0,
             (double)preexec_dur / 1000.0,
             (double)total_dur / 1000.0);
    }
  }
  /* Copy per-dispatch timing from Executable struct to thread-local vars
   * and globals. User callbacks read the globals on the same worker thread. */
  tl_dispatch_done_ns = executable.dispatch_done_ns;
  tl_dispatch_dur_ns = executable.dispatch_dur_ns;
  tl_assign_dur_ns = executable.assign_dur_ns;
  g_ne_cb_dispatch_done_ns = executable.dispatch_done_ns;
  g_ne_cb_dispatch_dur_ns = executable.dispatch_dur_ns;
  g_ne_cb_assign_dur_ns = executable.assign_dur_ns;
#endif

  if (executable.callback_group->type() == CallbackGroupType::MutuallyExclusive) {
    executable.callback_group->callback_group_mutex.lock();
  }
  switch (executable.type)
  {
  case ExecutableType::SUBSCRIPTION:
    this->execute_subscription(executable.subscription);
    break;
  case ExecutableType::SERVICE:
    this->execute_service(executable.service);
    break;
  case ExecutableType::CLIENT:
    this->execute_client(executable.client);
    break;
  case ExecutableType::WAITABLE:
    {
      std::shared_ptr<void> data = executable.waitable->take_data();
      executable.waitable->execute(data);
      break;
    }
  case ExecutableType::TIMER:
    executable.timer->execute_callback();
  default:
    break;
  }
  if (executable.callback_group->type() == CallbackGroupType::MutuallyExclusive) {
    executable.callback_group->callback_group_mutex.unlock();
    executable.callback_group.reset();
  }
}

struct rcl_timer_ {
  void* unused[5];
  std::atomic_int_least64_t period;
};

uint64_t
NoExecutor::get_period_from_timer(const rclcpp::TimerBase::SharedPtr &timer) {
  return ((rcl_timer_*) timer->get_timer_handle()->impl)->period;
}


void
NoExecutor::add_node(std::shared_ptr<rclcpp::Node> node_ptr, bool notify) {
  this->add_node(node_ptr->get_node_base_interface(), notify);
  node_ptr->for_each_callback_group([this](rclcpp::CallbackGroup::SharedPtr callback_group) {
    callback_group->collect_all_ptrs(
      [this, &callback_group](const rclcpp::SubscriptionBase::SharedPtr &subscription) {
        subscription->set_on_new_message_callback(std::bind(&NoExecutor::handle_subscription, this, callback_group, subscription, _1));
        subscription->set_on_new_intra_process_message_callback(std::bind(&NoExecutor::handle_subscription, this, callback_group, subscription, _1));
      },
      [this, &callback_group](const rclcpp::ServiceBase::SharedPtr &service) {
        service->set_on_new_request_callback(std::bind(&NoExecutor::handle_service, this, callback_group, service, _1));
      },
      [this, &callback_group](const rclcpp::ClientBase::SharedPtr &client) {
        client->set_on_new_response_callback(std::bind(&NoExecutor::handle_client, this, callback_group, client, _1));
      },
      [this, &callback_group](const rclcpp::TimerBase::SharedPtr &timer) {
        this->timers.push_back(new PosixTimer({this, get_period_from_timer(timer), timer, callback_group, 0, this->timers.size()}));
      },
      [this, &callback_group](const rclcpp::Waitable::SharedPtr &waitable) {
        waitable->set_on_ready_callback(std::bind(&NoExecutor::handle_waitable, this, callback_group, waitable, _1));
      }
    );
  });
}

void
NoExecutor::remove_node(std::shared_ptr<rclcpp::Node> node_ptr, bool notify)
{
  node_ptr->for_each_callback_group([&](rclcpp::CallbackGroup::SharedPtr callback_group) {
    callback_group->collect_all_ptrs(
      [](const rclcpp::SubscriptionBase::SharedPtr &subscription) {
        subscription->clear_on_new_message_callback();
        subscription->clear_on_new_intra_process_message_callback();
      },
      [](const rclcpp::ServiceBase::SharedPtr &service) {
        service->clear_on_new_request_callback();
      },
      [](const rclcpp::ClientBase::SharedPtr &client) {
        client->clear_on_new_response_callback();
      },
      [](const rclcpp::TimerBase::SharedPtr &timer) {
        UNUSED(timer);
      },
      [](const rclcpp::Waitable::SharedPtr &waitable) {
        waitable->clear_on_ready_callback();
      }
    );
  });
  this->remove_node(node_ptr->get_node_base_interface(), notify);
}

void
NoExecutor::handle_subscription(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::SubscriptionBase::SharedPtr &subscription, size_t num_msgs) {
  if (!started) {
    return;
  }
  while (num_msgs--) {
    Executable executable;
    executable.type = ExecutableType::SUBSCRIPTION;
    executable.callback_group = callback_group;
    executable.subscription = subscription;
    assign_or_create(executable);
  }
}

void
NoExecutor::handle_service(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::ServiceBase::SharedPtr &service, size_t num_msgs) {
  if (!started) {
    return;
  }
  while (num_msgs--) {
    Executable executable;
    executable.type = ExecutableType::SERVICE;
    executable.callback_group = callback_group;
    executable.service = service;
    assign_or_create(executable);
  }
}

void
NoExecutor::handle_client(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::ClientBase::SharedPtr &client, size_t num_msgs) {
  if (!started) {
    return;
  }
  while (num_msgs--) {
    Executable executable;
    executable.type = ExecutableType::CLIENT;
    executable.callback_group = callback_group;
    executable.client = client;
    assign_or_create(executable);
  }
}

void
NoExecutor::handle_waitable(rclcpp::CallbackGroup::SharedPtr callback_group, const rclcpp::Waitable::SharedPtr &waitable, size_t num_msgs) {
  if (!started) {
    return;
  }
  while (num_msgs--) {
    Executable executable;
    executable.type = ExecutableType::WAITABLE;
    executable.callback_group = callback_group;
    executable.waitable = waitable;
    assign_or_create(executable);
  }
}

void
handle_timer(int sig, siginfo_t *si, void *uc) {
  UNUSED(sig);
  UNUSED(uc);
#ifdef RTEMS
  tl_t0 = rtems_uptime_ns_fast();  /* T0: signal handler entry */
  /*
   * RTEMS bug: siginfo_t.si_value is NOT correctly filled for POSIX timers.
   * Workaround: since we can't identify which timer fired via si_value,
   * dispatch to ALL active timers on each signal.
   */
  if (g_active_executor != nullptr && g_active_executor->started) {
    for (auto *t : g_active_executor->timers) {
      if (!t->timer->is_canceled()) {
        Executable executable;
        executable.type = ExecutableType::TIMER;
        executable.callback_group = t->callback_group;
        executable.timer = t->timer;
        t->executor->assign_or_create(executable);
        /* Record dispatch completion time (T3) per timer index */
        if (t->index < G_NE_MAX_TIMERS) {
          g_ne_dispatch_done_ns[t->index] = tl_t3;
          g_ne_dispatch_dur_ns[t->index] = tl_t3 - tl_t0;
          g_ne_assign_dur_ns[t->index] = tl_t3 - tl_t1;
        }
      }
    }
  }
  return;
#else
  PosixTimer *ptimer = static_cast<PosixTimer*>(si->_sifields._rt.si_sigval.sival_ptr);
  if (!ptimer->executor->started) {
    return;
  }
  if (ptimer->timer->is_canceled()) {
    timer_settime(ptimer->timerid, 0, &unset_timer, NULL);
    return;
  }
  Executable executable;
  executable.type = ExecutableType::TIMER;
  executable.callback_group = ptimer->callback_group;
  executable.timer = ptimer->timer;
  ptimer->executor->assign_or_create(executable);
#endif
}


std::shared_ptr<rclcpp::sched::SchedBase> get_sched_base(rclcpp::executors::Executable& executable) {
  switch (executable.type)
  {
  case ExecutableType::SUBSCRIPTION:
    return executable.subscription;
  case ExecutableType::SERVICE:
    return executable.service;
  case ExecutableType::CLIENT:
    return executable.client;
  case ExecutableType::WAITABLE:
    return executable.waitable;
  case ExecutableType::TIMER:
    return executable.timer;
  default:
    return nullptr;
  }
}

void
NoExecutor::assign_or_create(Executable& executable) {
#ifdef RTEMS
  tl_t1 = rtems_uptime_ns_fast();  /* T1: assign_or_create entry */
  /* Find timer index for g_ne_dispatch_done_ns */
  int timer_idx = -1;
  if (executable.type == ExecutableType::TIMER) {
    for (auto *t : this->timers) {
      if (t->timer == executable.timer && t->index < G_NE_MAX_TIMERS) {
        timer_idx = t->index;
        break;
      }
    }
  }
#endif
  auto idle_thread = idle_threads.pop();
  if (idle_thread == nullptr) {
#ifdef RTEMS
    tl_t3 = rtems_uptime_ns_fast();  /* T3 for create_thread path */
    executable.dispatch_done_ns = (int64_t)tl_t3;
    executable.dispatch_dur_ns = (int64_t)(tl_t3 - tl_t0);
    executable.assign_dur_ns = (int64_t)(tl_t3 - tl_t1);
    if (timer_idx >= 0) {
      g_ne_dispatch_done_ns[timer_idx] = tl_t3;
    }
#endif
    create_thread(std::move(executable));
    return;
  }
  auto sched_base = get_sched_base(executable);
	assert(sched_base != nullptr);

  idle_thread->executable = std::move(executable);
  int res = 0;
  if (sched_base->sched_entity.edf_attr) {
    if (sched_base->sched_entity.is_source) {
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);
      sched_base->sched_entity.edf_attr->abs_deadline = (uint64_t) now.tv_sec * SEC_IN_NSEC + now.tv_nsec + sched_base->sched_entity.relative_deadline;
    }
    res = (sched::update_deadline(idle_thread->pthread_id, sched_base->sched_entity.edf_attr) == false);
  } else {
    res = sched::syscall_sched_setattr(idle_thread->pid, &sched_base->sched_attr);
  }
	assert(res == 0);
  idle_thread->is_busy.set_val(1, true);
#ifdef RTEMS
  tl_t3 = rtems_uptime_ns_fast();  /* T3: user dispatch done */
  {
    uint64_t assign_dur = tl_t3 - tl_t1;
    g_ne_assign_ns += assign_dur;
    g_ne_assign_calls++;

    uint64_t dispatch_dur = tl_t3 - tl_t0;
    g_ne_dispatch_ns += dispatch_dur;
    g_ne_dispatch_calls++;
    if (dispatch_dur < g_ne_dispatch_min_ns) g_ne_dispatch_min_ns = dispatch_dur;
    if (dispatch_dur > g_ne_dispatch_max_ns) g_ne_dispatch_max_ns = dispatch_dur;

    if (g_ne_raw_enabled) {
      printf("[E2.2-CSV] %d,%.3f,%.3f\n",
             g_ne_dispatch_calls,
             (double)dispatch_dur / 1000.0,
             (double)assign_dur / 1000.0);
    }

    if (timer_idx >= 0) {
      g_ne_dispatch_done_ns[timer_idx] = tl_t3;
    }
    /* Store per-dispatch timing in idle_thread->executable (already moved) */
    idle_thread->executable.dispatch_done_ns = (int64_t)tl_t3;
    idle_thread->executable.dispatch_dur_ns = (int64_t)dispatch_dur;
    idle_thread->executable.assign_dur_ns = (int64_t)assign_dur;
  }
#endif
}

void
NoExecutor::create_thread(Executable executable) {
  auto sched_base = get_sched_base(executable);
  if (sched_base->sched_entity.edf_attr) {
    if (sched_base->sched_entity.is_source) {
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);
      sched_base->sched_entity.edf_attr->abs_deadline = (uint64_t) now.tv_sec * SEC_IN_NSEC + now.tv_nsec + sched_base->sched_entity.relative_deadline;
    }
    std::thread new_thread(std::bind(&NoExecutor::thread_start, this, std::move(executable)));
    sched::update_deadline(new_thread.native_handle(), sched_base->sched_entity.edf_attr);
    new_thread.detach();
  } else {
    std::thread new_thread(std::bind(&NoExecutor::thread_start, this, std::move(executable)));
    sched::syscall_sched_setattr(sched::get_pid(new_thread.native_handle()), &sched_base->sched_attr);
    new_thread.detach();
  }
}

void
NoExecutor::thread_start(Executable executable) {
  rclcpp::executors::ThreadDataNoExec thread_data;
  thread_data.executable = executable;
  thread_data.is_busy.set_val(1, false);
  pthread_t self = pthread_self();
  thread_data.pthread_id = self;
#ifdef RTEMS
  thread_data.pid = (pid_t)pthread_self();
#else
  thread_data.pid = sched::get_pid(self);
#endif
  while (true) {
    thread_data.is_busy.wait_on(0);
#ifdef RTEMS
    tl_t4 = rtems_uptime_ns_fast();  /* T4: worker thread woke up */
    {
      uint64_t wakeup_dur = tl_t4 - tl_t3;
      g_ne_wakeup_ns += wakeup_dur;
      g_ne_wakeup_calls++;
    }
#endif
    this->execute_executable(thread_data.executable);
    thread_data.is_busy.set_val(0, false);
    this->idle_threads.push(&thread_data);
  }
}

void
NoExecutor::spin() {
  start();
  while (rclcpp::ok(this->context_)) {
    sleep(600);
  }
  stop();
}
