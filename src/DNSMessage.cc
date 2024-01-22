#include <time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include "DNSMessage.h"

#include <iostream>
using namespace std;

std::string
encode_domain_name( std::string domain_name ) {
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

std::tuple<std::string, ssize_t>
read_encoded_domain_name( char const* message_buffer, 
                                      ssize_t section_offset ) {
   std::string NAME;

   char const* buffer_cpy = message_buffer;
   buffer_cpy += section_offset;

   uint8_t label_size;
   ssize_t bytes_read = 0;
   ssize_t compressed_bytes_read = 0;
   uint16_t domain_name_offset;

   /* NAME */
   // TODO: the code handling compression has a bug, when double compression 
   //       is encountered.
   for ( ; ; ) {
      label_size = *buffer_cpy;
      buffer_cpy += 1;
      bytes_read += 1;

      if ( label_size == (char)0 ) {
         NAME += (char)0;
         break;
      }

      if ( ( label_size >> 6 ) == 3 ) { // Handle compression
         label_size = label_size << 2;
         label_size = label_size >> 2;

         uint16_t right_half = label_size;
         right_half = right_half << 8;

         uint16_t left_half = (uint16_t)*buffer_cpy;
         bytes_read += 1;
         left_half = left_half << 8;
         left_half = left_half >> 8;

         domain_name_offset = right_half | left_half;

         buffer_cpy = message_buffer + domain_name_offset;
         compressed_bytes_read = bytes_read = sizeof( domain_name_offset );
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
         cout << "Malformed question" << endl;
         break;
      }
   }
   if ( compressed_bytes_read != 0 ) {
      bytes_read = compressed_bytes_read;
   }

   return { NAME, bytes_read };
}

std::string
to_IPAddr_str( uint32_t ip_integer ) {
   std::string IPAddr;

   ssize_t i=0;
   for ( ; i<sizeof( ip_integer ) - 1; ++i ) {
      IPAddr += std::to_string( (uint8_t) ip_integer );
      IPAddr +=  ".";

      ip_integer = ip_integer >> 8;
   }
   IPAddr += std::to_string( (uint8_t) ip_integer );

   return IPAddr;
}

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

void
DNSMessage_header_t::parse_header() {
   char const* buffer = message_buffer;
   /* ID */
   ID = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( ID );

   /* codes */
   uint16_t codes = ntohs( *(uint16_t*)buffer );
   buffer += CODES_SIZE;

   /* QDCOUNT */
   QDCOUNT = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( QDCOUNT );

   /* ANCOUNT */
   ANCOUNT = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( ANCOUNT );

   /* NSCOUNT */
   NSCOUNT = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( NSCOUNT );

   /* ARCOUNT */
   ARCOUNT = ntohs( *(uint16_t*)buffer );
   buffer += sizeof( ARCOUNT );

   // Codes
   this->QR     = ( codes & ( 0b1 << 15 ) ) >> 15;
   this->OPCODE = ( codes & ( 0b1111 << 11 ) ) >> 11;
   this->AA     = ( codes & ( 0b1 << 10 ) ) >> 10;
   this->TC     = ( codes & ( 0b1 << 9 ) ) >> 9;
   this->RD     = ( codes & ( 0b1 << 8 ) ) >> 8;
   this->RA     = ( codes & ( 0b1 << 7 ) ) >> 7;
   this->Z      = ( codes & ( 0b111 << 4 ) ) >> 4;
   this->RCODE  = codes & ( 0b1111 );
}



std::string
DNSMessage_question_t::serialize() {
  std::string question_bytes;
  // question_bytes += encode_domain_name( domain_name );
  question_bytes += QNAME;

  question_bytes += (char) ( QTYPE >> 8 );
  question_bytes += (char) QTYPE;

  question_bytes += (char) ( QCLASS >> 8 );
  question_bytes += (char) QCLASS;

  return question_bytes;
}

ssize_t
DNSMessage_question::parse_questions( ssize_t q_section_offset ) {
   ssize_t offset = q_section_offset;
   ssize_t total_bytes_read = 0;

   questions.clear();
   ssize_t question_count = message_header->get_QDCOUNT();
   for ( ssize_t i=0; i<question_count; ++i ) {
      DNSMessage_question_t question;

      // QNAME
      auto[ QNAME, bytes_read ] = read_encoded_domain_name( 
                              message_header->get_message_buffer(), offset );

      const char* buffer_cpy = message_header->get_message_buffer();
      buffer_cpy += offset;
      buffer_cpy += bytes_read;

      /* QTYPE */
      uint16_t QTYPE = ntohs( *(uint16_t*)buffer_cpy );
      buffer_cpy += sizeof( QTYPE );
      bytes_read += sizeof( QTYPE );

      /* QCLASS */
      uint16_t QCLASS = ntohs( *(uint16_t*)buffer_cpy );
      buffer_cpy += sizeof( QCLASS );
      bytes_read += sizeof( QCLASS );

      question.set_QNAME( QNAME );
      question.set_QTYPE( QTYPE );
      question.set_QCLASS( QCLASS );

      questions.push_back( question );
      offset += bytes_read;
      total_bytes_read += bytes_read;
   }

   return total_bytes_read;
}



ssize_t
DNSMessage_rr::parse_records( ssize_t r_section_offset, 
                              RecordType section_type ) {
   set_section_type( section_type );

   ssize_t offset = r_section_offset;
   ssize_t total_bytes_read = 0;

   r_records_vec.clear();
   r_records_map.clear();
   ssize_t rr_count;
   switch ( section_type ) {
      case AN : rr_count = message_header->get_ANCOUNT(); break;
      case NS : rr_count = message_header->get_NSCOUNT(); break;
      case AR : rr_count = message_header->get_ARCOUNT(); break;
      default: exit( EXIT_FAILURE );
   };
   for ( ssize_t i=0; i<rr_count; ++i ) {
      DNSMessage_rr_t record;

      // NAME
      auto[ NAME, bytes_read ] = read_encoded_domain_name( 
                     message_header->get_message_buffer(), offset );

      const char* buffer_cpy = message_header->get_message_buffer();
      buffer_cpy += offset;
      buffer_cpy += bytes_read;

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
      if ( TYPE ==  2 || TYPE == 5 ) { // NS type or CNAME type
         ssize_t rdata_offset =
               (ssize_t) ( buffer_cpy - message_header->get_message_buffer() );
         auto[ rdata, _ ] = read_encoded_domain_name( 
               message_header->get_message_buffer(),  rdata_offset );
         RDATA = rdata;
      } else {
         for ( uint16_t i=0; i<RDLENGTH; ++i ) {
            RDATA += *( buffer_cpy + i );
         }
      }
      bytes_read += RDLENGTH;
      
      record.set_NAME( NAME );
      record.set_TYPE( TYPE );
      record.set_CLASS( CLASS );
      record.set_TTL( TTL );
      record.set_RDLENGTH( RDLENGTH );
      record.set_RDATA( RDATA );

      offset += bytes_read;
      total_bytes_read += bytes_read;

      if ( section_type == AR && TYPE != 1 ) { // Only IPv4 is being handled
         continue;
      }
      if ( section_type == AR ) {
         r_records_map[ NAME ] = record;
      } else {
         r_records_vec.push_back( record );
      }
   }

   return total_bytes_read;
}