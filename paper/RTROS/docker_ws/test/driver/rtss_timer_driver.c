/**
 * @file rtss_timer_driver.c
 *
 * RTEMS RTSS Timer Device Driver — Implementation
 *
 * =====================================================================
 *  Architecture / 架构  (v4 — PT hardware timer, single-shot re-arm)
 * =====================================================================
 *
 *   ┌───────────────────────────────────────────────────────────┐
 *   │  Cortex-A9 Private Timer (PT) @ 0x1f000600, IRQ 29       │
 *   │  100 MHz peripheral clock → 10 ns resolution              │
 *   │  Independent of system clock tick (GT, IRQ 27)            │
 *   └─────────────────────────┬─────────────────────────────────┘
 *                             │  hardware interrupt
 *                             ▼
 *   ┌───────────────────────────────────────────────────────────┐
 *   │  pt_isr() (ISR context)                                   │
 *   │                                                            │
 *   │  1. Clear PT interrupt flag                               │
 *   │  2. For each active channel:                              │
 *   │       if next_fire_ns <= now:                             │
 *   │         rtss_isr_fire_ns[ch] = now                        │
 *   │         rtems_event_send(ch->target_task, EVENT_1)        │
 *   │         advance next_fire_ns by period_ns (skip missed)   │
 *   │  3. Find nearest next_fire across all channels            │
 *   │  4. Re-arm PT Load register (one-shot)                    │
 *   │     — or disable PT if no channels active                 │
 *   │                                                            │
 *   │  ★ Runs in ISR context — NOT preemptable by user tasks    │
 *   │  ★ Absolute re-arm prevents cumulative drift              │
 *   └─────────────────────────┬─────────────────────────────────┘
 *                             │
 *                             ▼
 *   ┌───────────────────────────────────────────────────────────┐
 *   │  CallbackTask (user-specified priority)                    │
 *   │    rtems_event_receive(RTEMS_EVENT_1) → callback()        │
 *   └───────────────────────────────────────────────────────────┘
 *
 *   Why single-shot re-arm with absolute time?
 *   ──────────────────────────────────────────
 *   The PT has only one countdown register.  To multiplex N channels
 *   with different periods, we:
 *     - Arm the PT for the nearest upcoming expiration
 *     - In the ISR, dispatch all expired channels
 *     - Re-arm for the next nearest expiration
 *
 *   Absolute time (next_fire_ns += period_ns) prevents cumulative
 *   drift: even if an ISR fires late, the next target is computed
 *   from the original schedule, not from "now + period".
 */

#include <rtems.h>
#include <rtems/libio.h>
#include <bsp.h>
#include <bsp/arm-a9mpcore-regs.h>
#include <bsp/arm-a9mpcore-irq.h>
#include <rtems/irq-extension.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <limits.h>

#include "rtss_timer_driver.h"

/* ----------------------------------------------------------------
 *  Hardware Abstraction
 * ---------------------------------------------------------------- */

#define PT ((volatile a9mpcore_pt *) BSP_ARM_A9MPCORE_PT_BASE)

/* Peripheral clock frequency (100 MHz on RealView PBX-A9) */
#define PT_PERIPHCLK  BSP_ARM_A9MPCORE_PERIPHCLK

/* ----------------------------------------------------------------
 *  Internal Channel State
 * ---------------------------------------------------------------- */

typedef struct {
    bool            active;
    int64_t         period_ns;     /* Configured period in nanoseconds */
    int64_t         next_fire_ns;  /* Absolute time of next expiration */
    rtems_id        target_task;   /* Direct Task Notification target */
} rtss_channel_state;

/* ----------------------------------------------------------------
 *  Driver Instance State
 * ---------------------------------------------------------------- */

static struct {
    bool                 initialized;
    bool                 opened;
    bool                 isr_installed;
    rtss_channel_state   channels[RTSS_TIMER_MAX_CHANNELS];
} drv;

/* ----------------------------------------------------------------
 *  Per-Channel ISR fire timestamp (for sched_delay measurement)
 * ---------------------------------------------------------------- */

volatile int64_t rtss_isr_fire_ns[RTSS_TIMER_MAX_CHANNELS];

/* ----------------------------------------------------------------
 *  Time Utilities
 * ---------------------------------------------------------------- */

