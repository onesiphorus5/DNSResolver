#include "connection.h"

int main(int argc, char *argv[])
{
   struct sockaddr_un DNSResolver_addr;
   int client_socket;

   client_socket = socket(AF_UNIX, SOCK_STREAM, 0); /* Create client socket */
   if (client_socket == -1) {
      perror( "socket()" );
      exit( EXIT_FAILURE );
   }
   /* Connect to the server */
   memset( &DNSResolver_addr, 0, sizeof(struct sockaddr_un) );
   DNSResolver_addr.sun_family = AF_UNIX;
   strncpy( DNSResolver_addr.sun_path, SOCK_PATH, 
            sizeof( DNSResolver_addr.sun_path ) - 1);
   if (connect( client_socket, (struct sockaddr *) &DNSResolver_addr,
                sizeof(struct sockaddr_un) ) == -1 ) {
      perror( "connect()" );
      exit( EXIT_FAILURE );
   }
   
   // Write to the server
   char message[] = "Hello, I'm client";
   if ( write( client_socket, message, sizeof( message ) ) 
        != sizeof( message ) ) {
      perror( "write()" );
      exit( EXIT_FAILURE );
   }

   exit(EXIT_SUCCESS);
}