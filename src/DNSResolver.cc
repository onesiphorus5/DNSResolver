#include "DNSMessage.h"
#include "connection.h"
#include <stack>
#include <vector>
#include <iostream>

using namespace std;

#define MAX_DOMAIN_NAME_SIZE  256 // Technically it's 253 bytes
#define MAX_DNS_RESPONSE_SIZE 512 // for UDP packets 

void handle_DNSResolver_client( int, struct sockaddr_un, std::string );
std::tuple<char*, ssize_t> dns_request( std::string, std::string );
std::vector<uint32_t> iteratively_resolve( std::string qname );

// std::vector<uint32_t> resolve_domain_name( std::string );

int main() {
   // Initialize root_server_ips
   // https://www.internic.net/domain/named.root
   root_server_ips = {
      "198.41.0.4",
      "170.247.170.2",
      "192.33.4.12",
      "199.7.91.13",
      "192.203.230.10",
      "192.5.5.241",
      "192.112.36.4",
      "198.97.190.53",
      "192.36.148.17",
      "192.58.128.30",
      "193.0.14.129",
      "199.7.83.42",
      "202.12.27.33"
   };

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

   /* Resolve domain name */
   std::string qname = encode_domain_name( domain_name );
   std::vector<uint32_t> IP_addrs = iteratively_resolve( qname );

   /* Send resolved IP addresses to client */
   // 1. Send the number of IP addresses
   uint16_t answer_count = (uint16_t)IP_addrs.size();
   ssize_t bytes_sent = sendto( resolver_to_client_socket, &answer_count, 
                                sizeof( answer_count ), MSG_CONFIRM,
                                (const struct sockaddr*)&client_addr, 
                                sizeof( struct sockaddr_un) );
   if ( bytes_sent != sizeof( answer_count ) ) {
      perror( "write()" );
      exit( EXIT_FAILURE );
   }
   // 2. Send the IP addresses
   for ( ssize_t i=0; i<IP_addrs.size(); ++i ) {
      uint32_t IP_addr = IP_addrs[i];

      bytes_sent = sendto( resolver_to_client_socket, &IP_addr, 
                           sizeof( IP_addr ), MSG_CONFIRM,
                           (const struct sockaddr*)&client_addr, 
                           sizeof( struct sockaddr_un) );
      if ( bytes_sent != sizeof( IP_addr ) ) {
         perror( "write()" );
         exit( EXIT_FAILURE );
      }
   }

}


std::tuple<char*, ssize_t> 
dns_request( std::string addr, std::string QNAME ) {
   /* Build DNS message */
   // Build DNS message header section
   srand( time(NULL) );
   DNSMessage_header_t message_header;
   message_header.set_ID( rand() );
   message_header.set_QDCOUNT( 1 );    // one question

   // Build DNS message question section
   DNSMessage_question_t question( QNAME );
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
                                 create_socket_for_nameServer( addr );
   ssize_t sent_cnt;
   sent_cnt = sendto( resolver_to_nameServer_socket, DNS_query, DNS_query_size, 
                      MSG_CONFIRM,
                      server_addr.ai_addr, server_addr.ai_addrlen );
   if ( sent_cnt == -1 ) {
      perror( "sendto()" );
      exit( EXIT_FAILURE );
   }
   free( DNS_query );

   /* Receive DNS response */
   ssize_t buffer_size = MAX_DNS_RESPONSE_SIZE;
   char* buffer = (char*)malloc( buffer_size );

   ssize_t total_buffer_size = buffer_size;
   char* total_buffer = (char*)malloc( total_buffer_size );
   ssize_t offset = 0;

   ssize_t recvd_cnt;
   struct sockaddr_in name_server_addr;
   socklen_t addrlen = sizeof( name_server_addr );
   for ( ; ; ) {
      memset( buffer, 0, buffer_size );
      recvd_cnt = recvfrom( resolver_to_nameServer_socket, buffer, buffer_size, 
                            MSG_WAITALL, 
                            (struct sockaddr *)&name_server_addr, &addrlen );
      if ( recvd_cnt == -1 ) {
         perror( "recvfrom()" );
         exit( EXIT_FAILURE );
      }

      if ( ( total_buffer_size -  offset ) < recvd_cnt ) {
         total_buffer_size += MAX_DNS_RESPONSE_SIZE;
         total_buffer = (char*)realloc( total_buffer, total_buffer_size );
      }
      memcpy( total_buffer + offset, buffer, recvd_cnt );
      offset += recvd_cnt;

      if ( recvd_cnt < buffer_size ) {
         break;
      }
   }

   return {total_buffer, offset};
}

