#include <cstdio>
#include <rtems.h>

extern "C" {
#include "driver/rtss_timer_driver.h"
}

int main(int argc, char * argv[]);

extern "C" {
extern rtems_driver_address_table rtss_timer_driver_table;
}

void test_main()
{
  char *argv_default[] = {(char *)"exp3inter_ours", nullptr};

  /* Register timer driver before rclcpp::init */
  rtems_device_major_number rtss_major;
  rtems_status_code sc = rtems_io_register_driver(0, &rtss_timer_driver_table, &rtss_major);
  if (sc == RTEMS_SUCCESSFUL) {
    rtems_io_register_name(RTSS_DEVICE_NAME, rtss_major, 0);
  }

  main(1, argv_default);
}

#define DEFAULT_NETWORK_DHCPCD_NO_DHCP_DISCOVERY
#define DEFAULT_NETWORK_SHELL
#define DEFAULT_NETWORK_NO_STATIC_IFCONFIG

#define CONFIGURE_MICROSECONDS_PER_TICK    1000
#define CONFIGURE_MAXIMUM_DRIVERS          10

#include "default-network-init.h"
