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

int sub_main(int argc, char * argv[]);
int pub_main(int argc, char * argv[]);


void test_main(){
    char *argv_sub[]={(char*)"sub_main", (char*)"subscriber"};
    // sub_main(1, argv_sub);
    std::thread thread_sub(sub_main, 1, argv_sub);
    
    char *argv_pub[]={(char*)"test_main"};
    std::thread thread_pub(pub_main, 1, argv_pub);

    thread_sub.join();
    thread_pub.join();
}

#define DEFAULT_NETWORK_DHCPCD_NO_DHCP_DISCOVERY
#define DEFAULT_NETWORK_SHELL

#define DEFAULT_NETWORK_NO_STATIC_IFCONFIG
#include "default-network-init.h"