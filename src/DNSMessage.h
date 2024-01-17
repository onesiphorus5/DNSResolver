#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <tuple>
#include <vector>

#include <iostream>

// Header format
// https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
struct DNSMessage_header_t {
// TODO: change this to private
private:
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
   DNSMessage_header_t() : 
      ID{ 0 },
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

   void set_ID( uint16_t id )        { ID = id; }
   void set_QR( uint8_t qr )         { QR = qr; }
   void set_OPCODE( uint8_t opcode ) { OPCODE = opcode; }
   void set_AA( uint8_t aa )         { AA = aa; }
   void set_TC( uint8_t tc )         { TC = tc; }
   void set_RD( uint8_t rd )         { RD = rd; }
   void set_RA( uint8_t ra )         { RA = ra; }
   void set_Z( uint8_t z )           { Z = z; }
   void set_RCODE( uint8_t rcode )   { RCODE = rcode; }

   void set_QDCOUNT( uint16_t qdcount ) { QDCOUNT = qdcount; }
   void set_ANCOUNT( uint16_t ancount ) { ANCOUNT = ancount; }
   void set_NSCOUNT( uint16_t nscount ) { NSCOUNT = nscount; }
   void set_ARCOUNT( uint16_t arcount ) { ARCOUNT = arcount; }

   uint16_t get_ID()    { return ID; }
   uint8_t get_QR()     { return QR; }
   uint8_t get_OPCODE() { return OPCODE; }
   uint8_t get_AA()     { return AA; }
   uint8_t get_TC()     { return TC; }
   uint8_t get_RD()     { return RD; }
   uint8_t get_RA()     { return RA; }
   uint8_t get_Z()      { return Z; }
   uint8_t get_RCODE()  { return RCODE; }

   uint16_t get_QDCOUNT() { return QDCOUNT; }
   uint16_t get_ANCOUNT() { return ANCOUNT; }
   uint16_t get_NSCOUNT() { return NSCOUNT; }
   uint16_t get_ARCOUNT() { return ARCOUNT; }


   size_t size() { 
     return sizeof( ID )      + 
            CODES_SIZE        +
            sizeof( QDCOUNT ) +
            sizeof( ANCOUNT ) +
            sizeof( NSCOUNT ) +
            sizeof( ARCOUNT ); 
   }

   std::string serialize();
   static DNSMessage_header_t parse_header( const char* );
};

struct DNSMessage_question_t {
private:
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
   void set_QTYPE( uint16_t qtype )    { QTYPE = qtype; }
   void set_QCLASS( uint16_t qclass )  { QCLASS = qclass; }

   std::string get_QNAME() { return QNAME; }
   uint16_t get_QTYPE()    { return QTYPE; }
   uint16_t get_QCLASS()   { return QCLASS; }


   size_t size() {
      return QNAME.size()    + 
             sizeof( QTYPE ) +
             sizeof( QCLASS );
   }

   std::string serialize();
   static std::tuple<DNSMessage_question_t, ssize_t> parse_question( const char*, ssize_t );
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
   DNSMessage_rr_t() : NAME{}, TYPE{0}, CLASS{0}, TTL{0}, 
                       RDLENGTH{0}, RDATA{} {}

   void set_NAME( std::string name )      { NAME = name; }
   void set_TYPE( uint16_t type )         { TYPE = type; }
   void set_CLASS( uint16_t _class )      { CLASS = _class; }
   void set_TTL( uint32_t ttl )           { TTL = ttl; }
   void set_RDLENGTH( uint16_t rdlength ) { RDLENGTH = rdlength; }
   void set_RDATA( std::string rdata )    { RDATA = rdata; }

   std::string get_NAME()  { return NAME; }
   uint16_t get_TYPE()     { return TYPE; }
   uint16_t get_CLASS()    { return CLASS; }
   uint32_t get_TTL()      { return TTL; }
   uint16_t get_RDLENGTH() { return RDLENGTH; }
   std::string get_RDATA() { return RDATA; }

   static std::tuple<DNSMessage_rr_t, ssize_t> parse_resource_record( const char*, ssize_t );
   static std::string hl_to_IPAddr( uint32_t );
};