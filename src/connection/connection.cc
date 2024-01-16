#include "connection.h"

#include <iostream>
using namespace std;

struct sockaddr_un
unixDomain_addr( const char* socket_path, bool remove_path ) {
   int domain = AF_UNIX;

   struct sockaddr_un addr;
   if ( remove_path ) {
      if ( remove( socket_path ) == -1 && errno != ENOENT ) {
         perror( "remove() failed" );
         exit( EXIT_FAILURE );
      }
   }
   memset( &addr, 0, sizeof( struct sockaddr_un ) );
   addr.sun_family = domain;
   strncpy( addr.sun_path, socket_path, sizeof( addr.sun_path ) - 1 );

   return addr;
}

int 
setup_socket( const char* socket_path ) {
   int domain   = AF_UNIX;
   int type = SOCK_DGRAM;
   int protocol = 0;

   /* Create socket */
   int unix_socket;
   if ( ( unix_socket = socket( domain, type, protocol ) ) < 0 ) {
      perror( "socket() failed" );
      exit( EXIT_FAILURE );
   }

   /* Bind socket */
   struct sockaddr_un resolver_addr = unixDomain_addr( socket_path, true );
   if ( bind( unix_socket, (struct sockaddr *) &resolver_addr, 
              sizeof( struct sockaddr_un ) ) < 0 ) {
      perror( "bind() failed" );
      exit( EXIT_FAILURE );
   }

   return unix_socket;
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