#include <thread>

#include <rtems.h>
#include <sys/event.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

int main(int argc, char * argv[]);

void test_main(){
    char arg0[] = "timer_manager";
    char *argv_timer[] = { arg0, nullptr };
    main(1, argv_timer);
}

#define CONFIGURE_MICROSECONDS_PER_TICK    1000  /* 1ms/tick */

#define DEFAULT_NETWORK_DHCPCD_NO_DHCP_DISCOVERY
#define DEFAULT_NETWORK_SHELL
#define DEFAULT_NETWORK_NO_STATIC_IFCONFIG
#include "default-network-init.h"
