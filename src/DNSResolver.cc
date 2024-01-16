#include "DNSMessage.h"
#include "connection.h"
#include <vector>
#include <iostream>

using namespace std;

#define MAX_DOMAIN_NAME_SIZE 1024

void handle_DNSResolver_client( int, struct sockaddr_un, std::string );

int main() {
   int resolver_socket = setup_socket( RESOLVER_SOCK_PATH );

   ssize_t buffer_size = MAX_DOMAIN_NAME_SIZE;
   char* buffer = (char*) malloc( buffer_size );


   // Handle Resolver client requests
   ssize_t bytes_read;
   for ( ;; ) {
      // int client_socket = accept( resolver_server_socket, NULL, NULL );
      std::string domain_name;

      struct sockaddr_un client_addr;
      socklen_t client_addr_len = sizeof( struct sockaddr_un );
      bytes_read = recvfrom( resolver_socket, buffer, buffer_size, MSG_WAITALL, 
                             (struct sockaddr*)&client_addr, 
                             &client_addr_len );
      if ( bytes_read == -1 ) {
         perror( "recvfrom()" );
         exit( EXIT_FAILURE );
      }

      domain_name += std::string( buffer );
      memset( buffer, 0, buffer_size );

      handle_DNSResolver_client( resolver_socket, client_addr, domain_name );
   }

   free( buffer );

   return 0;
}

void 
handle_DNSResolver_client( int resolver_to_client_socket, 
                           struct sockaddr_un client_addr, 
                           std::string domain_name ) {
   /* TODO: Check if the domain name is in the cache */
   // if domain name is in the cache return the resolved ip

   /* Build DNS message */
   // Build DNS message header section
   srand( time(NULL) );
   // DNSMessage_header_t message_header( rand() );
   DNSMessage_header_t message_header( 22 );
   message_header.set_RD( 1 );         // recursion desired
   message_header.set_QDCOUNT( 1 );    // one question

   // Build DNS message question section
   DNSMessage_question_t question( domain_name );
   question.set_QTYPE( 1 );         // host address
   question.set_QCLASS( 1 );        // Internet

   /* Send DNS query message */
   ssize_t DNS_query_size = message_header.size();
   DNS_query_size += question.size();
   char* DNS_query = (char*) malloc( DNS_query_size );
   memset( DNS_query, 0, DNS_query_size );
   ssize_t pos = 0;

   memcpy( DNS_query, message_header.serialize().c_str(), 
           message_header.size() );
   pos += message_header.size();
   memcpy( DNS_query + pos, question.serialize().c_str(), question.size() );

   auto[ resolver_to_nameServer_socket, server_addr ] = \
                                             create_socket_for_nameServer();
   ssize_t sent_cnt;
   sent_cnt = sendto( resolver_to_nameServer_socket, DNS_query, DNS_query_size, 
                      MSG_CONFIRM,
                      server_addr.ai_addr, server_addr.ai_addrlen );
   if ( sent_cnt == -1 ) {
      perror( "sendto()" );
      exit( EXIT_FAILURE );
   }
   free( DNS_query );

   ssize_t buffer_size = MAX_DOMAIN_NAME_SIZE;
   char* buffer = (char*) malloc( buffer_size );

   /* Receive DNS response */
   struct sockaddr_in name_server_addr;
   socklen_t addrlen = sizeof( name_server_addr );
   memset( buffer, 0, buffer_size );
   ssize_t recvd_cnt = recvfrom( resolver_to_nameServer_socket, (char *)buffer, 
                                 buffer_size, MSG_WAITALL, 
                                 (struct sockaddr *)&name_server_addr, 
                                 &addrlen );
   if ( recvd_cnt == -1 ) {
      perror( "recvfrom()");
      exit( EXIT_FAILURE );
   }

   DNSMessage_header_t response_header = \
                        DNSMessage_header_t::parse_header( buffer );
   ssize_t offset = response_header.size();

   uint16_t question_count = response_header.get_QDCOUNT();
   std::vector<DNSMessage_question_t> questions;
   for ( int i=0; i<question_count; ++i ) {
      auto[ r_question, bytes_parsed ] = \
                  DNSMessage_question_t::parse_question( buffer, offset );
      questions.push_back( r_question );
      offset += bytes_parsed;
   }

   uint16_t answer_count = response_header.get_ANCOUNT();
   std::vector<DNSMessage_rr_t> answers;
   for ( int i=0; i<answer_count; ++i ) {
      auto[ answer, bytes_parsed ] = \
                  DNSMessage_rr_t::parse_resource_record( buffer, offset );
      answers.push_back( answer );
      offset += bytes_parsed;
   }

   /* Send resolved IP addresses to client */
   // 1. Send the number of IP addresses
   ssize_t bytes_sent = sendto( resolver_to_client_socket, &answer_count, 
                                sizeof( answer_count ), MSG_CONFIRM,
                                (const struct sockaddr*)&client_addr, 
                                sizeof( struct sockaddr_un) );
   if ( bytes_sent != sizeof( answer_count ) ) {
      perror( "write()" );
      exit( EXIT_FAILURE );
   }
   // 2. Send the IP addresses
   for ( ssize_t i=0; i<answers.size(); ++i ) {
      uint32_t IP_addr = *(uint32_t*)(answers[i].get_RDATA().c_str());

      bytes_sent = sendto( resolver_to_client_socket, &IP_addr, 
                           sizeof( IP_addr ), MSG_CONFIRM,
                           (const struct sockaddr*)&client_addr, 
                           sizeof( struct sockaddr_un) );
      if ( bytes_sent != sizeof( IP_addr ) ) {
         perror( "write()" );
         exit( EXIT_FAILURE );
      }
   }

   free( buffer );
}