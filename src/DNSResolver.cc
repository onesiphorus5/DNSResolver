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