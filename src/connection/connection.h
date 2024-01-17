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

char RESOLVER_SOCK_PATH[] = "/tmp/DNSResolver";
char CLIENT_SOCK_PATH[] = "/tmp/DNSClient";
// TODO: automate this part by:
// 1. downloading https://www.internic.net/domain/named.root
// 2. parsing the file to get an IP address of one of the root servers.
const char* root_server_ip   = "198.41.0.4";
const char* DNS_port = "53";

struct sockaddr_un unixDomain_addr( const char*, bool remove_path );
int setup_socket( const char* );
std::tuple<int, struct addrinfo> create_socket_for_nameServer( std::string );