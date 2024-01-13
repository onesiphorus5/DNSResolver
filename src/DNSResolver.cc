#include "DNSMessage.h"
#include "connection.h"

int main() {
   int resolver_server_socket = setup_server();
   // Wait for DNS resolver clients to connect
   for ( ;; ) {
      int client_socket = accept( resolver_server_socket, NULL, NULL );
      if ( client_socket < 0 ) {
         perror( "accept failed" );
         exit( EXIT_FAILURE );
      }

      handle_DNSResolver_client( client_socket );
      close( client_socket );
   }

   return 0;
}

void 
handle_DNSResolver_client( int client_socket ) {
   /* Read 'domain name' from client */
   std::string domain_name;
   
   size_t buffer_size = 1024;
   char* buffer = (char*) malloc( buffer_size );
   ssize_t read_bytes;
   for ( ; ; ) { 
      read_bytes = read( client_socket, buffer, buffer_size );
      if ( read_bytes < 0 ) {
         perror( "read()" );
         exit( EXIT_FAILURE );
      }
      if ( read_bytes == 0 ) {
         break;
      }
      domain_name += std::string( buffer );
      memset( buffer, 0, buffer_size );
   }
   free( buffer );

   /* Check if the domain name is in the cache */
   // if domain name is in the cache return the resolved ip

   /* Build DNS message */
   // We are here!!
   // DNSMessage DNS_message = build_DNS_message( domain_name );

   /* Send DNS message */
   send_DNS_query( domain_name );

   /* Send resolved ip address to the client */
   /*
   ssize_t ip_size = resolved_ip.size() + 1;
   if ( write( client_socket, resolved_ip.c_str(), ip_size ) 
        != ip_size ) {
      perror( "write()" );
      exit( EXIT_FAILURE );
   }
   */
}