/*
 * RT-ROS (EventsCBGExecutor) Benchmark on RTEMS
 *
 * Faithfully replicates the RT-ROS scheduling model:
 *   - TimerManager: separate task polls timer expiration (software, not ISR)
 *   - Event-driven dispatch: on-trigger callback fires when entity is ready
 *   - CallbackGroup (CBG): MutuallyExclusive groups serialize callbacks via mutex
 *   - FIFO dispatch within each CBG (FirstInFirstOutScheduler)
 *   - NO callback-level preemption: running callback blocks same-CBG callbacks
 *   - Inter-CBG: different CBGs can run concurrently on different tasks
 *
 * All timestamps in nanoseconds via rtems_clock_get_uptime().
 * Tick = 1ms, but measurements have sub-tick (~us) precision.
 *
 * Experiments (matching RTExecutor paper):
 *   A: Timer callback jitter (TimerManager-driven vs ISR-driven)
 *   B: Subscription end-to-end latency (event queue + CBG dispatch)
 *   C: Priority preemption (intra-CBG cannot preempt)
 *   D: Callback concurrency with interference
 *   E: Dispatch path overhead
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

#define MS_TO_NS(ms)   ((uint64_t)(ms) * 1000000000ULL / 1000ULL * 1000ULL)
#define MS_TO_TICKS(ms) ((rtems_interval)(ms))
#define NS_TO_US(ns)   ((ns) / 1000U)
#define NS_TO_US_F(ns) ((double)(ns) / 1000.0)

/* Busy-wait: spin-loop burning CPU for specified ns duration */
static inline void busy_wait(uint64_t ns)
{
    uint64_t deadline = now_ns() + ns;
    while (now_ns() < deadline)
        ;
}

/* ---- Priority definitions (RTEMS: lower value = higher priority) ---- */

#define PRIO_TIMER_MANAGER  40
#define PRIO_HIGH           60
#define PRIO_MEDIUM         70
#define PRIO_LOW            80
#define PRIO_BG            200

#define NUM_FIRINGS   100
#define MAX_CB         8
#define MAX_CBG        4

/* ---- Measurement record ---- */

typedef struct {
    uint64_t release_ns;    /* theoretical/actual release time */
    uint64_t start_ns;      /* callback execution start */
    uint64_t end_ns;        /* callback execution end */
} meas_t;

/* ---- EventsCBGExecutor data structures ---- */

typedef struct {
    rtems_id        task_id;
    rtems_id        cbg_mutex;       /* MutuallyExclusive CBG mutex */
    rtems_id        event_sem;       /* Signal: callback is ready */
    uint32_t        period_ms;       /* Period in ms */
    uint64_t        start_ns;        /* Experiment start time (ns) */
    uint32_t        priority;
    volatile uint32_t firing_count;
    uint32_t        callback_type;   /* 0=timer, 1=subscription */
    uint64_t        exec_ns;         /* Simulated execution time (ns) */
    int             cbg_id;          /* Which CBG this callback belongs to */
    /* Per-callback measurement */
    meas_t          meas[NUM_FIRINGS];
} rtros_cb_t;

/* CBG: a MutuallyExclusive callback group */
typedef struct {
    rtems_id  mutex;
    int       num_callbacks;
    rtros_cb_t *callbacks[MAX_CB];
} rtros_cbg_t;

/* Global state */
static rtros_cb_t  g_cb[MAX_CB];
static rtros_cbg_t g_cbg[MAX_CBG];
static int g_num_cb = 0;
static int g_num_cbg = 0;

/* TimerManager task */
static rtems_id g_timer_mgr_id;

/* Pub/Sub queue */
static rtems_id g_pubsub_q;

/* ---- TimerManager task ----
 *
 * In RT-ROS, the TimerManager runs as a separate thread that polls
 * timer expiration independently of the main wait-set loop.
 * It checks rcl_timer_is_ready() for each registered timer and
 * enqueues timer events to the CBG scheduler.
 *
 * Key: this is SOFTWARE polling, not hardware ISR.
 * The polling introduces variable detection latency.
 */
