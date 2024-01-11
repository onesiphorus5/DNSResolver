#include <string>
#include <iostream>

       #include <sys/socket.h>
       #include <netinet/in.h>
       #include <arpa/inet.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "DNSMessage.h"
#include "connection.h"

using namespace std;

TEST_CASE( "DNSMessage" ) {
    SUBCASE( "DNSMessage header" ) {
        DNSMessage_header_t header;
        CHECK( sizeof( header ) == 12 );
    }

    SUBCASE( "encode_domain_name()" ) {
        std::string domain_name = "www.google.com";
        std::string expected_encoding = "3www6google3com";
        CHECK( encode_domain_name( domain_name ) == expected_encoding );       
    }

    SUBCASE( "get_local_ipAddr()" ) {
        char peer_addr_str[ INET_ADDRSTRLEN ];
        auto local_ip = get_local_ipAddr();
        cout << inet_ntop( AF_INET, &local_ip, peer_addr_str, INET_ADDRSTRLEN ) << endl;
    }
}
