#include "DNSMessage.h"
#include "connection.h"
#include <vector>
#include <iostream>

using namespace std;

#define MAX_DOMAIN_NAME_SIZE 1024

void handle_DNSResolver_client( int );

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
   
   size_t buffer_size = MAX_DOMAIN_NAME_SIZE;
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

   /* Check if the domain name is in the cache */
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
   for ( ssize_t i=0; i<DNS_query_size; ++i ) {
      printf("%02hhx", DNS_query[i] );
   }
   cout << endl;

   auto[ resolver_socket, server_addr ] = create_socket_for_nameServer();
   ssize_t sent_cnt;
   sent_cnt = sendto( resolver_socket, DNS_query, DNS_query_size, MSG_CONFIRM,
                      server_addr.ai_addr, server_addr.ai_addrlen );
   if ( sent_cnt == -1 ) {
      perror( "sendto()" );
      exit( EXIT_FAILURE );
   }
   free( DNS_query );

   /* Receive DNS response */
   struct sockaddr_in name_server_addr;
   socklen_t addrlen = sizeof( name_server_addr );
   memset( buffer, 0, buffer_size );
   ssize_t recvd_cnt = recvfrom( resolver_socket, (char *)buffer, buffer_size, 
                                 MSG_WAITALL, 
                                 (struct sockaddr *)&name_server_addr, 
                                 &addrlen );
   if ( recvd_cnt == -1 ) {
      perror( "recvfrom()");
      exit( EXIT_FAILURE );
   }
   for ( ssize_t i=0; i<recvd_cnt; ++i ) {
      printf( "%02hhx", buffer[i] );
   }
   cout << endl;

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
      cout << "answer name: " << answer.get_NAME() << endl;
      answers.push_back( answer );
      offset += bytes_parsed;
   }

   free( buffer );
}