static void timer_manager_task(rtems_task_argument arg)
{
    int i;
    uint64_t *next_fire_ns = (uint64_t *)arg; /* Array of next fire times */

    for (;;) {
        uint64_t t = now_ns();
        for (i = 0; i < g_num_cb; i++) {
            if (g_cb[i].callback_type == 0) { /* timer callback */
                if (g_cb[i].firing_count < NUM_FIRINGS && t >= next_fire_ns[i]) {
                    /* Timer expired: signal the CBG task */
                    rtems_semaphore_release(g_cb[i].event_sem);
                    next_fire_ns[i] = g_cb[i].start_ns +
                        (uint64_t)(g_cb[i].firing_count + 1) * g_cb[i].period_ms * 1000000ULL;
                }
            }
        }
        /* Yield: poll on every tick (1ms) — this is the TimerManager granularity */
        rtems_task_wake_after(1);
    }
}

/* ---- CBG callback task ----
 *
 * In EventsCBGExecutor, callbacks within a MutuallyExclusive CBG
 * share a mutex and execute serially. No preemption within a CBG.
 * The FirstInFirstOutScheduler dispatches callbacks in arrival order.
 *
 * This faithfully replicates the RT-ROS dispatch model:
 *   1. Wait for event (from TimerManager or publisher)
 *   2. Acquire CBG mutex (MutuallyExclusive serialization)
 *   3. Execute callback
 *   4. Release CBG mutex
 */
