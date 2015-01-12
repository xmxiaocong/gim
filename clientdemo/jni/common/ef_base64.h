#ifndef EF_BASE64_H
#define EF_BASE64_H

#include <string>

namespace ef{

std::string base64_encode(const std::string& s);
std::string base64_decode(const std::string& s);

}

#endif
