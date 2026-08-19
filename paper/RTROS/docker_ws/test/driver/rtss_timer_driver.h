/**
 * @file rtss_timer_driver.h
 *
 * RTEMS RTSS Timer Device Driver — Public API
 *
 * =====================================================================
 *  Overview
 * =====================================================================
 *
 * Provides periodic hardware timer channels via /dev/rtss_timer.
 * Each channel can directly notify a specific RTEMS task via
 * rtems_event_send when the channel expires — no intermediary
 * Reader/Worker tasks needed.
 *
 * =====================================================================
 *  Hardware
 * =====================================================================
 *
 * Uses the Cortex-A9 Private Timer (PT) at 0x1f000600 (IRQ 29).
 * The PT runs at the peripheral clock frequency (100 MHz on
 * RealView PBX-A9), providing 10 ns resolution.
 *
 * The BSP's system clock tick uses the Global Timer (GT, IRQ 27).
 * The PT is completely independent — no shared interrupt, no
 * dependency on CONFIGURE_MICROSECONDS_PER_TICK.
 *
 * =====================================================================
 *  Channel Multiplexing
 * =====================================================================
 *
 * A single PT hardware timer is multiplexed across up to
 * RTSS_TIMER_MAX_CHANNELS channels using single-shot re-arm:
 *
 *   1. PT fires (one-shot mode, AUTO_RLD=0)
 *   2. ISR checks all channels for expiration
 *   3. For each expired channel: rtems_event_send(target_task)
 *   4. ISR computes the nearest next-fire time across all channels
 *   5. ISR re-arms PT with the delta to that next-fire time
 *   6. If no channels are active, PT is disabled
 *
 * Absolute time calculation prevents cumulative drift:
 *   next_fire = next_fire + period_ns  (not "now + period_ns")
 *
 * =====================================================================
 *  Direct Task Notification Path
 * =====================================================================
 *
 *   PT hardware interrupt (IRQ 29)
 *       |
 *       v
 *   pt_isr() (ISR context)
 *       |  rtems_event_send(ch->target_task, RTEMS_EVENT_1)
 *       v
 *   CallbackTask (user priority)
 *       |  rtems_event_receive() returns
 *       |  callback_() executes
 *
 *   ISR context cannot be preempted by any user-level task,
 *   eliminating priority inversion entirely.
 *
 * =====================================================================
 *  Driver Usage Flow
 * =====================================================================
 *
 *   1. open("/dev/rtss_timer", O_RDWR)
 *   2. ioctl(fd, RTSS_TIMER_SET_MODE, &mode_cfg)     — Set EVENT mode
 *   3. ioctl(fd, RTSS_TIMER_SET_PERIOD, &period_cfg)  — Set channel period (ticks)
 *      OR ioctl(fd, RTSS_TIMER_SET_PERIOD_NS, &ns_cfg)— Set channel period (nanoseconds)
 *   4. ioctl(fd, RTSS_TIMER_SET_TASK, &task_cfg)      — Set target task
 *   5. ioctl(fd, RTSS_TIMER_START, &start_cfg)        — Start the channel
 *   6. [Driver directly sends events to target_task on expiration]
 *   7. ioctl(fd, RTSS_TIMER_STOP, &stop_cfg)          — Stop the channel
 *   8. close(fd)
 *
 * =====================================================================
 *  Integration with RTExecutor CallbackTask
 * =====================================================================
 *
 *   register_rt_timer() → CallbackTask → setup_timer_driver():
 *     1. Create CallbackTask (RTEMS task at user-specified priority)
 *     2. Start CallbackTask (it blocks on rtems_event_receive)
 *     3. RTSS_TIMER_SET_PERIOD(channel, period)  or  SET_PERIOD_NS
 *     4. RTSS_TIMER_SET_TASK(channel, callback_task->task_id())
 *     5. RTSS_TIMER_START(channel)
 *
 *   Now the driver directly wakes the CallbackTask on each fire:
 *     PT IRQ 29 → pt_isr() → rtems_event_send(CBTask) → callback()
 *
 *   The ISR callback is NOT preemptable by any user task.
 */

#ifndef RTSS_TIMER_DRIVER_H
#define RTSS_TIMER_DRIVER_H

#include <rtems.h>
#include <sys/ioccom.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Device node path */
#define RTSS_DEVICE_NAME      "/dev/rtss_timer"

