/**
 * @file timing_globals.cpp
 *
 * Definitions for RTSS instrumentation global variables.
 * These are declared extern in the patched rclcpp headers
 * (subscription_intra_process_buffer.hpp) but are not included
 * in the standard compiled librclcpp.a.
 */

#include <cstdint>

volatile int g_intra_process_published = 0;

volatile int64_t g_rclcpp_gc_trigger_ns = 0;
volatile int64_t g_rclcpp_invoke_cb_ns = 0;
volatile int64_t g_rclcpp_getnext_ns = 0;
volatile int64_t g_rclcpp_take_data_ns = 0;
volatile int64_t g_rclcpp_execute_start_ns = 0;
