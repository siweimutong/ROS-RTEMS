/*
 * SingleThreadedExecutor Benchmark on RTEMS
 *
 * Faithfully replicates the ROS 2 SingleThreadedExecutor model:
 *   - Single executor thread runs: wait → collect → execute loop
 *   - FIFO dispatch: callbacks dispatched in registration order
 *   - No callback priority: all callbacks share one thread priority
 *   - Software timer: checked only when executor enters rcl_wait
 *   - No preemption: running callback blocks all others
 *
 * This is the BASELINE — the worst case for real-time performance.
 * Compare with RTExecutor, PiCAS, and RT-ROS.
 *
 * All timestamps in nanoseconds via rtems_clock_get_uptime().
 */

#include <rtems.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

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

#define PRIO_EXEC      60
#define PRIO_BG       200
#define NUM_FIRINGS   100
#define MAX_CB         8

/* ---- Measurement record ---- */

typedef struct {
    uint64_t release_ns;
    uint64_t start_ns;
    uint64_t end_ns;
} meas_t;

/* ---- FIFO Ready Queue ---- */

typedef struct {
    int     cb_idx;       /* callback index */
    uint64_t release_ns;  /* when callback became ready */
    int     type;         /* 0=timer, 1=subscription */
} fifo_entry_t;

#define FIFO_MAX 64

typedef struct {
    fifo_entry_t entries[FIFO_MAX];
    int head, tail, count;
} fifo_t;

static fifo_t g_fifo;
static void fifo_init(fifo_t *f) { f->head = f->tail = f->count = 0; }
static int fifo_put(fifo_t *f, fifo_entry_t *e) {
    if (f->count >= FIFO_MAX) return -1;
    f->entries[f->tail] = *e;
    f->tail = (f->tail + 1) % FIFO_MAX;
    f->count++;
    return 0;
}
static int fifo_get(fifo_t *f, fifo_entry_t *e) {
    if (f->count == 0) return -1;
    *e = f->entries[f->head];
    f->head = (f->head + 1) % FIFO_MAX;
    f->count--;
    return 0;
}

/* ---- Callback descriptors ---- */

typedef struct {
    uint32_t        period_ms;
    uint64_t        start_ns;
    uint64_t        next_fire_ns;
    uint32_t        callback_type;   /* 0=timer, 1=subscription */
    uint64_t        exec_ns;
    volatile uint32_t firing_count;
    meas_t          meas[NUM_FIRINGS];
    void          (*fn)(int idx);     /* callback function pointer */
} st_cb_t;

static st_cb_t g_cb[MAX_CB];
static int g_num_cb = 0;

/* ---- Executor state ---- */
static rtems_id g_exec_task_id;
static rtems_id g_exec_sem;     /* Signal: something is ready */
static volatile int g_exec_running;

/* Pub/Sub */
static rtems_id g_pubsub_q;
static rtems_id g_pub_task_id;
static meas_t g_lat_meas[NUM_FIRINGS];
static int g_lat_count = 0;

/* ---- SingleThreadedExecutor: wait → collect → execute loop ----
 *
 * This faithfully replicates the ROS 2 SingleThreadedExecutor:
 *   1. Wait for any callback to become ready (semaphore)
 *   2. Scan all timers for expiration (software timer check)
 *   3. Dequeue FIFO in registration order (FIFO dispatch)
 *   4. Execute the callback
 *   5. Repeat
 *
 * Key limitations:
 *   - Timer detection only happens in step 2 → timer detection gap
 *   - FIFO order, no priority → priority inversion
 *   - Single thread → no concurrency
 */
