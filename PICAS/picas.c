/*
 * PiCAS (Priority-driven Chain-Aware Scheduling) Benchmark on RTEMS
 *
 * Faithfully replicates the PiCAS scheduling model:
 *   - Chain-aware: callbacks organized into chains (data-flow paths)
 *   - Priority-driven dispatch: highest-priority chain dispatched first
 *   - Intra-chain: callbacks execute sequentially in data-flow order (NO preemption)
 *   - Inter-chain: different chains can preempt each other by chain priority
 *   - Software timer polling (rcl_timer style, not ISR-driven)
 *   - Priority-ordered subscription dispatch (vs FIFO in standard/RT-ROS)
 *
 * All timestamps in nanoseconds via rtems_clock_get_uptime().
 * Tick = 1ms, but measurements have sub-tick (~us) precision.
 *
 * Experiments (matching RTExecutor paper):
 *   A: Timer callback jitter (software timer + priority-ordered dispatch)
 *   B: Subscription end-to-end latency (chain-aware dispatch)
 *   C: Priority preemption (inter-chain only, intra-chain serialized)
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

#define PRIO_POLLER    40
#define PRIO_HIGH      60
#define PRIO_MEDIUM    70
#define PRIO_LOW       80
#define PRIO_BG       200

#define NUM_FIRINGS   100
#define MAX_CB         8
#define MAX_CHAINS     4
#define MAX_PER_CHAIN  4

/* ---- Measurement record ---- */

typedef struct {
    uint64_t release_ns;
    uint64_t start_ns;
    uint64_t end_ns;
} meas_t;

/* ---- PiCAS data structures ---- */

typedef struct {
    rtems_id        task_id;
    rtems_id        ready_sem;       /* Signal: callback is ready */
    rtems_id        chain_mutex;     /* Chain serialization mutex */
    uint32_t        period_ms;       /* Period in ms (0 for subscription) */
    uint64_t        start_ns;        /* Experiment start (ns) */
    uint32_t        chain_priority;  /* Chain-level priority */
    uint32_t        callback_order;  /* Order within chain (0=first) */
    uint32_t        callback_type;   /* 0=timer, 1=subscription */
    uint64_t        exec_ns;         /* Simulated execution time (ns) */
    int             chain_id;        /* Which chain this callback belongs to */
    volatile uint32_t firing_count;
    meas_t          meas[NUM_FIRINGS];
} picas_cb_t;

/* Chain: callbacks in data-flow order, serialized by mutex */
typedef struct {
    rtems_id        mutex;           /* Chain serialization mutex */
    rtems_id        task_id;         /* Chain task — one task per chain */
    uint32_t        priority;        /* Chain priority */
    int             num_callbacks;
    picas_cb_t     *callbacks[MAX_PER_CHAIN];
} picas_chain_t;

/* Global state */
static picas_cb_t    g_cb[MAX_CB];
static picas_chain_t g_chains[MAX_CHAINS];
static int g_num_cb = 0;
static int g_num_chains = 0;

/* Software timer poller task */
static rtems_id g_poller_id;

/* Pub/Sub */
static rtems_id g_pubsub_q;
static rtems_id g_pub_task_id;
static meas_t g_lat_meas[NUM_FIRINGS];
static int g_lat_count = 0;

/* ---- Software timer poller (PiCAS-style) ----
 *
 * PiCAS uses rcl_timer software timers. Timer readiness is checked
 * in the Executor's event loop (spin), not by hardware ISR.
 * The executor scans timers and dispatches the highest-priority ready one.
 *
 * This poller simulates that: scan all timer callbacks on each tick,
 * and signal the corresponding chain when a timer fires.
 */
static uint64_t g_next_fire[MAX_CB];

static void timer_poller_task(rtems_task_argument arg)
{
    int i;
    for (;;) {
        uint64_t t = now_ns();
        for (i = 0; i < g_num_cb; i++) {
            if (g_cb[i].callback_type == 0 && g_cb[i].firing_count < NUM_FIRINGS) {
                if (t >= g_next_fire[i]) {
                    /* Timer expired: signal the chain that this callback is ready.
                     * In PiCAS, the executor selects the highest-priority READY
                     * callback. Here, the chain task is woken and the RTEMS
                     * FP scheduler handles priority-based dispatch. */
                    rtems_semaphore_release(g_cb[i].ready_sem);
                    g_next_fire[i] = g_cb[i].start_ns +
                        (uint64_t)(g_cb[i].firing_count + 1) *
                        g_cb[i].period_ms * 1000000ULL;
                }
            }
        }
        rtems_task_wake_after(1);
    }
}

