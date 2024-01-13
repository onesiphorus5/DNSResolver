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
   return "";
}