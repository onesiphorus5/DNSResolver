#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <tuple>

// Header format
// https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
struct DNSMessage_header_t {
// TODO: change this to private
public:
   uint16_t ID;

   // the codes
   unsigned QR :    1;
   unsigned OPCODE: 4;
   unsigned AA :    1;
   unsigned TC :    1;
   unsigned RD :    1;
   unsigned RA :    1;
   unsigned Z  :    3;
   unsigned RCODE:  4;
   static const ssize_t CODES_SIZE = 2;

   uint16_t QDCOUNT;
   uint16_t ANCOUNT;
   uint16_t NSCOUNT;
   uint16_t ARCOUNT;

public:
   DNSMessage_header_t( uint16_t id ) : 
      ID{ id },
      QR{ 0 },
      OPCODE{ 0 },
      AA{ 0 },
      TC{ 0 },
      RD{ 0 },
      RA{ 0 },
      Z{ 0 },
      RCODE{ 0 },
      QDCOUNT{ 0 },
      ANCOUNT{ 0 },
      NSCOUNT{ 0 },
      ARCOUNT{ 0 } {}

   void set_QR( uint8_t qr )  { QR = qr; }
   void set_OPCODE( uint8_t opcode )  { OPCODE = opcode; }
   void set_AA( uint8_t aa )  { AA = aa; }
   void set_TC( uint8_t tc ) { TC = tc; }
   void set_RD( uint8_t rd ) { RD = rd; }
   void set_RA( uint8_t ra ) { RA = ra; }
   void set_Z( uint8_t z ) { Z = z; }
   void set_RCODE( uint8_t rcode ) { RCODE = rcode; }

   void set_QDCOUNT( uint16_t qdcount ) { QDCOUNT = qdcount; }
   void set_ANCOUNT( uint16_t ancount ) { ANCOUNT = ancount; }
   void set_NSCOUNT( uint16_t nscount ) { NSCOUNT = nscount; }
   void set_ARCOUNT( uint16_t arcount ) { ARCOUNT = arcount; }

   size_t size() { 
     return sizeof( ID )      + 
            CODES_SIZE        +
            sizeof( QDCOUNT ) +
            sizeof( ANCOUNT ) +
            sizeof( NSCOUNT ) +
            sizeof( ARCOUNT ); 
   }

   std::string serialize();
   static DNSMessage_header_t parse_header( char* );
};

struct DNSMessage_question_t {
// private:
// TODO change to private after testing
public:
   std::string domain_name;
   std::string QNAME;
   uint16_t    QTYPE;
   uint16_t    QCLASS;

   std::string encode_domain_name();

public:
   DNSMessage_question_t() : 
      domain_name{}, QNAME{}, QTYPE{0}, QCLASS{0} {};

   DNSMessage_question_t( std::string d_name ) : 
      domain_name{d_name}, QTYPE{0}, QCLASS{0} {
      QNAME = encode_domain_name();
   }

   void set_QNAME( std::string qname ) { QNAME = qname; }
   void set_QTYPE( uint16_t qtype ) { QTYPE = qtype; }
   void set_QCLASS( uint16_t qclass ) { QCLASS = qclass; }

   size_t size() {
      return QNAME.size()    + 
             sizeof( QTYPE ) +
             sizeof( QCLASS );
   }

   std::string serialize();
   static std::tuple<DNSMessage_question_t, ssize_t> parse_question( char*, ssize_t );
};

struct DNSMessage_rr_t { // rr : resource record
private:
   std::string NAME;
   uint16_t TYPE;
   uint16_t CLASS;
   uint32_t TTL;
   uint16_t RDLENGTH;
   std::string RDATA;

public:
   std::string serialize();
};

std::string encode_domain_name( std::string );
void send_DNS_query( std::string );