#include "connection.h"

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

std::tuple<int, struct addrinfo> 
create_socket_for_nameServer( bool use_udp ) {
   // TODO: handle TCP also
   int resolver_socket;
   struct addrinfo hints, *serverinfo, *iter;

   memset( &hints, 0, sizeof( hints ) );
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_DGRAM; 

   if ( getaddrinfo( name_server_ip, name_server_port, 
                     &hints, &serverinfo ) != 0 ) {
      perror( "getaddrinfo()" );
      exit( EXIT_FAILURE );
   }
   for ( iter = serverinfo; iter != NULL; iter = iter->ai_next ) {
      if ( (resolver_socket = socket( iter->ai_family, iter->ai_socktype, 
                                      iter->ai_protocol ) ) == -1 ) {
         perror( "socket()" );
         continue;
      }
      break;
   }
   if ( iter == NULL ) {
      fprintf( stderr, "failed to create socket\n" );
      exit( EXIT_FAILURE );
   }

   struct addrinfo addr = *iter;

   freeaddrinfo( serverinfo );

   return { resolver_socket, addr };
}