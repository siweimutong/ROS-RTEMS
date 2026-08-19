# rclcpp-rtems

| | |
|---|---|
| **开发人员** | siweimutong |
| **联系方式** | luoxiantong@bjut.edu.cn |

Forked from rclcpp-rtss25.  Adds DDS / inter-process notification support
to RTExecutor so that subscription callbacks are kernel-driven for BOTH
intra-process and DDS paths.

## Changes from rclcpp-rtss25

### 1. `subscription_base.hpp` — added `get_on_new_message_callback()`

Public getter for the DDS-level `on_new_message_callback_`.  Enables
RTExecutor to wrap the existing DDS callback with an `rtems_event_send`.

### 2. `rt_executor.cpp` — DDS notification in `register_rt_subscription()`

After setting the intra-process `direct_notify_callback_`, also wraps
`on_new_message_callback_` so that when a DDS message arrives, the
corresponding CallbackTask is woken via `rtems_event_send`.

#### Notification paths (after modification)

| Path | Condition | Mechanism |
|------|-----------|-----------|
| Intra-process | `use_intra_process_comms(true)` | `direct_notify_callback_ → rtems_event_send` |
| DDS/inter-process | `use_intra_process_comms(false)` | `on_new_message_callback_ wrapper → rtems_event_send` |

Both paths converge on the same CallbackTask: `rtems_event_receive() → callback() → take + handle`.
