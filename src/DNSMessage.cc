#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
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

DNSMessage_header_t
DNSMessage_header_t::parse_header( const char* buffer ) {
   /* ID */
   uint16_t id = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( id );

   /* codes */
   uint16_t codes = ntohs( *(uint16_t*)buffer );
   buffer += CODES_SIZE;

   /* QDCOUNT */
   uint16_t qdcount = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( qdcount );

   /* ANCOUNT */
   uint16_t ancount = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( ancount );

   /* NSCOUNT */
   uint16_t nscount = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( nscount );

   /* ARCOUNT */
   uint16_t arcount = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( arcount );

   DNSMessage_header_t header;
   
   header.ID     =  id;
   header.QR     = ( codes & ( 0b1 << 15 ) ) >> 15;
   header.OPCODE = ( codes & ( 0b1111 << 11 ) ) >> 11;
   header.AA     = ( codes & ( 0b1 << 10 ) ) >> 10;
   header.TC     = ( codes & ( 0b1 << 9 ) ) >> 9;
   header.RD     = ( codes & ( 0b1 << 8 ) ) >> 8;
   header.RA     = ( codes & ( 0b1 << 7 ) ) >> 7;
   header.Z      = ( codes & ( 0b111 << 4 ) ) >> 4;
   header.RCODE  = codes & ( 0b1111 );

   header.QDCOUNT = qdcount;
   header.ANCOUNT = ancount;
   header.NSCOUNT = nscount;
   header.ARCOUNT = arcount;

   return header;
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
  std::string question_bytes;
  question_bytes += encode_domain_name();

  question_bytes += (char) ( QTYPE >> 8 );
  question_bytes += (char) QTYPE;

  question_bytes += (char) ( QCLASS >> 8 );
  question_bytes += (char) QCLASS;

  return question_bytes;
}

std::tuple<DNSMessage_question_t, ssize_t>
DNSMessage_question_t::parse_question( const char* buffer, ssize_t question_offset ) {
   const char* buffer_cpy = buffer;
   buffer_cpy += question_offset;
   uint8_t label_size;

   ssize_t bytes_read = 0;
   ssize_t compressed_bytes_read = 0;

   DNSMessage_question_t question;

   /* QNAME */
   std::string QNAME;
   for ( ; ; ) {
      label_size = *buffer_cpy;
      buffer_cpy += 1;
      bytes_read += 1;

      if ( label_size == (char)0 ) {
         break;
      }

      if ( ( label_size >> 6 ) == 3 ) { // Handle compression
         uint16_t new_offset = label_size << 2;
         new_offset = label_size >> 2;
         new_offset = new_offset << 8;
         new_offset = new_offset | *buffer_cpy;

         buffer_cpy = buffer + new_offset;
         bytes_read += 1;
         compressed_bytes_read = bytes_read;
      } else if ( label_size <= 63 ) {
         QNAME += label_size;
         for ( int i=0; i<label_size; ++i ) {
            QNAME += *buffer_cpy;
            buffer_cpy += 1;
            bytes_read += 1;
         }
      } else {
         // TODO: notify the caller of a malformed question
         cout << "Malformed question" << endl;
         break;
      }
   }
   if ( compressed_bytes_read != 0 ) {
      bytes_read = compressed_bytes_read;
   }

   /* QTYPE */
   buffer_cpy = buffer + question_offset + bytes_read;
   uint16_t QTYPE = ntohs( *(uint16_t*)buffer_cpy );
   buffer_cpy += sizeof( QTYPE );
   bytes_read += sizeof( QTYPE );

   /* QCLASS */
   uint16_t QCLASS = ntohs( *(uint16_t*)buffer_cpy );
   bytes_read += sizeof( QTYPE );

   question.set_QNAME( QNAME );
   question.set_QTYPE( QTYPE );
   question.set_QCLASS( QCLASS );

   return { question, bytes_read };
}

