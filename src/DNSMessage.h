#include <stdint.h>
#include <string>

// Header format
// https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1
struct DNSMessage_header_t {
    uint16_t ID;

    unsigned QR :    1;
    unsigned OPCODE: 4;
    unsigned AA :    1;
    unsigned TC :    1;
    unsigned RD :    1;
    unsigned RA :    1;
    unsigned Z  :    3;
    unsigned RCODE:  4;
    
    uint16_t QDCOUNT;
    uint16_t ANCOUNT;
    uint16_t NSCOUNT;
    uint16_t ARCOUNT;
};

struct DNSMessage_question_t {
    std::string QNAME;
    uint16_t    QTYPE;
    uint16_t    QCLASS;
};
std::string encode_domain_name( std::string );

struct DNSMessage_rr_t { // rr : resource record

};

struct DNSMessage {
    DNSMessage_header_t     header;
    DNSMessage_question_t   question;
    DNSMessage_rr_t         answer;
    DNSMessage_rr_t         authority;
    DNSMessage_rr_t         additional;
};