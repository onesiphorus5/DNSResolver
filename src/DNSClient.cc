#include "connection.h"

int main(int argc, const char *argv[])
{
   if ( argc < 2 ) {
      std::cout << "You must provide the domain name" << std::endl;
      exit( EXIT_FAILURE );
   }
   const char* domain_name = argv[1];

   /* Create client socket */
   int client_socket;
   client_socket = socket(AF_UNIX, SOCK_STREAM, 0); /* Create client socket */
   if (client_socket == -1) {
      perror( "socket()" );
      exit( EXIT_FAILURE );
   }
   /* Connect to the server */
   struct sockaddr_un DNSResolver_addr;
   memset( &DNSResolver_addr, 0, sizeof(struct sockaddr_un) );
   DNSResolver_addr.sun_family = AF_UNIX;
   strncpy( DNSResolver_addr.sun_path, SOCK_PATH, 
            sizeof( DNSResolver_addr.sun_path ) - 1);
   if (connect( client_socket, (struct sockaddr *) &DNSResolver_addr,
                sizeof(struct sockaddr_un) ) == -1 ) {
      perror( "connect()" );
      exit( EXIT_FAILURE );
   }
   
   /* Send the domain name to the resolver */
   if ( write( client_socket, domain_name, strlen( domain_name )+1 ) 
        != strlen( domain_name )+1 ) {
      perror( "write()" );
      exit( EXIT_FAILURE );
   }

   /* Read the resolved ip address of the domain name from the resolver */
   
   exit(EXIT_SUCCESS);
}