/** Maximum number of independent timer channels */
#define RTSS_TIMER_MAX_CHANNELS 8

/**
 * Minimum useful period in nanoseconds (~1 us at 100 MHz, Load=100).
 * Periods below this are rejected to prevent ISR-storm hangs.
 */
#define RTSS_TIMER_MIN_PERIOD_NS   1000

/**
 * Maximum single-shot period in nanoseconds (~42.9 s at 100 MHz).
 * Limited by the 32-bit PT Load register: UINT32_MAX * (10 ns/tick).
 */
#define RTSS_TIMER_MAX_PERIOD_NS   ((int64_t)4294967295LL * 10)

/* ----------------------------------------------------------------
 *  Trigger Modes (for legacy global notification)
 * ---------------------------------------------------------------- */
typedef enum {
    RTSS_MODE_EVENT            = 0,
    RTSS_MODE_BINARY_SEMAPHORE = 1,
    RTSS_MODE_MESSAGE_QUEUE    = 2
} rtss_trigger_mode;

/* ----------------------------------------------------------------
 *  IOCTL Commands
 * ---------------------------------------------------------------- */

#define RTSS_TIMER_IOC_MAGIC  'R'

/** Set the driver's trigger mode (for legacy global notification) */
#define RTSS_TIMER_SET_MODE    _IOW(RTSS_TIMER_IOC_MAGIC, 1, rtss_timer_mode_config)

/** Set the period for a specific channel (in RTEMS ticks) */
#define RTSS_TIMER_SET_PERIOD  _IOW(RTSS_TIMER_IOC_MAGIC, 2, rtss_timer_config)

/** Activate a timer channel */
#define RTSS_TIMER_START       _IOW(RTSS_TIMER_IOC_MAGIC, 3, rtss_timer_config)

/** Deactivate a timer channel */
#define RTSS_TIMER_STOP        _IOW(RTSS_TIMER_IOC_MAGIC, 4, rtss_timer_config)

/**
 * Set the target RTEMS task for a channel (Direct Task Notification).
 *
 * When set, the ISR sends rtems_event_send(target_task, EVENT_1)
 * directly when this channel expires.
 */
#define RTSS_TIMER_SET_TASK    _IOW(RTSS_TIMER_IOC_MAGIC, 5, rtss_timer_task_config)

/**
 * Set the period for a specific channel (nanosecond precision).
 *
 * Uses the PT hardware timer directly — independent of the system
 * clock tick.  period_ns must be >= RTSS_TIMER_MIN_PERIOD_NS and
 * <= RTSS_TIMER_MAX_PERIOD_NS.
 */
#define RTSS_TIMER_SET_PERIOD_NS  _IOW(RTSS_TIMER_IOC_MAGIC, 6, rtss_timer_ns_config)

/* ----------------------------------------------------------------
 *  Configuration Structures
 * ---------------------------------------------------------------- */

/** Trigger mode configuration (for legacy global notification) */
typedef struct {
    rtss_trigger_mode mode;
} rtss_timer_mode_config;

/** Channel period configuration (RTEMS ticks) */
typedef struct {
    int             channel;
    rtems_interval  period_ticks;
} rtss_timer_config;

/** Channel period configuration (nanoseconds) */
typedef struct {
    int     channel;
    int64_t period_ns;
} rtss_timer_ns_config;

/**
 * Direct Task Notification configuration.
 *
 * When target_task is set to a valid RTEMS task ID, the ISR
 * will call rtems_event_send(target_task, RTEMS_EVENT_1)
 * directly when this channel expires.
 */
typedef struct {
    int         channel;
    rtems_id    target_task;
} rtss_timer_task_config;

/** Event structure returned by read() (legacy mode only) */
typedef struct {
    int channel;
} rtss_timer_event;

/** Driver table entry for RTEMS configuration */
extern rtems_driver_address_table rtss_timer_driver_table;

/**
 * Per-channel ISR fire timestamp (nanoseconds since boot).
 *
 * Updated by the ISR on each timer fire.  The CallbackTask
 * reads this to compute the actual scheduling delay:
 *   sched_delay_timer = callback_start_time - rtss_isr_fire_ns[ch]
 */
extern volatile int64_t rtss_isr_fire_ns[RTSS_TIMER_MAX_CHANNELS];

#ifdef __cplusplus
}
#endif

#endif /* RTSS_TIMER_DRIVER_H */
