#include <string>

// It is assumed domain names passed to the function have the following format:
// "<nth-level domain>...<third-level domain>.<second-level domain>.<top-level domain>"
std::string encode_domain_name( std::string domain_name ) {
    std::string QNAME;

    std::string delimiter = ".";
    size_t left_pos=0, right_pos;
    size_t label_size;
    while ( ( right_pos = domain_name.find( delimiter, left_pos ) ) 
            != std::string::npos ) {
        label_size = right_pos - left_pos;    
        QNAME += std::to_string( label_size );
        QNAME += domain_name.substr( left_pos, label_size );

        left_pos = right_pos+1;
    }
    label_size = domain_name.size() - left_pos;
    QNAME += std::to_string( label_size );
    QNAME += domain_name.substr( left_pos, label_size );

    return QNAME;
}