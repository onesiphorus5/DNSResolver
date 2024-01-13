#include "connection.h"
#include "DNSMessage.h"

#include <iostream>
using namespace std;

int 
setup_server() {
   int domain   = AF_UNIX;
   int type = SOCK_STREAM;
   int protocol = 0;

   // Create server side socket
   int server_socket;
   if ( ( server_socket = socket( domain, type, protocol ) ) < 0 ) {
      perror( "socket() failed" );
      exit( EXIT_FAILURE );
   }

   // Bind the server socket to the server addr and port
   struct sockaddr_un server_addr;

   if ( remove(SOCK_PATH) == -1 && errno != ENOENT ) {
      perror( "remove() failed" );
      exit( EXIT_FAILURE );
   }
   memset( &server_addr, 0, sizeof( struct sockaddr_un ) );
   server_addr.sun_family = domain;
   strncpy( server_addr.sun_path, SOCK_PATH, sizeof( server_addr.sun_path ) - 1 );

   if ( bind( server_socket, (struct sockaddr *) &server_addr, 
              sizeof( sockaddr_un ) ) < 0 ) {
      perror( "bind() failed" );
      exit( EXIT_FAILURE );
   }

   // Mark the server socket as the listening side
   int backlog = 10;
   if ( listen( server_socket, backlog ) < 0 ) {
      perror( "listen() failed" );
      exit( EXIT_FAILURE );
   }

   return server_socket;
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