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
#include <tuple>

char SOCK_PATH[] = "/tmp/DNSResolver";
const char* name_server_ip   = "8.8.8.8";
const char* name_server_port = "53";

int setup_server();
std::tuple<int, struct addrinfo> create_socket_for_nameServer( bool use_udp = true );