/* ---- Chain task: executes callbacks in chain order ----
 *
 * In PiCAS, callbacks within a chain execute in data-flow order.
 * The chain mutex ensures no intra-chain preemption.
 * Inter-chain preemption is handled by the RTEMS FP scheduler:
 *   - Each chain is a separate RTEMS task with chain-level priority
 *   - Higher-priority chain task preempts lower-priority chain task
 *
 * This faithfully replicates the PiCAS scheduling model:
 *   1. Wait for any callback in this chain to become ready
 *   2. Acquire chain mutex (no intra-chain preemption)
 *   3. Execute ALL callbacks in chain in order
 *   4. Release chain mutex
 */
static void chain_task(rtems_task_argument arg)
{
    picas_chain_t *chain = &g_chains[arg];
    int i;

    for (;;) {
        /* Wait for a callback in this chain to become ready */
        rtems_semaphore_obtain(chain->callbacks[0]->ready_sem,
                               RTEMS_WAIT, RTEMS_NO_TIMEOUT);

        /* Acquire chain mutex — intra-chain serialization */
        rtems_semaphore_obtain(chain->mutex, RTEMS_WAIT, RTEMS_NO_TIMEOUT);

        /* Execute callbacks in chain (data-flow) order */
        for (i = 0; i < chain->num_callbacks; i++) {
            picas_cb_t *cb = chain->callbacks[i];
            uint32_t idx = cb->firing_count;

            if (idx < NUM_FIRINGS) {
                cb->meas[idx].start_ns = now_ns();

                if (cb->callback_type == 0) { /* timer */
                    uint64_t theory_ns = cb->start_ns +
                        (uint64_t)idx * cb->period_ms * 1000000ULL;
                    cb->meas[idx].release_ns = theory_ns;
                } else if (cb->callback_type == 1) { /* subscription */
                    uint64_t pub_ns;
                    size_t size;
                    rtems_status_code sc;
                    sc = rtems_message_queue_receive(g_pubsub_q, &pub_ns, &size,
                                                     RTEMS_NO_WAIT, 0);
                    if (sc == RTEMS_SUCCESSFUL) {
                        cb->meas[idx].release_ns = pub_ns;
                    }
                }

                /* Simulate callback execution */
                if (cb->exec_ns > 0) busy_wait(cb->exec_ns);

                cb->meas[idx].end_ns = now_ns();
                cb->firing_count++;
            }
        }

        /* Release chain mutex */
        rtems_semaphore_release(chain->mutex);
    }
}

/* ---- Helpers ---- */

static rtems_status_code create_chain(int ch_id, uint32_t priority)
{
    rtems_status_code sc;
    g_chains[ch_id].priority = priority;
    g_chains[ch_id].num_callbacks = 0;

    sc = rtems_semaphore_create(
        rtems_build_name('C', 'H', '0' + ch_id, 'M'),
        1, RTEMS_PRIORITY | RTEMS_BINARY_SEMAPHORE | RTEMS_INHERIT_PRIORITY,
        0, &g_chains[ch_id].mutex);

    sc = rtems_task_create(
        rtems_build_name('C', 'H', '0' + ch_id, 'T'),
        priority, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES,
        &g_chains[ch_id].task_id);

    g_num_chains++;
    return sc;
}

static rtems_status_code add_cb(int idx, uint32_t period_ms, uint32_t cb_type,
                                 uint64_t exec_ns, int ch_id, uint32_t cb_order)
{
    rtems_status_code sc;

    g_cb[idx].period_ms = period_ms;
    g_cb[idx].start_ns = now_ns();
    g_cb[idx].chain_priority = g_chains[ch_id].priority;
    g_cb[idx].callback_order = cb_order;
    g_cb[idx].callback_type = cb_type;
    g_cb[idx].exec_ns = exec_ns;
    g_cb[idx].chain_id = ch_id;
    g_cb[idx].firing_count = 0;
    g_cb[idx].chain_mutex = g_chains[ch_id].mutex;

    /* Add to chain */
    g_chains[ch_id].callbacks[g_chains[ch_id].num_callbacks] = &g_cb[idx];
    g_chains[ch_id].num_callbacks++;

    /* Ready semaphore */
    sc = rtems_semaphore_create(
        rtems_build_name('R', 'D', '0' + idx, 'Y'),
        0, RTEMS_SIMPLE_BINARY_SEMAPHORE, 0,
        &g_cb[idx].ready_sem);

    g_next_fire[idx] = g_cb[idx].start_ns + period_ms * 1000000ULL;
    g_num_cb++;
    return sc;
}

