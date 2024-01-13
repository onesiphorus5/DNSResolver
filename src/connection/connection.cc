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