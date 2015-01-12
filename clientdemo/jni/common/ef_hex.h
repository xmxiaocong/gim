#ifndef EF_HEX_H
#define EF_HEX_H

#include "ef_btype.h"
#include <string>

namespace ef{


char* byte_to_hex(uint8 c, char buf[2]);
int hex_to_byte(char buf[2]);

int32 bytes_to_hexs(const std::string& bytes, std::string& hex);
int32 bytes_to_hexs(const char* bytes, int32 len, char* out, int32 outlen);
int32 hexs_to_bytes(const std::string& hex, std::string& bytes);
int32 hexs_to_bytes(const char* hex, int32 len, char* out, int32 outlen);

int32 hexs_bytes_test(const std::string& input);

};

#endif