static int64_t now_ns(void)
{
    struct timespec ts;
    rtems_clock_get_uptime(&ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

static uint32_t ns_to_pt_load(int64_t ns)
{
    int64_t load = ns * (int64_t)PT_PERIPHCLK / 1000000000LL;
    if (load < 1)          load = 1;
    if (load > 0xFFFFFFFF) load = 0xFFFFFFFF;
    return (uint32_t)load;
}

/* ----------------------------------------------------------------
 *  PT Hardware Control
 * ---------------------------------------------------------------- */

static void pt_disable(void)
{
    PT->ctrl = 0;
}

static void pt_arm_delta(int64_t delta_ns)
{
    uint32_t load = ns_to_pt_load(delta_ns);

    PT->ctrl  = 0;
    PT->load  = load;
    PT->irqst = A9MPCORE_PT_IRQST_EFLG;
    PT->ctrl  = A9MPCORE_PT_CTRL_IRQ_EN | A9MPCORE_PT_CTRL_TMR_EN;
}

static void pt_arm_absolute(int64_t target_ns)
{
    int64_t delta = target_ns - now_ns();
    if (delta < 1) delta = 1;
    pt_arm_delta(delta);
}

/* ----------------------------------------------------------------
 *  Channel Multiplexing
 * ---------------------------------------------------------------- */

static int find_nearest_expiration(int64_t *nearest_ns)
{
    int nearest_ch = -1;
    int64_t nearest = INT64_MAX;

    for (int i = 0; i < RTSS_TIMER_MAX_CHANNELS; i++) {
        rtss_channel_state *ch = &drv.channels[i];
        if (ch->active && ch->next_fire_ns < nearest) {
            nearest = ch->next_fire_ns;
            nearest_ch = i;
        }
    }

    if (nearest_ch >= 0)
        *nearest_ns = nearest;
    return nearest_ch;
}

static void rearm_pt(void)
{
    int64_t nearest;
    int ch = find_nearest_expiration(&nearest);

    if (ch < 0) {
        pt_disable();
        return;
    }

    pt_arm_absolute(nearest);
}

/* ----------------------------------------------------------------
 *  PT Interrupt Service Routine
 * ---------------------------------------------------------------- */

static void pt_isr(void *arg)
{
    (void)arg;

    PT->irqst = A9MPCORE_PT_IRQST_EFLG;

    int64_t now = now_ns();

    for (int i = 0; i < RTSS_TIMER_MAX_CHANNELS; i++) {
        rtss_channel_state *ch = &drv.channels[i];

        if (!ch->active || ch->target_task == RTEMS_ID_NONE)
            continue;

        if (ch->next_fire_ns <= now) {
            rtss_isr_fire_ns[i] = now;
            rtems_event_send(ch->target_task, RTEMS_EVENT_1);

            while (ch->next_fire_ns <= now)
                ch->next_fire_ns += ch->period_ns;
        }
    }

    rearm_pt();
}

/* ----------------------------------------------------------------
 *  ISR Installation
 * ---------------------------------------------------------------- */

static rtems_status_code install_pt_isr(void)
{
    if (drv.isr_installed)
        return RTEMS_SUCCESSFUL;

    rtems_status_code sc = rtems_interrupt_handler_install(
        A9MPCORE_IRQ_PT,
        "PT",
        RTEMS_INTERRUPT_UNIQUE,
        pt_isr,
        NULL
    );

    if (sc == RTEMS_SUCCESSFUL)
        drv.isr_installed = true;

    return sc;
}

/* ----------------------------------------------------------------
 *  Driver Operations
 * ---------------------------------------------------------------- */

static rtems_status_code rtss_init(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg)
{
    (void)major; (void)minor; (void)arg;

    memset(&drv, 0, sizeof(drv));
    drv.initialized = true;

    for (int i = 0; i < RTSS_TIMER_MAX_CHANNELS; i++) {
        drv.channels[i].target_task  = RTEMS_ID_NONE;
        drv.channels[i].next_fire_ns = INT64_MAX;
    }

    return RTEMS_SUCCESSFUL;
}

static rtems_status_code rtss_open(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg)
{
    (void)major; (void)minor; (void)arg;

    if (drv.opened)
        return RTEMS_TOO_MANY;

    rtems_status_code sc = install_pt_isr();
    if (sc != RTEMS_SUCCESSFUL)
        return sc;

    drv.opened = true;
    return RTEMS_SUCCESSFUL;
}

static rtems_status_code rtss_close(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg)
{
    (void)major; (void)minor; (void)arg;

    for (int i = 0; i < RTSS_TIMER_MAX_CHANNELS; i++) {
        rtss_channel_state *ch = &drv.channels[i];
        ch->active       = false;
        ch->target_task  = RTEMS_ID_NONE;
        ch->next_fire_ns = INT64_MAX;
    }

    pt_disable();
    drv.opened = false;
    return RTEMS_SUCCESSFUL;
}

static rtems_status_code rtss_read(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg)
{
    (void)major; (void)minor; (void)arg;
    return RTEMS_NOT_DEFINED;
}

static rtems_status_code rtss_write(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg)
{
    (void)major; (void)minor; (void)arg;
    return RTEMS_SUCCESSFUL;
}

/**
 * @brief IOCTL control operations
 *
 * Commands:
 *   RTSS_TIMER_SET_MODE     — Ignored (legacy)
 *   RTSS_TIMER_SET_PERIOD   — Set channel period (RTEMS ticks → ns)
 *   RTSS_TIMER_SET_PERIOD_NS— Set channel period (nanoseconds)
 *   RTSS_TIMER_START        — Activate a channel
 *   RTSS_TIMER_STOP         — Deactivate a channel
 *   RTSS_TIMER_SET_TASK     — Set target task for Direct Task Notification
 */
static rtems_status_code rtss_control(
    rtems_device_major_number major,
    rtems_device_minor_number minor,
    void *arg)
{
    (void)major; (void)minor;

    rtems_libio_ioctl_args_t *ctl = arg;

    switch (ctl->command) {

    case RTSS_TIMER_SET_MODE: {
        ctl->ioctl_return = 0;
        return RTEMS_SUCCESSFUL;
    }

    case RTSS_TIMER_SET_PERIOD: {
        rtss_timer_config *cfg = (rtss_timer_config *)ctl->buffer;
        if (!cfg)
            return RTEMS_INVALID_ADDRESS;
        if (cfg->channel < 0 || cfg->channel >= RTSS_TIMER_MAX_CHANNELS)
            return RTEMS_INVALID_NUMBER;

        rtems_interval tps = rtems_clock_get_ticks_per_second();
        if (tps == 0)
            return RTEMS_NOT_DEFINED;

        drv.channels[cfg->channel].period_ns =
            (int64_t)cfg->period_ticks * 1000000000LL / tps;

        ctl->ioctl_return = 0;
        return RTEMS_SUCCESSFUL;
    }

    case RTSS_TIMER_SET_PERIOD_NS: {
        rtss_timer_ns_config *cfg = (rtss_timer_ns_config *)ctl->buffer;
        if (!cfg)
            return RTEMS_INVALID_ADDRESS;
        if (cfg->channel < 0 || cfg->channel >= RTSS_TIMER_MAX_CHANNELS)
            return RTEMS_INVALID_NUMBER;
        if (cfg->period_ns < RTSS_TIMER_MIN_PERIOD_NS)
            return RTEMS_INVALID_NUMBER;
        if (cfg->period_ns > RTSS_TIMER_MAX_PERIOD_NS)
            return RTEMS_INVALID_NUMBER;

        drv.channels[cfg->channel].period_ns = cfg->period_ns;
        ctl->ioctl_return = 0;
        return RTEMS_SUCCESSFUL;
    }

    case RTSS_TIMER_START: {
        rtss_timer_config *cfg = (rtss_timer_config *)ctl->buffer;
        if (!cfg)
            return RTEMS_INVALID_ADDRESS;
        if (cfg->channel < 0 || cfg->channel >= RTSS_TIMER_MAX_CHANNELS)
            return RTEMS_INVALID_NUMBER;

        rtss_channel_state *ch = &drv.channels[cfg->channel];

        if (ch->period_ns == 0)
            return RTEMS_NOT_DEFINED;

        rtems_interrupt_level level;
        rtems_interrupt_disable(level);

        ch->active = true;
        ch->next_fire_ns = now_ns() + ch->period_ns;
        rearm_pt();

        rtems_interrupt_enable(level);

        ctl->ioctl_return = 0;
        return RTEMS_SUCCESSFUL;
    }

    case RTSS_TIMER_STOP: {
        rtss_timer_config *cfg = (rtss_timer_config *)ctl->buffer;
        if (!cfg)
            return RTEMS_INVALID_ADDRESS;
        if (cfg->channel < 0 || cfg->channel >= RTSS_TIMER_MAX_CHANNELS)
            return RTEMS_INVALID_NUMBER;

        rtss_channel_state *ch = &drv.channels[cfg->channel];

        rtems_interrupt_level level;
        rtems_interrupt_disable(level);

        ch->active       = false;
        ch->next_fire_ns = INT64_MAX;
        rearm_pt();

        rtems_interrupt_enable(level);

        ctl->ioctl_return = 0;
        return RTEMS_SUCCESSFUL;
    }

    case RTSS_TIMER_SET_TASK: {
        rtss_timer_task_config *cfg = (rtss_timer_task_config *)ctl->buffer;
        if (!cfg)
            return RTEMS_INVALID_ADDRESS;
        if (cfg->channel < 0 || cfg->channel >= RTSS_TIMER_MAX_CHANNELS)
            return RTEMS_INVALID_NUMBER;

        drv.channels[cfg->channel].target_task = cfg->target_task;
        ctl->ioctl_return = 0;
        return RTEMS_SUCCESSFUL;
    }

    default:
        ctl->ioctl_return = -1;
        return RTEMS_NOT_DEFINED;
    }
}

/* ----------------------------------------------------------------
 *  Driver Table Entry
 * ---------------------------------------------------------------- */

rtems_driver_address_table rtss_timer_driver_table = {
    .initialization_entry = rtss_init,
    .open_entry           = rtss_open,
    .close_entry          = rtss_close,
    .read_entry           = rtss_read,
    .write_entry          = rtss_write,
    .control_entry        = rtss_control
};
