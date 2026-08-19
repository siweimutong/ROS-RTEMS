#include <cstdio>

int main(int argc, char * argv[]);

void test_main()
{
  char *argv_default[] = {(char *)"exp3inter_picas", nullptr};
  main(1, argv_default);
}

#define DEFAULT_NETWORK_DHCPCD_NO_DHCP_DISCOVERY
#define DEFAULT_NETWORK_SHELL
#define DEFAULT_NETWORK_NO_STATIC_IFCONFIG

#define CONFIGURE_MICROSECONDS_PER_TICK    1000
#define CONFIGURE_MAXIMUM_DRIVERS          10

#include "default-network-init.h"
