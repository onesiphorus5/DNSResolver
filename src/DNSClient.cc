#include <vector>
#include <unistd.h>
#include "connection.h"
#include "DNSMessage.h"

int main(int argc, const char *argv[])
{
   if ( argc < 2 ) {
      std::cout << "You must provide the domain name" << std::endl;
      exit( EXIT_FAILURE );
   }
   const char* domain_name = argv[1];

   /* Create client socket */
   int client_socket;
   client_socket = setup_socket( CLIENT_SOCK_PATH );

   /* Send domain name to the resolver process */
   sockaddr_un resolver_addr = unixDomain_addr( RESOLVER_SOCK_PATH,
                                                       false );
   ssize_t bytes_sent = sendto( client_socket, domain_name, 
                                strlen(domain_name) + 1, MSG_CONFIRM, 
                                (struct sockaddr*)&resolver_addr, 
                                sizeof( struct sockaddr_un ) );
   if ( bytes_sent == -1 ) {
      perror( "sendto()" );
      exit( EXIT_FAILURE );
   }

   /* Receive resolved IP addresses of the domain name */
   // 1. Receive the number of resolved IP addresses
   uint16_t IP_count;
   struct sockaddr_un addr;
   socklen_t addr_len = sizeof( struct sockaddr_un );
   ssize_t bytes_recvd = recvfrom( client_socket, &IP_count, sizeof( IP_count ), 
                                   MSG_WAITALL, 
                                   (struct sockaddr*)&addr, &addr_len );
   if ( bytes_recvd == -1 ) {
      perror( "recvfrom()" );
      exit( EXIT_FAILURE );
   }
   std::cout << "IP count: " << IP_count << std::endl;

   // 2. Receive those IP addresses
   std::vector<uint32_t> IP_addrs;
   for ( uint16_t i=0; i<IP_count; ++i ) {
      uint32_t IP_addr;
      bytes_recvd = recvfrom( client_socket, &IP_addr, sizeof( IP_addr ), 
                              MSG_WAITALL, 
                              (struct sockaddr*)&addr, &addr_len );
      if ( bytes_recvd == -1 ) {
         perror( "recvfrom()" );
         exit( EXIT_FAILURE );
      }
      IP_addr = ntohl( IP_addr );
      std::cout << DNSMessage_rr_t::hl_to_IPAddr( IP_addr ) << std::endl;
   }

   exit(EXIT_SUCCESS);
}