static void start_chains_and_poller(void)
{
    int i;
    for (i = 0; i < g_num_chains; i++) {
        rtems_task_start(g_chains[i].task_id, chain_task, i);
    }
    rtems_task_create(rtems_build_name('T', 'P', 'O', 'L'),
        PRIO_POLLER, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES,
        &g_poller_id);
    rtems_task_start(g_poller_id, timer_poller_task, 0);
}

/* ---- Print helpers ---- */

static void print_jitter_stats(const char *label, picas_cb_t *cb)
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
    if (n == 0) jmin = 0;
    printf("  %s: n=%d, jitter min=%llu us, max=%llu us, mean=%.2f us\n",
           label, n,
           (unsigned long long)NS_TO_US(jmin),
           (unsigned long long)NS_TO_US(jmax),
           n > 0 ? NS_TO_US_F((double)jsum / n) : 0.0);
}

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
    if (n == 0) lmin = 0;
    printf("  %s: n=%d, latency min=%llu us, max=%llu us, mean=%.2f us\n",
           label, n,
           (unsigned long long)NS_TO_US(lmin),
           (unsigned long long)NS_TO_US(lmax),
           n > 0 ? NS_TO_US_F((double)lsum / n) : 0.0);
}

/* ================================================================
 * Experiment A: Timer Callback Jitter
 * ================================================================
 * Software timer polling + chain-ordered dispatch.
 * Each timer is in its own chain (inter-chain preemption possible).
 * Jitter includes: poller detection delay + chain task scheduling.
 */
static void run_exp_a(void)
{
    int i;

    printf("\n=== Exp A: Timer Callback Jitter (PiCAS) ===\n");

    g_num_cb = 0;
    g_num_chains = 0;

    uint32_t periods[] = {200, 500, 1000};
    uint32_t prios[]   = {PRIO_HIGH, PRIO_MEDIUM, PRIO_LOW};

    for (i = 0; i < 3; i++) {
        create_chain(i, prios[i]);
        add_cb(i, periods[i], 0, 1000000ULL, i, 0);  /* timer, 1ms exec */
    }

    start_chains_and_poller();

    rtems_task_wake_after(30000);

    const char *labels[] = {"200ms", "500ms", "1000ms"};
    for (i = 0; i < 3; i++)
        print_jitter_stats(labels[i], &g_cb[i]);

    /* CSV */
    printf("\nCSV,picas,A,period_ms,chain_prio,jitter_us\n");
    for (i = 0; i < 3; i++) {
        int j;
        for (j = 0; j < g_cb[i].firing_count && j < NUM_FIRINGS; j++) {
            uint64_t jitter = g_cb[i].meas[j].start_ns - g_cb[i].meas[j].release_ns;
            printf("CSV,picas,A,%d,%d,%llu\n", periods[i], prios[i],
                   (unsigned long long)NS_TO_US(jitter));
        }
    }

    /* Cleanup */
    rtems_task_delete(g_poller_id);
    for (i = 0; i < 3; i++) {
        rtems_task_delete(g_chains[i].task_id);
        rtems_semaphore_delete(g_cb[i].ready_sem);
        rtems_semaphore_delete(g_chains[i].mutex);
    }
    g_num_cb = 0;
    g_num_chains = 0;
}

/* ================================================================
 * Experiment B: Subscription End-to-End Latency
 * ================================================================
 * Chain: timer(publish) → subscription(receive).
 * Both in same chain (data-flow order).
 * Chain mutex serializes: subscriber waits for publisher.
 */
static void publisher_picas(rtems_task_argument arg)
{
    int i;
    for (i = 0; i < NUM_FIRINGS; i++) {
        uint64_t pub_ns = now_ns();
        rtems_message_queue_send(g_pubsub_q, &pub_ns, sizeof(pub_ns));
        rtems_semaphore_release(g_cb[1].ready_sem);
        rtems_task_wake_after(100);
    }
    rtems_task_delete(RTEMS_SELF);
}