std::tuple<DNSMessage_rr_t, ssize_t>
DNSMessage_rr_t::parse_resource_record( const char* buffer, ssize_t record_offset ) {
   const char* buffer_cpy = buffer;
   buffer_cpy += record_offset;
   uint8_t label_size;

   ssize_t bytes_read = 0;
   ssize_t compressed_bytes_read = 0;

   DNSMessage_rr_t record;

   /* NAME */
   // TODO: the code handling compression has a bug, when double compression 
   //       is encountered.
   std::string NAME;
   cout << "record_offset: " << record_offset << endl;
   for ( ; ; ) {
      label_size = *buffer_cpy;
      buffer_cpy += 1;
      bytes_read += 1;

      if ( label_size == (char)0 ) {
         break;
      }

      cout << "label size: " << (int)label_size << endl;
      if ( ( label_size >> 6 ) == 3 ) { // Handle compression
         label_size = label_size << 2;
         uint16_t new_offset = label_size >> 2;
         cout << "new offset: " << new_offset << endl;
         new_offset = new_offset << 8;
         printf( "the char: %02hhX\n", (char)*buffer_cpy );
         // cout << "new offset: " << (char)*buffer_cpy << endl;
         uint16_t the_char = (uint16_t)*buffer_cpy;
         the_char = the_char << 8;
         the_char = the_char >> 8;
         // new_offset = new_offset | (uint16_t)*buffer_cpy;
         new_offset = new_offset | the_char;
         cout << "new offset: " << new_offset << endl;

         buffer_cpy = buffer + new_offset;
         bytes_read += 1;
         compressed_bytes_read = bytes_read = sizeof( new_offset );
         // TODO: add assert( compressed_bytes_read == 2 )
      } else if ( label_size <= 63 ) {
         NAME += label_size;
         for ( int i=0; i<label_size; ++i ) {
            NAME += *buffer_cpy;
            buffer_cpy += 1;
            bytes_read += 1;
         }
      } else {
         // TODO: notify the caller of a malformed question
         cout << "Malformed question2" << endl;
         break;
      }
   }
   if ( compressed_bytes_read != 0 ) {
      bytes_read = compressed_bytes_read;
   }

   buffer_cpy = buffer + record_offset + bytes_read;

   /* TYPE */
   uint16_t TYPE = ntohs( *(uint16_t*)buffer_cpy );
   buffer_cpy += sizeof( TYPE );
   bytes_read += sizeof( TYPE );

   /* CLASS */
   uint16_t CLASS = ntohs( *(uint16_t*)buffer_cpy );
   buffer_cpy += sizeof( CLASS );
   bytes_read += sizeof( CLASS );

   /* TTL */
   uint32_t TTL = ntohl( *(uint32_t*)buffer_cpy );
   buffer_cpy += sizeof( TTL );
   bytes_read += sizeof( TTL );

   /* RDLENGTH */
   uint16_t RDLENGTH = ntohs( *(uint16_t*)buffer_cpy );
   buffer_cpy += sizeof( RDLENGTH );
   bytes_read += sizeof( RDLENGTH );

   /* RDATA */
   std::string RDATA;
   for ( uint16_t i=0; i<RDLENGTH; ++i ) {
      RDATA += *( buffer_cpy + i );
   }
   bytes_read += RDLENGTH;

   record.set_NAME( NAME );
   record.set_TYPE( TYPE );
   record.set_CLASS( CLASS );
   record.set_TTL( TTL );
   record.set_RDLENGTH( RDLENGTH );
   record.set_RDATA( RDATA );

   return { record, bytes_read };
}

std::string
DNSMessage_rr_t::hl_to_IPAddr( uint32_t ip_integer ) {
   ip_integer = ntohl( ip_integer );
   std::string IPAddr;
   ssize_t i=0;
   for ( ; i<sizeof( ip_integer ) - 1; ++i ) {
      IPAddr += std::to_string( (char) ip_integer );
      IPAddr +=  ".";

      ip_integer = ip_integer >> 8;
   }
   IPAddr += std::to_string( (char) ip_integer );
  
   // std::reverse( IPAddr.begin(), IPAddr.end() );

   return IPAddr;
}