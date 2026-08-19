/*
 * RTEMS configuration for RT-ROS (EventsCBGExecutor) benchmark
 *
 * This app replicates the RT-ROS scheduling model on RTEMS:
 * - Event-driven callback dispatch (no rcl_wait spin loop)
 * - CallbackGroup-based scheduling with FIFO policy
 * - TimerManager for independent timer expiration detection
 * - Callbacks execute in dedicated threads but share CBG mutexes
 */

/* Drivers */
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER

/* 1 ms tick for precise timing */
#define CONFIGURE_MICROSECONDS_PER_TICK 1000

/* Allow dynamic object creation */
#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

/* Timer Server task is created by rtems_timer_initiate_server */
#define CONFIGURE_TIMER_SERVER_FOR_TIMERS

/* We need multiple tasks for the EventsCBG model */
#define CONFIGURE_MAXIMUM_TASKS 20
#define CONFIGURE_MAXIMUM_TIMERS 10
#define CONFIGURE_MAXIMUM_SEMAPHORES 10
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 10

/* Init task */
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT

#include <rtems/confdefs.h>