static void cbg_task(rtems_task_argument arg)
{
    rtros_cb_t *cb = &g_cb[arg];

    for (;;) {
        /* Wait for event from TimerManager or publisher */
        rtems_semaphore_obtain(cb->event_sem, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

        /* Acquire CBG mutex (MutuallyExclusive) —
         * THIS IS THE KEY: if another callback in the same CBG holds
         * the mutex, we block. No intra-CBG preemption possible. */
        rtems_semaphore_obtain(cb->cbg_mutex, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

        uint32_t idx = cb->firing_count;
        if (idx < NUM_FIRINGS) {
            cb->meas[idx].start_ns = now_ns();

            if (cb->callback_type == 0) { /* timer */
                uint64_t theory_ns = cb->start_ns +
                    (uint64_t)idx * cb->period_ms * 1000000ULL;
                cb->meas[idx].release_ns = theory_ns;
            }

            /* Simulate callback execution (busy-wait, not sleep!) */
            if (cb->exec_ns > 0) {
                busy_wait(cb->exec_ns);
            }

            cb->meas[idx].end_ns = now_ns();
            cb->firing_count++;
        }

        /* Release CBG mutex */
        rtems_semaphore_release(cb->cbg_mutex);
    }
}

/* ---- Helper: create CBG with mutex ---- */
static rtems_status_code create_cbg(int cbg_id)
{
    rtems_status_code sc;
    sc = rtems_semaphore_create(
        rtems_build_name('G', '0' + cbg_id, 'M', 'X'),
        1,  /* initially available */
        RTEMS_PRIORITY | RTEMS_BINARY_SEMAPHORE | RTEMS_INHERIT_PRIORITY,
        0, &g_cbg[cbg_id].mutex);
    g_cbg[cbg_id].num_callbacks = 0;
    g_num_cbg++;
    return sc;
}

/* ---- Helper: create callback task ---- */
static rtems_status_code create_cb(int idx, uint32_t prio, uint32_t period_ms,
                                    uint32_t cb_type, uint64_t exec_ns,
                                    int cbg_id)
{
    rtems_status_code sc;

    g_cb[idx].period_ms = period_ms;
    g_cb[idx].start_ns = now_ns();
    g_cb[idx].priority = prio;
    g_cb[idx].callback_type = cb_type;
    g_cb[idx].exec_ns = exec_ns;
    g_cb[idx].cbg_id = cbg_id;
    g_cb[idx].firing_count = 0;

    /* Assign to CBG */
    g_cbg[cbg_id].callbacks[g_cbg[cbg_id].num_callbacks] = &g_cb[idx];
    g_cbg[cbg_id].num_callbacks++;
    g_cb[idx].cbg_mutex = g_cbg[cbg_id].mutex;

    /* Create event semaphore */
    sc = rtems_semaphore_create(
        rtems_build_name('E', 'V', '0' + idx, 'T'),
        0, RTEMS_SIMPLE_BINARY_SEMAPHORE, 0,
        &g_cb[idx].event_sem);

    /* Create CBG task */
    sc = rtems_task_create(
        rtems_build_name('C', 'B', '0' + idx, 'T'),
        prio, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES,
        &g_cb[idx].task_id);
    sc = rtems_task_start(g_cb[idx].task_id, cbg_task, idx);

    g_num_cb++;
    return sc;
}

/* ---- Helper: start TimerManager ---- */
static uint64_t g_next_fire[MAX_CB];

static void start_timer_manager(void)
{
    int i;
    for (i = 0; i < g_num_cb; i++) {
        if (g_cb[i].callback_type == 0) {
            g_next_fire[i] = g_cb[i].start_ns + g_cb[i].period_ms * 1000000ULL;
        }
    }
    rtems_task_create(rtems_build_name('T', 'M', 'G', 'R'),
        PRIO_TIMER_MANAGER, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES,
        &g_timer_mgr_id);
    rtems_task_start(g_timer_mgr_id, timer_manager_task, (rtems_task_argument)g_next_fire);
}

/* ---- Helper: print jitter statistics ---- */
static void print_jitter_stats(const char *label, rtros_cb_t *cb)
{
    uint64_t jmin = UINT64_MAX, jmax = 0, jsum = 0;
    int n = cb->firing_count;
    int i;
    for (i = 0; i < n && i < NUM_FIRINGS; i++) {
        uint64_t jitter = cb->meas[i].start_ns - cb->meas[i].release_ns;
        if (jitter < jmin) jmin = jitter;
        if (jitter > jmax) jmax = jitter;
        jsum += jitter;
    }
    if (n == 0) { jmin = 0; }
    printf("  %s: n=%d, jitter min=%llu us, max=%llu us, mean=%.2f us\n",
           label, n,
           (unsigned long long)NS_TO_US(jmin),
           (unsigned long long)NS_TO_US(jmax),
           n > 0 ? NS_TO_US_F((double)jsum / n) : 0.0);
}

/* ---- Helper: print latency statistics ---- */
static void print_latency_stats(const char *label, meas_t *m, int n)
{
    uint64_t lmin = UINT64_MAX, lmax = 0, lsum = 0;
    int i;
    for (i = 0; i < n; i++) {
        uint64_t lat = m[i].end_ns - m[i].release_ns;
        if (lat < lmin) lmin = lat;
        if (lat > lmax) lmax = lat;
        lsum += lat;
    }
    if (n == 0) { lmin = 0; }
    printf("  %s: n=%d, latency min=%llu us, max=%llu us, mean=%.2f us\n",
           label, n,
           (unsigned long long)NS_TO_US(lmin),
           (unsigned long long)NS_TO_US(lmax),
           n > 0 ? NS_TO_US_F((double)lsum / n) : 0.0);
}

/* ================================================================
 * Experiment A: Timer Callback Jitter
 * ================================================================
 * TimerManager (software polling) detects timer expiration.
 * CBG mutex serialization adds nondeterministic delay.
 * Compare with RTExecutor's ISR-driven ≤2ms jitter.
 */
static void run_exp_a(void)
{
    rtems_status_code sc;
    int i;

    printf("\n=== Exp A: Timer Callback Jitter (RT-ROS EventsCBG) ===\n");

    /* Create CBGs — each timer callback in its own CBG */
    for (i = 0; i < 3; i++) create_cbg(i);

    /* 3 timer callbacks: 200ms, 500ms, 1000ms */
    uint32_t periods[] = {200, 500, 1000};
    uint32_t prios[]   = {PRIO_HIGH, PRIO_MEDIUM, PRIO_LOW};

    for (i = 0; i < 3; i++) {
        create_cb(i, prios[i], periods[i], 0,  /* timer */
                  1000000ULL,  /* 1ms exec time */
                  i);          /* own CBG */
    }

    start_timer_manager();

    /* Run for 30 seconds */
    rtems_task_wake_after(30000);

    /* Results */
    const char *labels[] = {"200ms", "500ms", "1000ms"};
    for (i = 0; i < 3; i++) {
        print_jitter_stats(labels[i], &g_cb[i]);
    }

    /* CSV output */
    printf("\nCSV,rtros,A,period_ms,prio,jitter_us\n");
    for (i = 0; i < 3; i++) {
        int j;
        for (j = 0; j < g_cb[i].firing_count && j < NUM_FIRINGS; j++) {
            uint64_t jitter = g_cb[i].meas[j].start_ns - g_cb[i].meas[j].release_ns;
            printf("CSV,rtros,A,%d,%d,%llu\n", periods[i], prios[i],
                   (unsigned long long)NS_TO_US(jitter));
        }
    }

    /* Cleanup */
    for (i = 0; i < 3; i++) {
        rtems_task_delete(g_cb[i].task_id);
        rtems_semaphore_delete(g_cb[i].event_sem);
        rtems_semaphore_delete(g_cbg[i].mutex);
    }
    rtems_task_delete(g_timer_mgr_id);
    g_num_cb = 0;
    g_num_cbg = 0;
}

/* ================================================================
 * Experiment B: Subscription End-to-End Latency
 * ================================================================
 * Publisher sends message → event signals subscriber's CBG →
 * subscriber acquires CBG mutex → processes message.
 * The CBG mutex + event dispatch add latency vs direct notification.
 */
static rtems_id g_pub_task_id, g_sub_task_id;
static meas_t g_lat_meas[NUM_FIRINGS];
static int g_lat_count = 0;

static void publisher_task(rtems_task_argument arg)
{
    int i;
    for (i = 0; i < NUM_FIRINGS; i++) {
        uint64_t pub_ns = now_ns();

        /* Publish: send timestamp via message queue */
        rtems_message_queue_send(g_pubsub_q, &pub_ns, sizeof(pub_ns));

        /* Signal subscriber's CBG event */
        rtems_semaphore_release(g_cb[1].event_sem);

        rtems_task_wake_after(100); /* 100ms period */
    }
    rtems_task_delete(RTEMS_SELF);
}

static void subscriber_task(rtems_task_argument arg)
{
    for (;;) {
        rtems_semaphore_obtain(g_cb[1].event_sem, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

        /* Acquire CBG mutex (MutuallyExclusive) */
        rtems_semaphore_obtain(g_cb[1].cbg_mutex, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

        uint64_t pub_ns;
        size_t size;
        rtems_status_code sc;
        sc = rtems_message_queue_receive(g_pubsub_q, &pub_ns, &size,
                                         RTEMS_NO_WAIT, 0);
        if (sc == RTEMS_SUCCESSFUL && g_lat_count < NUM_FIRINGS) {
            g_lat_meas[g_lat_count].release_ns = pub_ns;
            g_lat_meas[g_lat_count].start_ns = now_ns();
            busy_wait(1000000ULL); /* 1ms callback work */
            g_lat_meas[g_lat_count].end_ns = now_ns();
            g_lat_count++;
        }

        rtems_semaphore_release(g_cb[1].cbg_mutex);
    }
}

static void run_exp_b(void)
{
    rtems_status_code sc;

    printf("\n=== Exp B: Subscription E2E Latency (RT-ROS EventsCBG) ===\n");

    g_lat_count = 0;
    g_num_cb = 0;
    g_num_cbg = 0;

    /* Create CBG for subscriber */
    create_cbg(0);

    /* Subscriber callback (index 1) */
    create_cb(1, PRIO_MEDIUM, 0, 1, 1000000ULL, 0);

    /* Message queue */
    sc = rtems_message_queue_create(rtems_build_name('P', 'S', 'Q', '0'),
        10, sizeof(uint64_t), RTEMS_DEFAULT_ATTRIBUTES, &g_pubsub_q);

    /* Subscriber task */
    sc = rtems_task_create(rtems_build_name('S', 'U', 'B', 'T'),
        PRIO_MEDIUM, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_sub_task_id);
    sc = rtems_task_start(g_sub_task_id, subscriber_task, 0);

    /* Publisher task */
    sc = rtems_task_create(rtems_build_name('P', 'U', 'B', 'T'),
        PRIO_MEDIUM, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_pub_task_id);
    sc = rtems_task_start(g_pub_task_id, publisher_task, 0);

    rtems_task_wake_after(15000);

    print_latency_stats("sub_e2e", g_lat_meas, g_lat_count);

    /* CSV */
    printf("\nCSV,rtros,B,latency_us\n");
    for (int i = 0; i < g_lat_count; i++) {
        uint64_t lat = g_lat_meas[i].end_ns - g_lat_meas[i].release_ns;
        printf("CSV,rtros,B,%llu\n", (unsigned long long)NS_TO_US(lat));
    }

    rtems_task_delete(g_pub_task_id);
    rtems_task_delete(g_sub_task_id);
    rtems_semaphore_delete(g_cb[1].event_sem);
    rtems_semaphore_delete(g_cbg[0].mutex);
    rtems_message_queue_delete(g_pubsub_q);
    g_num_cb = 0;
    g_num_cbg = 0;
}

/* ================================================================
 * Experiment C: Priority Preemption
 * ================================================================
 * In EventsCBGExecutor, callbacks within the SAME MutuallyExclusive
 * CBG CANNOT preempt each other — they are serialized by the CBG mutex.
 *
 * Test: Put high-priority timer and low-priority callback in SAME CBG.
 * The high-priority timer must wait for the low-priority callback
 * to release the CBG mutex → priority inversion within CBG.
 *
 * Also test: Put them in DIFFERENT CBGs.
 * High-priority CBG task can preempt low-priority CBG task → works.
 */
static void run_exp_c(void)
{
    rtems_status_code sc;
    int i;

    printf("\n=== Exp C: Priority Preemption (RT-ROS EventsCBG) ===\n");

    /* --- Case 1: Same CBG (intra-CBG cannot preempt) --- */
    printf("\n--- Case 1: High-pri timer + Low-pri cb in SAME CBG ---\n");

    g_num_cb = 0;
    g_num_cbg = 0;
    create_cbg(0);  /* One CBG for both */

    /* Low-priority callback in CBG 0 */
    create_cb(0, PRIO_LOW, 0, 1, 30000000ULL, 0);  /* 30ms exec */

    /* High-priority timer in same CBG 0 */
    create_cb(1, PRIO_HIGH, 100, 0, 2000000ULL, 0); /* 100ms period, 2ms exec */

    start_timer_manager();

    /* Start low-priority callback running */
    rtems_semaphore_release(g_cb[0].event_sem);

    rtems_task_wake_after(2000); /* 2 seconds */

    printf("  High-pri timer in same CBG: n=%d\n", g_cb[1].firing_count);
    if (g_cb[1].firing_count > 0) {
        /* Measure how long high-pri had to wait for CBG mutex */
        uint64_t max_wait = 0;
        for (i = 0; i < g_cb[1].firing_count && i < NUM_FIRINGS; i++) {
            uint64_t wait = g_cb[1].meas[i].start_ns - g_cb[1].meas[i].release_ns;
            if (wait > max_wait) max_wait = wait;
        }
        printf("  Max CBG mutex wait: %llu us (intra-CBG blocking)\n",
               (unsigned long long)NS_TO_US(max_wait));
    }

    /* Cleanup */
    rtems_task_delete(g_timer_mgr_id);
    for (i = 0; i < 2; i++) {
        rtems_task_delete(g_cb[i].task_id);
        rtems_semaphore_delete(g_cb[i].event_sem);
    }
    rtems_semaphore_delete(g_cbg[0].mutex);

    /* --- Case 2: Different CBGs (inter-CBG CAN preempt) --- */
    printf("\n--- Case 2: High-pri timer + Low-pri cb in DIFFERENT CBGs ---\n");

    g_num_cb = 0;
    g_num_cbg = 0;
    create_cbg(0);  /* CBG for low-pri */
    create_cbg(1);  /* CBG for high-pri */

    create_cb(0, PRIO_LOW, 0, 1, 30000000ULL, 0);    /* 30ms, CBG 0 */
    create_cb(1, PRIO_HIGH, 100, 0, 2000000ULL, 1);   /* 100ms, CBG 1 */

    start_timer_manager();

    rtems_semaphore_release(g_cb[0].event_sem);

    rtems_task_wake_after(2000);

    printf("  High-pri timer in different CBG: n=%d\n", g_cb[1].firing_count);
    if (g_cb[1].firing_count > 0) {
        uint64_t max_wait = 0;
        for (i = 0; i < g_cb[1].firing_count && i < NUM_FIRINGS; i++) {
            uint64_t wait = g_cb[1].meas[i].start_ns - g_cb[1].meas[i].release_ns;
            if (wait > max_wait) max_wait = wait;
        }
        printf("  Max dispatch delay: %llu us (inter-CBG preemption works)\n",
               (unsigned long long)NS_TO_US(max_wait));
    }

    /* Cleanup */
    rtems_task_delete(g_timer_mgr_id);
    for (i = 0; i < 2; i++) {
        rtems_task_delete(g_cb[i].task_id);
        rtems_semaphore_delete(g_cb[i].event_sem);
    }
    for (i = 0; i < 2; i++)
        rtems_semaphore_delete(g_cbg[i].mutex);
    g_num_cb = 0;
    g_num_cbg = 0;
}

/* ================================================================
 * Experiment D: Callback Concurrency with Interference
 * ================================================================
 * Two timer callbacks in different CBGs with a background
 * busy-wait task. Measure jitter under sustained CPU load.
 */
static rtems_id g_bg_task_id;

static void bg_busy_task(rtems_task_argument arg)
{
    for (;;) {
        busy_wait(10000000ULL); /* 10ms busy, then yield */
        rtems_task_wake_after(1);
    }
}

static void run_exp_d(void)
{
    rtems_status_code sc;
    int i;

    printf("\n=== Exp D: Callback Concurrency (RT-ROS EventsCBG) ===\n");

    g_num_cb = 0;
    g_num_cbg = 0;
    create_cbg(0);
    create_cbg(1);

    /* High-pri timer 200ms */
    create_cb(0, PRIO_HIGH, 200, 0, 1000000ULL, 0);
    /* Low-pri timer 500ms */
    create_cb(1, PRIO_LOW, 500, 0, 1000000ULL, 1);

    start_timer_manager();

    /* Background CPU load */
    sc = rtems_task_create(rtems_build_name('B', 'G', 'L', 'O'),
        PRIO_BG, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_bg_task_id);
    sc = rtems_task_start(g_bg_task_id, bg_busy_task, 0);

    rtems_task_wake_after(30000);

    print_jitter_stats("high(200ms)", &g_cb[0]);
    print_jitter_stats("low(500ms)",  &g_cb[1]);

    /* Priority inversions = cases where high-pri CBG had to wait */
    uint64_t inversions = 0;
    for (i = 0; i < g_cb[0].firing_count && i < NUM_FIRINGS; i++) {
        uint64_t jitter = g_cb[0].meas[i].start_ns - g_cb[0].meas[i].release_ns;
        if (jitter > 5000000ULL) inversions++; /* >5ms = likely blocked */
    }
    printf("  High-pri jitter >5ms count: %llu (possible CBG blocking)\n",
           (unsigned long long)inversions);

    rtems_task_delete(g_bg_task_id);
    rtems_task_delete(g_timer_mgr_id);
    for (i = 0; i < 2; i++) {
        rtems_task_delete(g_cb[i].task_id);
        rtems_semaphore_delete(g_cb[i].event_sem);
    }
    for (i = 0; i < 2; i++)
        rtems_semaphore_delete(g_cbg[i].mutex);
    g_num_cb = 0;
    g_num_cbg = 0;
}

/* ================================================================
 * Experiment E: Dispatch Path Overhead
 * ================================================================
 * Measure: TimerManager detection → CBG event signal →
 *          CBG task wakeup → CBG mutex acquire → callback start
 */
static void run_exp_e(void)
{
    rtems_status_code sc;
    int i;

    printf("\n=== Exp E: Dispatch Path Overhead (RT-ROS EventsCBG) ===\n");

    g_num_cb = 0;
    g_num_cbg = 0;
    create_cbg(0);

    /* Single timer callback for clean measurement */
    create_cb(0, PRIO_HIGH, 200, 0, 0, 0);  /* 0 exec_ns: measure pure overhead */

    start_timer_manager();

    rtems_task_wake_after(15000);

    if (g_cb[0].firing_count > 0) {
        uint64_t osum = 0, omin = UINT64_MAX, omax = 0;
        for (i = 0; i < g_cb[0].firing_count && i < NUM_FIRINGS; i++) {
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

    rtems_task_delete(g_timer_mgr_id);
    rtems_task_delete(g_cb[0].task_id);
    rtems_semaphore_delete(g_cb[0].event_sem);
    rtems_semaphore_delete(g_cbg[0].mutex);
    g_num_cb = 0;
    g_num_cbg = 0;
}

/* ---- Main ---- */
rtems_task Init(rtems_task_argument arg)
{
    printf("RT-ROS (EventsCBGExecutor) Benchmark on RTEMS\n");
    printf("=============================================\n");
    printf("Tick: 1ms | Timing: ns (rtems_clock_get_uptime)\n");

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