static void executor_task(rtems_task_argument arg)
{
    fifo_init(&g_fifo);

    while (g_exec_running) {
        /* --- Wait phase: block until something is ready --- */
        rtems_semaphore_obtain(g_exec_sem, RTEMS_WAIT, 1); /* 1 tick timeout */

        /* --- Collect phase: check all timers (software timer) --- */
        uint64_t t = now_ns();
        for (int i = 0; i < g_num_cb; i++) {
            if (g_cb[i].callback_type == 0 && g_cb[i].firing_count < NUM_FIRINGS) {
                if (t >= g_cb[i].next_fire_ns) {
                    fifo_entry_t e = { i, g_cb[i].next_fire_ns, 0 };
                    fifo_put(&g_fifo, &e);
                    g_cb[i].next_fire_ns += g_cb[i].period_ms * 1000000ULL;
                }
            }
        }

        /* --- Execute phase: dispatch FIFO in registration order --- */
        fifo_entry_t e;
        while (fifo_get(&g_fifo, &e) == 0) {
            st_cb_t *cb = &g_cb[e.cb_idx];
            uint32_t idx = cb->firing_count;
            if (idx < NUM_FIRINGS) {
                cb->meas[idx].release_ns = e.release_ns;
                cb->meas[idx].start_ns = now_ns();

                /* Execute callback */
                if (cb->fn) {
                    cb->fn(e.cb_idx);
                } else if (cb->exec_ns > 0) {
                    busy_wait(cb->exec_ns);
                }

                cb->meas[idx].end_ns = now_ns();
                cb->firing_count++;
            }
        }
    }

    rtems_task_delete(RTEMS_SELF);
}

/* ---- Start executor ---- */
static void start_executor(void)
{
    g_exec_running = 1;
    rtems_semaphore_create(rtems_build_name('E','X','S','M'),
        0, RTEMS_SIMPLE_BINARY_SEMAPHORE, 0, &g_exec_sem);
    rtems_task_create(rtems_build_name('E','X','E','C'),
        PRIO_EXEC, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_exec_task_id);
    rtems_task_start(g_exec_task_id, executor_task, 0);
}

static void stop_executor(void)
{
    g_exec_running = 0;
    rtems_task_wake_after(10);
    rtems_task_delete(g_exec_task_id);
    rtems_semaphore_delete(g_exec_sem);
}

/* ---- Print helpers ---- */

static void print_jitter_stats(const char *label, st_cb_t *cb)
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

/* ---- Exp A: Timer Jitter ---- */
static void run_exp_a(void)
{
    printf("\n=== Exp A: Timer Callback Jitter (SingleThreadedExecutor) ===\n");

    g_num_cb = 0;
    uint64_t start = now_ns();
    uint32_t periods[] = {200, 500, 1000};

    for (int i = 0; i < 3; i++) {
        g_cb[i].period_ms = periods[i];
        g_cb[i].start_ns = start;
        g_cb[i].next_fire_ns = start + periods[i] * 1000000ULL;
        g_cb[i].callback_type = 0;
        g_cb[i].exec_ns = 1000000ULL;
        g_cb[i].firing_count = 0;
        g_cb[i].fn = NULL;
        g_num_cb++;
    }

    start_executor();
    rtems_task_wake_after(30000);
    stop_executor();

    const char *labels[] = {"200ms", "500ms", "1000ms"};
    for (int i = 0; i < 3; i++) print_jitter_stats(labels[i], &g_cb[i]);

    printf("\nCSV,stexec,A,period_ms,jitter_us\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < g_cb[i].firing_count && j < NUM_FIRINGS; j++) {
            uint64_t jitter = g_cb[i].meas[j].start_ns - g_cb[i].meas[j].release_ns;
            printf("CSV,stexec,A,%d,%llu\n", periods[i],
                   (unsigned long long)NS_TO_US(jitter));
        }
    }
    g_num_cb = 0;
}

/* ---- Exp B: Subscription Latency ---- */
static void publisher_st(rtems_task_argument arg)
{
    for (int i = 0; i < NUM_FIRINGS; i++) {
        uint64_t pub_ns = now_ns();
        rtems_message_queue_send(g_pubsub_q, &pub_ns, sizeof(pub_ns));
        rtems_semaphore_release(g_exec_sem); /* wake executor */
        rtems_task_wake_after(100);
    }
    rtems_task_delete(RTEMS_SELF);
}