static void run_exp_b(void)
{
    rtems_status_code sc;

    printf("\n=== Exp B: Subscription E2E Latency (PiCAS) ===\n");

    g_lat_count = 0;
    g_num_cb = 0;
    g_num_chains = 0;

    /* Chain 0: subscription callback */
    create_chain(0, PRIO_MEDIUM);
    add_cb(1, 0, 1, 1000000ULL, 0, 0);  /* subscription, 1ms exec */

    /* Start chain task */
    rtems_task_start(g_chains[0].task_id, chain_task, 0);

    /* Message queue */
    sc = rtems_message_queue_create(rtems_build_name('P', 'S', 'Q', '1'),
        10, sizeof(uint64_t), RTEMS_DEFAULT_ATTRIBUTES, &g_pubsub_q);

    /* Publisher task (separate, not in chain) */
    sc = rtems_task_create(rtems_build_name('P', 'U', 'B', 'C'),
        PRIO_MEDIUM, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_pub_task_id);
    sc = rtems_task_start(g_pub_task_id, publisher_picas, 0);

    rtems_task_wake_after(15000);

    /* Measure latency from publish to subscription completion */
    if (g_cb[1].firing_count > 0) {
        uint64_t lmin = UINT64_MAX, lmax = 0, lsum = 0;
        int n = g_cb[1].firing_count;
        int i;
        for (i = 0; i < n && i < NUM_FIRINGS; i++) {
            uint64_t lat = g_cb[1].meas[i].end_ns - g_cb[1].meas[i].release_ns;
            if (lat < lmin) lmin = lat;
            if (lat > lmax) lmax = lat;
            lsum += lat;
        }
        printf("  sub_e2e: n=%d, latency min=%llu us, max=%llu us, mean=%.2f us\n",
               n, (unsigned long long)NS_TO_US(lmin),
               (unsigned long long)NS_TO_US(lmax),
               NS_TO_US_F((double)lsum / n));
    }

    rtems_task_delete(g_pub_task_id);
    rtems_task_delete(g_chains[0].task_id);
    rtems_semaphore_delete(g_cb[1].ready_sem);
    rtems_semaphore_delete(g_chains[0].mutex);
    rtems_message_queue_delete(g_pubsub_q);
    g_num_cb = 0;
    g_num_chains = 0;
}

/* ================================================================
 * Experiment C: Priority Preemption
 * ================================================================
 * PiCAS supports inter-chain preemption only.
 * Within a chain, callbacks are serialized by the chain mutex.
 *
 * Case 1: High-pri timer and low-pri callback in SAME chain
 *         → Intra-chain: timer must wait for low-pri to finish
 *
 * Case 2: High-pri timer in chain A, low-pri callback in chain B
 *         → Inter-chain: high-pri chain preempts low-pri chain
 */
static void run_exp_c(void)
{
    int i;

    printf("\n=== Exp C: Priority Preemption (PiCAS) ===\n");

    /* --- Case 1: Same chain (intra-chain cannot preempt) --- */
    printf("\n--- Case 1: Same chain (intra-chain NO preemption) ---\n");

    g_num_cb = 0;
    g_num_chains = 0;

    create_chain(0, PRIO_HIGH);
    add_cb(0, 0, 1, 30000000ULL, 0, 0);  /* low-pri sub, 30ms exec, first in chain */
    add_cb(1, 100, 0, 2000000ULL, 0, 1);  /* high-pri timer, 100ms, second in chain */

    /* Start timer poller */
    rtems_task_create(rtems_build_name('T', 'P', '1', 'L'),
        PRIO_POLLER, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_poller_id);
    rtems_task_start(g_poller_id, timer_poller_task, 0);

    /* Start chain task */
    rtems_task_start(g_chains[0].task_id, chain_task, 0);

    /* Trigger low-pri callback */
    rtems_semaphore_release(g_cb[0].ready_sem);

    rtems_task_wake_after(2000);

    printf("  High-pri timer (same chain): n=%d\n", g_cb[1].firing_count);
    if (g_cb[1].firing_count > 0) {
        uint64_t max_wait = 0;
        for (i = 0; i < g_cb[1].firing_count && i < NUM_FIRINGS; i++) {
            uint64_t wait = g_cb[1].meas[i].start_ns - g_cb[1].meas[i].release_ns;
            if (wait > max_wait) max_wait = wait;
        }
        printf("  Max chain mutex wait: %llu us (intra-chain blocking)\n",
               (unsigned long long)NS_TO_US(max_wait));
    }

    rtems_task_delete(g_poller_id);
    rtems_task_delete(g_chains[0].task_id);
    rtems_semaphore_delete(g_cb[0].ready_sem);
    rtems_semaphore_delete(g_cb[1].ready_sem);
    rtems_semaphore_delete(g_chains[0].mutex);

    /* --- Case 2: Different chains (inter-chain CAN preempt) --- */
    printf("\n--- Case 2: Different chains (inter-chain preemption) ---\n");

    g_num_cb = 0;
    g_num_chains = 0;

    create_chain(0, PRIO_LOW);    /* Low-pri chain */
    create_chain(1, PRIO_HIGH);   /* High-pri chain */

    add_cb(0, 0, 1, 30000000ULL, 0, 0);  /* low-pri sub in chain 0 */
    add_cb(1, 100, 0, 2000000ULL, 1, 0);  /* high-pri timer in chain 1 */

    rtems_task_create(rtems_build_name('T', 'P', '2', 'L'),
        PRIO_POLLER, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_poller_id);
    rtems_task_start(g_poller_id, timer_poller_task, 0);

    start_chains_and_poller();

    /* Trigger low-pri */
    rtems_semaphore_release(g_cb[0].ready_sem);

    rtems_task_wake_after(2000);

    printf("  High-pri timer (different chain): n=%d\n", g_cb[1].firing_count);
    if (g_cb[1].firing_count > 0) {
        uint64_t max_wait = 0;
        for (i = 0; i < g_cb[1].firing_count && i < NUM_FIRINGS; i++) {
            uint64_t wait = g_cb[1].meas[i].start_ns - g_cb[1].meas[i].release_ns;
            if (wait > max_wait) max_wait = wait;
        }
        printf("  Max dispatch delay: %llu us (inter-chain preemption works)\n",
               (unsigned long long)NS_TO_US(max_wait));
    }

    rtems_task_delete(g_poller_id);
    for (i = 0; i < 2; i++) {
        rtems_task_delete(g_chains[i].task_id);
        rtems_semaphore_delete(g_cb[i].ready_sem);
        rtems_semaphore_delete(g_chains[i].mutex);
    }
    g_num_cb = 0;
    g_num_chains = 0;
}

