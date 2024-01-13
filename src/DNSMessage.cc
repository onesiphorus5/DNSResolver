#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include "DNSMessage.h"

#include <iostream>
using namespace std;

std::string
DNSMessage_header_t::serialize() {
   std::string header_bytes;

   header_bytes += (char) ( ID >> 8 );
   header_bytes += (char) ID;

   uint16_t codes = RCODE;
   codes = codes | ( Z << 4 );
   codes = codes | ( RA << 7 );
   codes = codes | ( RD << 8 );
   codes = codes | ( TC << 9 );
   codes = codes | ( AA << 10 );
   codes = codes | ( OPCODE << 11 );
   codes = codes | ( QR << 15 );
   header_bytes += (char) ( codes >> 8 );
   header_bytes += (char) codes;

   header_bytes += (char) ( QDCOUNT >> 8 );
   header_bytes += (char) QDCOUNT;

   header_bytes += (char) ( ANCOUNT >> 8 );
   header_bytes += (char) ANCOUNT;

   header_bytes += (char) ( NSCOUNT  >> 8 );
   header_bytes += (char) NSCOUNT;

   header_bytes += (char) ( ARCOUNT >> 8 );
   header_bytes += (char) ARCOUNT;

   return header_bytes;
}

// It is assumed domain names passed to the function have 
// the following format:
// "<nth-level domain>...<third-level domain>.<second-level domain>.<top-level domain>"
std::string
DNSMessage_question_t::encode_domain_name() {
   std::string QNAME;

   std::string delimiter = ".";
   size_t left_pos=0, right_pos;
   size_t label_size;
   while ( ( right_pos = domain_name.find( delimiter, left_pos ) ) 
           != std::string::npos ) {
       label_size = right_pos - left_pos;
       // label_size fits within a char
       QNAME +=  (char) label_size;
       QNAME += domain_name.substr( left_pos, label_size );

       left_pos = right_pos+1;
   }
   label_size = domain_name.size() - left_pos;
   // label_size fits within a char
   QNAME += (char) label_size;
   QNAME += domain_name.substr( left_pos, label_size );

   // NULL character to mark the end
   QNAME += (char) 0;

   return QNAME;
}

std::string
DNSMessage_question_t::serialize() {
   // Question
   /*
   memcpy( DNS_query + pos, question.QNAME.c_str(), question.QNAME.size() );
   pos += question.QNAME.size();
   memcpy( DNS_query + pos, &question.QTYPE, sizeof( question.QTYPE ) );
   pos += sizeof( question.QTYPE );
   memcpy( DNS_query + pos, &question.QCLASS, sizeof( question.QCLASS ) );
   pos += sizeof( question.QCLASS );
   */

  std::string question_bytes;
  question_bytes += encode_domain_name();

  question_bytes += (char) ( QTYPE >> 8 );
  question_bytes += (char) QTYPE;

  question_bytes += (char) ( QCLASS >> 8 );
  question_bytes += (char) QCLASS;

  return question_bytes;
}

std::string
DNSMessage_rr_t::serialize() {

}

void
send_DNS_query( std::string domain_name ) {
   /* Build DNS message header section */
   srand( time(NULL) );
   // DNSMessage_header_t message_header( rand() );
   DNSMessage_header_t message_header( 22 );
   message_header.set_RD( 1 );
   message_header.set_QDCOUNT( 1 );

   std::string header_bytes = message_header.serialize();
   for ( ssize_t i=0; i<header_bytes.size(); ++i ) {
      printf("%02hhx", header_bytes[i] );
   }
   cout << endl;

   /* Build DNS message question section */
   DNSMessage_question_t question( domain_name );
   question.set_QTYPE( 1 );
   question.set_QCLASS( 1 );

   std::string question_bytes = question.serialize();

   /* Put the sections together */
   ssize_t DNS_query_size = 0;
   DNS_query_size += message_header.size();
   DNS_query_size += question.size();
  
   char* DNS_query = (char*) malloc( DNS_query_size );
   memset( DNS_query, 0, DNS_query_size );
   ssize_t pos     = 0;
   // Header
   memcpy( DNS_query + pos, header_bytes.c_str(), header_bytes.size() );
   pos += header_bytes.size();

   // Question
   memcpy( DNS_query + pos, question_bytes.c_str(), question_bytes.size() );
   pos += question_bytes.size();

   // Send the query to Google's DNS server
   // IP: 8.8.8.8, port: 53
   // TODO: make this part generic
   const char* name_server_ip   = "8.8.8.8";
   const char* name_server_port = "53";

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

   // Send DNS query
   /* For debugging
   for ( ssize_t i=0; i<DNS_query_size; ++i ) {
      printf("%02hhx", DNS_query[i] );
   }
   */ 
   ssize_t bytes_cnt;
   bytes_cnt = sendto( resolver_socket, DNS_query, DNS_query_size, MSG_CONFIRM, 
                       iter->ai_addr, iter->ai_addrlen );
   if ( bytes_cnt == -1 ) {
      perror( "sendto()" );
      exit( EXIT_FAILURE );
   }

   // Receive DNS response
   struct sockaddr_in name_server_addr;
   socklen_t addrlen = sizeof( name_server_addr );
   char buffer[1024];
   memset( buffer, 0, sizeof(buffer) );
   bytes_cnt = recvfrom( resolver_socket, (char *)buffer, sizeof( buffer ), 
                         MSG_WAITALL, (struct sockaddr *)&name_server_addr, 
                         &addrlen );
   if ( bytes_cnt == -1 ) {
      perror( "recvfrom()");
      exit( EXIT_FAILURE );
   }
   cout << "received bytes_cnt: " << bytes_cnt << endl;
}                                                  