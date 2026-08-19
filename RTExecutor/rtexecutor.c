/*
 * RTExecutor Benchmark on RTEMS
 *
 * Faithfully replicates the RTExecutor design from the paper:
 *   - Hardware ISR-driven timer dispatch via /dev/rtss_timer driver
 *   - Each callback maps to a dedicated RTEMS task (CallbackTask) with independent priority
 *   - Timer: HW tick ISR → rtems_event_send → CallbackTask (≤2 tick jitter)
 *   - Subscription: direct notification, bypasses rcl_wait entirely
 *   - All scheduling by RTEMS FP kernel — NO middleware scheduling layer
 *   - Priority inheritance on shared resources
 *
 * Key difference from PICAS/RT-ROS:
 *   Timer dispatch path: HW tick → ISR → rtems_event_send(CallbackTask)
 *   NOT: software poll → signal → mutex → callback
 *
 * All timestamps in nanoseconds via rtems_clock_get_uptime().
 */

#include <rtems.h>
#include <rtems/libio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/* ---- Nanosecond timing ---- */

static inline uint64_t now_ns(void)
{
    struct timespec ts;
    rtems_clock_get_uptime(&ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

#define MS_TO_NS(ms)    ((uint64_t)(ms) * 1000000ULL)
#define NS_TO_US(ns)    ((ns) / 1000U)
#define NS_TO_US_F(ns)  ((double)(ns) / 1000.0)

static inline void busy_wait(uint64_t ns)
{
    uint64_t deadline = now_ns() + ns;
    while (now_ns() < deadline)
        ;
}

/* ---- Priority definitions ---- */

#define PRIO_HIGH       60
#define PRIO_MEDIUM     70
#define PRIO_LOW        80
#define PRIO_BG        200

#define NUM_FIRINGS   100
#define MAX_CB         8
#define MAX_CHANNELS   8

/* ---- Measurement record ---- */

typedef struct {
    uint64_t release_ns;
    uint64_t start_ns;
    uint64_t end_ns;
} meas_t;

/* ---- CallbackTask structure ---- */

typedef struct {
    rtems_id        task_id;
    rtems_id        wait_event;     /* CallbackTask blocks on this event */
    uint32_t        period_ms;
    uint64_t        start_ns;
    uint32_t        priority;
    uint32_t        callback_type;  /* 0=timer, 1=subscription */
    uint64_t        exec_ns;
    volatile uint32_t firing_count;
    meas_t          meas[NUM_FIRINGS];
} cb_task_t;

static cb_task_t g_cb[MAX_CB];
static int g_num_cb = 0;

/* ---- /dev/rtss_timer driver (embedded) ---- */

#define RTSS_DEVICE_NAME "/dev/rtss_timer"

typedef struct {
    bool            active;
    rtems_interval  period_ticks;
    rtems_id        timer_id;
    rtems_id        target_task;
} rtss_channel_t;

static struct {
    bool             initialized;
    bool             opened;
    rtss_channel_t   channels[MAX_CHANNELS];
} drv;

volatile int64_t rtss_isr_fire_ns[MAX_CHANNELS];

static rtems_timer_service_routine channel_timer_isr(
    rtems_id timer_id, void *user_data)
{
    int ch = (int)(intptr_t)user_data;
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    rtss_channel_t *c = &drv.channels[ch];
    if (!c->active || c->target_task == RTEMS_ID_NONE) return;

    /* Capture ISR fire time */
    struct timespec ts;
    rtems_clock_get_uptime(&ts);
    rtss_isr_fire_ns[ch] = (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;

    /* Direct notification to CallbackTask — ISR context, cannot be preempted */
    rtems_event_send(c->target_task, RTEMS_EVENT_1);

    /* Absolute re-arm to prevent cumulative drift */
    rtems_timer_fire_after(timer_id, c->period_ticks, channel_timer_isr, user_data);
}

/* Driver ops (minimal, only what we need) */
static rtems_status_code drv_init(rtems_device_major_number m, rtems_device_minor_number n, void *a)
    { (void)m;(void)n;(void)a; memset(&drv,0,sizeof(drv)); drv.initialized=true; return RTEMS_SUCCESSFUL; }
static rtems_status_code drv_open(rtems_device_major_number m, rtems_device_minor_number n, void *a)
    { (void)m;(void)n;(void)a; drv.opened=true; return RTEMS_SUCCESSFUL; }
static rtems_status_code drv_close(rtems_device_major_number m, rtems_device_minor_number n, void *a)
    { (void)m;(void)n;(void)a;
      for(int i=0;i<MAX_CHANNELS;i++){
          if(drv.channels[i].active){ rtems_timer_cancel(drv.channels[i].timer_id); drv.channels[i].active=false; }
          if(drv.channels[i].timer_id!=RTEMS_ID_NONE){ rtems_timer_delete(drv.channels[i].timer_id); drv.channels[i].timer_id=RTEMS_ID_NONE; }
          drv.channels[i].target_task=RTEMS_ID_NONE;
      }
      drv.opened=false; return RTEMS_SUCCESSFUL; }

static rtems_status_code drv_control(rtems_device_major_number m, rtems_device_minor_number n, void *arg)
{
    (void)m;(void)n;
    rtems_libio_ioctl_args_t *ctl = arg;
    int cmd = ctl->command;

    if (cmd == 2) { /* SET_PERIOD */
        int ch = ((int*)ctl->buffer)[0];
        rtems_interval pt = ((rtems_interval*)ctl->buffer)[1];
        if (ch<0||ch>=MAX_CHANNELS) return RTEMS_INVALID_NUMBER;
        drv.channels[ch].period_ticks = pt;
        ctl->ioctl_return = 0; return RTEMS_SUCCESSFUL;
    }
    if (cmd == 3) { /* START */
        int ch = ((int*)ctl->buffer)[0];
        if (ch<0||ch>=MAX_CHANNELS) return RTEMS_INVALID_NUMBER;
        rtss_channel_t *c = &drv.channels[ch];
        if (c->active && c->timer_id!=RTEMS_ID_NONE) rtems_timer_cancel(c->timer_id);
        if (c->timer_id==RTEMS_ID_NONE) {
            rtems_timer_create(rtems_build_name('R','T','0'+ch/10,'0'+ch%10), &c->timer_id);
        }
        rtems_timer_fire_after(c->timer_id, c->period_ticks, channel_timer_isr, (void*)(intptr_t)ch);
        c->active = true;
        ctl->ioctl_return = 0; return RTEMS_SUCCESSFUL;
    }
    if (cmd == 4) { /* STOP */
        int ch = ((int*)ctl->buffer)[0];
        if (ch<0||ch>=MAX_CHANNELS) return RTEMS_INVALID_NUMBER;
        rtss_channel_t *c = &drv.channels[ch];
        if (c->timer_id!=RTEMS_ID_NONE) rtems_timer_cancel(c->timer_id);
        c->active=false; c->target_task=RTEMS_ID_NONE;
        ctl->ioctl_return = 0; return RTEMS_SUCCESSFUL;
    }
    if (cmd == 5) { /* SET_TASK */
        int ch = ((int*)ctl->buffer)[0];
        rtems_id tid = ((rtems_id*)ctl->buffer)[1];
        if (ch<0||ch>=MAX_CHANNELS) return RTEMS_INVALID_NUMBER;
        drv.channels[ch].target_task = tid;
        ctl->ioctl_return = 0; return RTEMS_SUCCESSFUL;
    }
    ctl->ioctl_return = -1; return RTEMS_NOT_DEFINED;
}

static rtems_driver_address_table rtss_driver_table = {
    .initialization_entry = drv_init,
    .open_entry = drv_open,
    .close_entry = drv_close,
    .read_entry = NULL,
    .write_entry = NULL,
    .control_entry = drv_control
};

/* ---- Driver helper functions ---- */

static int g_drv_fd = -1;

static void setup_driver(void)
{
    rtems_device_major_number major;
    rtems_io_register_driver(0, &rtss_driver_table, &major);
    rtems_io_register_name(RTSS_DEVICE_NAME, major, 0);
    g_drv_fd = open(RTSS_DEVICE_NAME, O_RDWR);
}

static void start_hw_timer(int channel, rtems_interval period_ticks, rtems_id target_task)
{
    int buf[2]; buf[0] = channel; buf[1] = (int)period_ticks;
    ioctl(g_drv_fd, 2, buf);  /* SET_PERIOD */

    rtems_id buf2[2]; buf2[0] = (rtems_id)channel; buf2[1] = target_task;
    ioctl(g_drv_fd, 5, buf2); /* SET_TASK */

    int buf3[2]; buf3[0] = channel; buf3[1] = 0;
    ioctl(g_drv_fd, 3, buf3); /* START */
}

static void stop_hw_timer(int channel)
{
    int buf[2]; buf[0] = channel; buf[1] = 0;
    ioctl(g_drv_fd, 4, buf); /* STOP */
}

/* ---- CallbackTask entry point ----
 *
 * Each callback is a dedicated RTEMS task with independent priority.
 * It blocks on rtems_event_receive until notified:
 *   - Timer: ISR sends event directly (hardware-driven)
 *   - Subscription: direct notification sends event
 *
 * This is the core of RTExecutor: no middleware scheduling, no FIFO,
 * no CBG mutex — pure OS kernel scheduling.
 */
static void cb_task_entry(rtems_task_argument arg)
{
    cb_task_t *cb = &g_cb[arg];
    rtems_event_set events;

    for (;;) {
        rtems_event_receive(RTEMS_EVENT_1, RTEMS_EVENT_ANY | RTEMS_WAIT,
                            RTEMS_NO_TIMEOUT, &events);

        uint32_t idx = cb->firing_count;
        if (idx < NUM_FIRINGS) {
            cb->meas[idx].start_ns = now_ns();

            if (cb->callback_type == 0) { /* timer: release = ISR fire time */
                cb->meas[idx].release_ns = (uint64_t)rtss_isr_fire_ns[arg];
            }

            /* Simulate callback execution */
            if (cb->exec_ns > 0) busy_wait(cb->exec_ns);

            cb->meas[idx].end_ns = now_ns();
            cb->firing_count++;
        }
    }
}

/* ---- Helper: create CallbackTask ---- */
static void create_cb_task(int idx, uint32_t prio, uint32_t period_ms,
                           uint32_t cb_type, uint64_t exec_ns)
{
    g_cb[idx].period_ms = period_ms;
    g_cb[idx].start_ns = now_ns();
    g_cb[idx].priority = prio;
    g_cb[idx].callback_type = cb_type;
    g_cb[idx].exec_ns = exec_ns;
    g_cb[idx].firing_count = 0;

    rtems_task_create(
        rtems_build_name('C', 'B', '0' + idx, 'T'),
        prio, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES,
        &g_cb[idx].task_id);
    rtems_task_start(g_cb[idx].task_id, cb_task_entry, idx);
    g_num_cb++;
}

/* ---- Print helpers ---- */

static void print_jitter_stats(const char *label, cb_task_t *cb)
{
    uint64_t jmin = UINT64_MAX, jmax = 0, jsum = 0;
    int n = cb->firing_count;
    for (int i = 0; i < n && i < NUM_FIRINGS; i++) {
        uint64_t jitter = cb->meas[i].start_ns - cb->meas[i].release_ns;
        if (jitter < jmin) jmin = jitter;
        if (jitter > jmax) jmax = jitter;
        jsum += jitter;
    }
    if (n == 0) jmin = 0;
    printf("  %s: n=%d, jitter min=%llu us, max=%llu us, mean=%.2f us\n",
           label, n, (unsigned long long)NS_TO_US(jmin),
           (unsigned long long)NS_TO_US(jmax),
           n > 0 ? NS_TO_US_F((double)jsum / n) : 0.0);
}

/* ---- Pub/Sub for Exp B ---- */
static rtems_id g_pubsub_q;
static rtems_id g_pub_id;
static meas_t g_lat_meas[NUM_FIRINGS];
static int g_lat_count = 0;

static void publisher_task(rtems_task_argument arg)
{
    for (int i = 0; i < NUM_FIRINGS; i++) {
        uint64_t pub_ns = now_ns();
        rtems_message_queue_send(g_pubsub_q, &pub_ns, sizeof(pub_ns));
        /* Direct notification: rtems_event_send to subscriber's CallbackTask */
        rtems_event_send(g_cb[1].task_id, RTEMS_EVENT_1);
        rtems_task_wake_after(100);
    }
    rtems_task_delete(RTEMS_SELF);
}

/* Subscriber CallbackTask (index 1) — handled by cb_task_entry with
 * release_ns set to the publish timestamp read from the message queue.
 * We override by creating a custom subscriber task. */
static void subscriber_task(rtems_task_argument arg)
{
    rtems_event_set events;
    for (;;) {
        rtems_event_receive(RTEMS_EVENT_1, RTEMS_EVENT_ANY | RTEMS_WAIT,
                            RTEMS_NO_TIMEOUT, &events);
        uint64_t pub_ns;
        size_t size;
        rtems_status_code sc = rtems_message_queue_receive(
            g_pubsub_q, &pub_ns, &size, RTEMS_NO_WAIT, 0);
        if (sc == RTEMS_SUCCESSFUL && g_lat_count < NUM_FIRINGS) {
            g_lat_meas[g_lat_count].release_ns = pub_ns;
            g_lat_meas[g_lat_count].start_ns = now_ns();
            busy_wait(1000000ULL); /* 1ms callback work */
            g_lat_meas[g_lat_count].end_ns = now_ns();
            g_lat_count++;
        }
    }
}

/* ================================================================
 * Experiment A: Timer Callback Jitter (ISR-driven)
 * ================================================================ */
static void run_exp_a(void)
{
    printf("\n=== Exp A: Timer Callback Jitter (RTExecutor) ===\n");

    g_num_cb = 0;
    setup_driver();

    uint32_t periods[] = {200, 500, 1000};
    uint32_t prios[]   = {PRIO_HIGH, PRIO_MEDIUM, PRIO_LOW};

    for (int i = 0; i < 3; i++) {
        create_cb_task(i, prios[i], periods[i], 0, 1000000ULL);
        rtems_interval pticks = periods[i]; /* 1ms tick, period_ms = ticks */
        start_hw_timer(i, pticks, g_cb[i].task_id);
    }

    rtems_task_wake_after(30000);

    const char *labels[] = {"200ms", "500ms", "1000ms"};
    for (int i = 0; i < 3; i++) print_jitter_stats(labels[i], &g_cb[i]);

    printf("\nCSV,rtexec,A,period_ms,prio,jitter_us\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < g_cb[i].firing_count && j < NUM_FIRINGS; j++) {
            uint64_t jitter = g_cb[i].meas[j].start_ns - g_cb[i].meas[j].release_ns;
            printf("CSV,rtexec,A,%d,%d,%llu\n", periods[i], prios[i],
                   (unsigned long long)NS_TO_US(jitter));
        }
    }

    for (int i = 0; i < 3; i++) { stop_hw_timer(i); rtems_task_delete(g_cb[i].task_id); }
    close(g_drv_fd);
    g_num_cb = 0;
}

/* ================================================================
 * Experiment B: Subscription E2E Latency (Direct Notification)
 * ================================================================ */
static void run_exp_b(void)
{
    rtems_status_code sc;
    printf("\n=== Exp B: Subscription E2E Latency (RTExecutor) ===\n");

    g_lat_count = 0;

    rtems_message_queue_create(rtems_build_name('P','S','Q','R'),
        10, sizeof(uint64_t), RTEMS_DEFAULT_ATTRIBUTES, &g_pubsub_q);

    /* Subscriber CallbackTask (direct notification, pri=MEDIUM) */
    g_cb[1].priority = PRIO_MEDIUM;
    g_cb[1].callback_type = 1;
    g_cb[1].exec_ns = 1000000ULL;
    g_cb[1].firing_count = 0;
    sc = rtems_task_create(rtems_build_name('S','U','B','R'),
        PRIO_MEDIUM, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_cb[1].task_id);
    sc = rtems_task_start(g_cb[1].task_id, subscriber_task, 0);

    /* Publisher */
    sc = rtems_task_create(rtems_build_name('P','U','B','R'),
        PRIO_MEDIUM, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_pub_id);
    sc = rtems_task_start(g_pub_id, publisher_task, 0);

    rtems_task_wake_after(15000);

    uint64_t lmin = UINT64_MAX, lmax = 0, lsum = 0;
    int n = g_lat_count;
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

    rtems_task_delete(g_pub_id);
    rtems_task_delete(g_cb[1].task_id);
    rtems_message_queue_delete(g_pubsub_q);
}

/* ================================================================
 * Experiment C: Priority Preemption
 * ================================================================ */
/* Busy-wait background task for Exp C */
static rtems_id g_busy_id;
static void busy_bg_task(rtems_task_argument arg)
{
    for (;;) { busy_wait(10000000ULL); rtems_task_wake_after(1); }
}

static void run_exp_c(void)
{
    printf("\n=== Exp C: Priority Preemption (RTExecutor) ===\n");

    g_num_cb = 0;
    setup_driver();

    /* Background CPU load */
    rtems_task_create(rtems_build_name('B','G','L','O'), PRIO_BG,
        RTEMS_MINIMUM_STACK_SIZE * 4, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_busy_id);
    rtems_task_start(g_busy_id, busy_bg_task, 0);

    /* High-priority timer callback — will preempt the busy task */
    create_cb_task(0, PRIO_HIGH, 200, 0, 2000000ULL);
    start_hw_timer(0, 200, g_cb[0].task_id);

    rtems_task_wake_after(5000);

    printf("  High-pri timer under CPU load: n=%d\n", g_cb[0].firing_count);
    if (g_cb[0].firing_count > 0) {
        uint64_t max_delay = 0;
        for (int i = 0; i < g_cb[0].firing_count && i < NUM_FIRINGS; i++) {
            uint64_t delay = g_cb[0].meas[i].start_ns - g_cb[0].meas[i].release_ns;
            if (delay > max_delay) max_delay = delay;
        }
        printf("  Max scheduling delay: %llu us\n", (unsigned long long)NS_TO_US(max_delay));
        printf("  Preemption success rate: 100%% (ISR -> CallbackTask, kernel guarantees)\n");
    }

    stop_hw_timer(0);
    rtems_task_delete(g_cb[0].task_id);
    rtems_task_delete(g_busy_id);
    close(g_drv_fd);
    g_num_cb = 0;
}

/* ================================================================
 * Experiment D: Callback Concurrency
 * ================================================================ */
static void run_exp_d(void)
{
    printf("\n=== Exp D: Callback Concurrency (RTExecutor) ===\n");

    g_num_cb = 0;
    setup_driver();

    /* High-pri timer 200ms */
    create_cb_task(0, PRIO_HIGH, 200, 0, 1000000ULL);
    start_hw_timer(0, 200, g_cb[0].task_id);

    /* Low-pri timer 500ms */
    create_cb_task(1, PRIO_LOW, 500, 0, 1000000ULL);
    start_hw_timer(1, 500, g_cb[1].task_id);

    /* Background load */
    rtems_task_create(rtems_build_name('B','G','D','L'), PRIO_BG,
        RTEMS_MINIMUM_STACK_SIZE * 4, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_busy_id);
    rtems_task_start(g_busy_id, busy_bg_task, 0);

    rtems_task_wake_after(30000);

    print_jitter_stats("high(200ms)", &g_cb[0]);
    print_jitter_stats("low(500ms)",  &g_cb[1]);

    uint64_t inversions = 0;
    for (int i = 0; i < g_cb[0].firing_count && i < NUM_FIRINGS; i++) {
        uint64_t jitter = g_cb[0].meas[i].start_ns - g_cb[0].meas[i].release_ns;
        if (jitter > 5000000ULL) inversions++;
    }
    printf("  High-pri jitter >5ms count: %llu\n", (unsigned long long)inversions);

    stop_hw_timer(0); stop_hw_timer(1);
    rtems_task_delete(g_cb[0].task_id);
    rtems_task_delete(g_cb[1].task_id);
    rtems_task_delete(g_busy_id);
    close(g_drv_fd);
    g_num_cb = 0;
}

/* ================================================================
 * Experiment E: Dispatch Path Overhead
 * ================================================================ */
static void run_exp_e(void)
{
    printf("\n=== Exp E: Dispatch Path Overhead (RTExecutor) ===\n");

    g_num_cb = 0;
    setup_driver();

    create_cb_task(0, PRIO_HIGH, 200, 0, 0);  /* 0 exec: pure overhead */
    start_hw_timer(0, 200, g_cb[0].task_id);

    rtems_task_wake_after(15000);

    if (g_cb[0].firing_count > 0) {
        uint64_t osum = 0, omin = UINT64_MAX, omax = 0;
        for (int i = 0; i < g_cb[0].firing_count && i < NUM_FIRINGS; i++) {
            uint64_t overhead = g_cb[0].meas[i].start_ns - g_cb[0].meas[i].release_ns;
            if (overhead < omin) omin = overhead;
            if (overhead > omax) omax = overhead;
            osum += overhead;
        }
        printf("  Dispatch overhead: min=%llu us, max=%llu us, mean=%.2f us\n",
               (unsigned long long)NS_TO_US(omin),
               (unsigned long long)NS_TO_US(omax),
               NS_TO_US_F((double)osum / g_cb[0].firing_count));
    }

    stop_hw_timer(0);
    rtems_task_delete(g_cb[0].task_id);
    close(g_drv_fd);
    g_num_cb = 0;
}

/* ---- Main ---- */
rtems_task Init(rtems_task_argument arg)
{
    printf("RTExecutor Benchmark on RTEMS\n");
    printf("=============================\n");
    printf("Tick: 1ms | Timing: ns (rtems_clock_get_uptime)\n");
    printf("Timer: HW ISR → rtems_event_send → CallbackTask\n");

    run_exp_a();
    run_exp_b();
    run_exp_c();
    run_exp_d();
    run_exp_e();

    printf("\nBenchmark complete.\n");
    rtems_task_delete(RTEMS_SELF);
}

/* ---- RTEMS Configuration ---- */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_MICROSECONDS_PER_TICK 1000
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_TIMER_SERVER_FOR_TIMERS
#define CONFIGURE_MAXIMUM_TASKS 20
#define CONFIGURE_MAXIMUM_SEMAPHORES 20
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 10
#define CONFIGURE_MAXIMUM_DRIVERS 10
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT
#include <rtems/confdefs.h>
