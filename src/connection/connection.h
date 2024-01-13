// #include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <string>
#include <iostream>

char SOCK_PATH[] = "/tmp/DNSResolver";

int setup_server();
void handle_DNSResolver_client( int );