struct domain_request {
   std::string qname;
   std::vector<uint32_t> server_addrs;

   domain_request( std::string name, std::vector<uint32_t> addrs ) : 
      qname{ name }, server_addrs{ addrs } {}
};

std::vector<uint32_t>
iteratively_resolve( std::string qname ) {
   std::stack<domain_request> request_stack;
   request_stack.push(domain_request( qname, {} ) );

   srand( time( NULL ) );

   std::vector<uint32_t> resolved_IPs;

   int iter_count = 0;
   while ( !request_stack.empty() ) {
      iter_count++;

      domain_request request = request_stack.top();
      std::string server_addr;
      if ( request.server_addrs.empty() ) {
         server_addr = root_server_ips[ rand() % root_server_ips.size() ];
      } else {
         ssize_t addrs_count = request.server_addrs.size();
         // Pick a random address to use.
         server_addr = \
            to_IPAddr_str( request.server_addrs[ rand() % addrs_count ] );
      }

      ssize_t offset = 0;
      auto[reply_buffer, reply_buffer_size] = \
                     dns_request( server_addr, request.qname );

      // Header section
      DNSMessage_header_t reply_header( reply_buffer, reply_buffer_size );
      offset += reply_header.size();

      // Question section
      DNSMessage_question questions( &reply_header );
      ssize_t bytes_read = questions.parse_questions( offset );
      offset += bytes_read;

      // Answer section
      DNSMessage_rr answers( &reply_header );
      bytes_read = answers.parse_records( offset, RecordType::AN );
      offset += bytes_read;

      // Authoritative name server section
      DNSMessage_rr ns_records( &reply_header );
      bytes_read = ns_records.parse_records( offset, RecordType::NS );
      offset += bytes_read;

      // Additional records section
      DNSMessage_rr add_records( &reply_header );
      bytes_read = add_records.parse_records( offset, RecordType::AR );

      // Processing the DNS reply sections
      std::vector<std::string> cname_aliases;
      std::vector<uint32_t> ans_IPs;
      for ( auto answer : answers.get_records() ) {
         if ( answer.get_TYPE() == 1 ) { 
            uint32_t ans_addr = *(uint32_t*)answer.get_RDATA().c_str();
            ans_IPs.push_back( ans_addr );
         } else if ( answer.get_TYPE() == 28 ) {
            cout << "Only IPv4 is being handled" << endl;
         } else if ( answer.get_TYPE() == 5 ) {
            cname_aliases.push_back( answer.get_RDATA() );
         } else {
            cout << "Malformed reply!" << endl;
            exit( EXIT_FAILURE );
         }
      }
      // Case 1: the reply to the original request has an ANS section 
      //         with type1 records
      if ( request_stack.size() == 1 ) {
         if ( ans_IPs.size() > 0 ) {
            resolved_IPs = ans_IPs;
            break;
         }
      }
      // Case 2: the reply has an ANS section with type1 records, 
      //         but it's not for the original request
      if ( ans_IPs.size() > 0 ) {
         request_stack.pop();
         domain_request& top_request = request_stack.top();
         top_request.server_addrs = ans_IPs;
         continue;
      }
      // Case 3: the reply has an ANS section with type5 record(s)
      if ( cname_aliases.size() > 0 ) {
         domain_request& top_request = request_stack.top();
         top_request.qname = cname_aliases[ rand() % cname_aliases.size() ];
         top_request.server_addrs = {};
      }

      std::vector<uint32_t> ns_addrs;
      for ( auto record : ns_records.get_records() ) {
         std::string ns_domain = record.get_RDATA();
         if ( add_records.has_record( ns_domain ) ) {
            uint32_t ns_addr = *(uint32_t*)add_records.\
                     get_record( ns_domain ).get_RDATA().c_str();
            ns_addrs.push_back( ns_addr );
         }
      }
      if ( ns_addrs.size() > 0 ) {
         domain_request& top_request = request_stack.top();
         top_request.server_addrs = ns_addrs;
      } else if ( ns_records.size() > 0 ) {
         ssize_t ns_size = ns_records.size();
         std::string ns_domain = \
                     ns_records.get_records()[ rand() % ns_size ].get_RDATA();
         auto ns_request = domain_request( ns_domain, {} );
         request_stack.push( ns_request );
      }
   }

   return resolved_IPs;
}