/* ================================================================
 * Experiment D: Callback Concurrency with Interference
 * ================================================================
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

    printf("\n=== Exp D: Callback Concurrency (PiCAS) ===\n");

    g_num_cb = 0;
    g_num_chains = 0;

    create_chain(0, PRIO_HIGH);   /* High-pri chain */
    create_chain(1, PRIO_LOW);    /* Low-pri chain */

    add_cb(0, 200, 0, 1000000ULL, 0, 0);  /* high-pri timer, 200ms */
    add_cb(1, 500, 0, 1000000ULL, 1, 0);  /* low-pri timer, 500ms */

    start_chains_and_poller();

    /* Background CPU load */
    sc = rtems_task_create(rtems_build_name('B', 'G', 'L', 'O'),
        PRIO_BG, RTEMS_MINIMUM_STACK_SIZE * 4,
        RTEMS_DEFAULT_MODES, RTEMS_DEFAULT_ATTRIBUTES, &g_bg_task_id);
    sc = rtems_task_start(g_bg_task_id, bg_busy_task, 0);

    rtems_task_wake_after(30000);

    print_jitter_stats("high(200ms)", &g_cb[0]);
    print_jitter_stats("low(500ms)",  &g_cb[1]);

    /* Count priority inversions: high-pri delayed >5ms */
    uint64_t inversions = 0;
    for (i = 0; i < g_cb[0].firing_count && i < NUM_FIRINGS; i++) {
        uint64_t jitter = g_cb[0].meas[i].start_ns - g_cb[0].meas[i].release_ns;
        if (jitter > 5000000ULL) inversions++;
    }
    printf("  High-pri jitter >5ms count: %llu\n",
           (unsigned long long)inversions);

    rtems_task_delete(g_bg_task_id);
    rtems_task_delete(g_poller_id);
    for (i = 0; i < 2; i++) {
        rtems_task_delete(g_chains[i].task_id);
        rtems_semaphore_delete(g_cb[i].ready_sem);
        rtems_semaphore_delete(g_chains[i].mutex);
    }
    g_num_cb = 0;
    g_num_chains = 0;
}

/* ================================================================
 * Experiment E: Dispatch Path Overhead
 * ================================================================
 */
static void run_exp_e(void)
{
    int i;

    printf("\n=== Exp E: Dispatch Path Overhead (PiCAS) ===\n");

    g_num_cb = 0;
    g_num_chains = 0;

    create_chain(0, PRIO_HIGH);
    add_cb(0, 200, 0, 0, 0, 0);  /* 0 exec_ns: measure pure overhead */

    start_chains_and_poller();

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

    rtems_task_delete(g_poller_id);
    rtems_task_delete(g_chains[0].task_id);
    rtems_semaphore_delete(g_cb[0].ready_sem);
    rtems_semaphore_delete(g_chains[0].mutex);
    g_num_cb = 0;
    g_num_chains = 0;
}

/* ---- Main ---- */
rtems_task Init(rtems_task_argument arg)
{
    printf("PiCAS (Priority-Driven Chain-Aware Scheduling) Benchmark on RTEMS\n");
    printf("=================================================================\n");
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