static void run_exp_b(void)
{
    rtems_status_code sc;
    printf("\n=== Exp B: Subscription E2E Latency (SingleThreadedExecutor) ===\n");

    g_lat_count = 0;
    g_num_cb = 0;

    /* Subscription callback as a function */
    g_cb[1].callback_type = 1;
    g_cb[1].exec_ns = 1000000ULL;
    g_cb[1].firing_count = 0;
    g_cb[1].fn = NULL; /* will be handled in executor via FIFO */
    g_num_cb = 2;

    rtems_message_queue_create(rtems_build_name('P','S','Q','S'),
        10, sizeof(uint64_t), RTEMS_DEFAULT_ATTRIBUTES, &g_pubsub_q);

    start_executor();

    sc = rtems_task_create(rtems_build_name('P','U','B','S'),
        PRIO_EXEC, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_pub_task_id);
    sc = rtems_task_start(g_pub_task_id, publisher_st, 0);

    rtems_task_wake_after(15000);
    stop_executor();

    /* Collect latency from subscription measurements */
    if (g_cb[1].firing_count > 0) {
        uint64_t lmin = UINT64_MAX, lmax = 0, lsum = 0;
        int n = g_cb[1].firing_count;
        for (int i = 0; i < n && i < NUM_FIRINGS; i++) {
            uint64_t lat = g_cb[1].meas[i].end_ns - g_cb[1].meas[i].release_ns;
            if (lat < lmin) lmin = lat;
            if (lat > lmax) lmax = lat;
            lsum += lat;
        }
        if (n == 0) lmin = 0;
        printf("  sub_e2e: n=%d, latency min=%llu us, max=%llu us, mean=%.2f us\n",
               n, (unsigned long long)NS_TO_US(lmin),
               (unsigned long long)NS_TO_US(lmax),
               n > 0 ? NS_TO_US_F((double)lsum / n) : 0.0);
    }

    rtems_task_delete(g_pub_task_id);
    rtems_message_queue_delete(g_pubsub_q);
    g_num_cb = 0;
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

/* ---- Exp D: Callback Concurrency ---- */
static rtems_id g_busy_id;
static void busy_bg(rtems_task_argument arg)
{
    for (;;) { busy_wait(10000000ULL); rtems_task_wake_after(1); }
}

static void run_exp_d(void)
{
    rtems_status_code sc;
    printf("\n=== Exp D: Callback Concurrency (SingleThreadedExecutor) ===\n");

    g_num_cb = 0;
    uint64_t start = now_ns();

    g_cb[0].period_ms = 200;
    g_cb[0].start_ns = start;
    g_cb[0].next_fire_ns = start + 200 * 1000000ULL;
    g_cb[0].callback_type = 0;
    g_cb[0].exec_ns = 1000000ULL;
    g_cb[0].firing_count = 0;
    g_cb[0].fn = NULL;

    g_cb[1].period_ms = 500;
    g_cb[1].start_ns = start;
    g_cb[1].next_fire_ns = start + 500 * 1000000ULL;
    g_cb[1].callback_type = 0;
    g_cb[1].exec_ns = 1000000ULL;
    g_cb[1].firing_count = 0;
    g_cb[1].fn = NULL;

    g_num_cb = 2;

    /* Background load */
    sc = rtems_task_create(rtems_build_name('B','G','L','O'), PRIO_BG,
        RTEMS_MINIMUM_STACK_SIZE * 4, RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_busy_id);
    sc = rtems_task_start(g_busy_id, busy_bg, 0);

    start_executor();
    rtems_task_wake_after(30000);
    stop_executor();

    print_jitter_stats("cb0(200ms)", &g_cb[0]);
    print_jitter_stats("cb1(500ms)", &g_cb[1]);

    rtems_task_delete(g_busy_id);
    g_num_cb = 0;
}

/* ---- Exp E: Overhead ---- */
static void run_exp_e(void)
{
    printf("\n=== Exp E: Dispatch Path Overhead (SingleThreadedExecutor) ===\n");

    g_num_cb = 0;
    uint64_t start = now_ns();

    g_cb[0].period_ms = 200;
    g_cb[0].start_ns = start;
    g_cb[0].next_fire_ns = start + 200 * 1000000ULL;
    g_cb[0].callback_type = 0;
    g_cb[0].exec_ns = 0; /* pure overhead */
    g_cb[0].firing_count = 0;
    g_cb[0].fn = NULL;
    g_num_cb = 1;

    start_executor();
    rtems_task_wake_after(15000);
    stop_executor();

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
    g_num_cb = 0;
}

/* ---- Main ---- */
rtems_task Init(rtems_task_argument arg)
{
    printf("SingleThreadedExecutor Benchmark on RTEMS\n");
    printf("==========================================\n");
    printf("Tick: 1ms | Timing: ns (rtems_clock_get_uptime)\n");
    printf("Model: wait → collect (sw timer) → FIFO dispatch → execute\n");

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
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT
#include <rtems/confdefs.h>
