#ifndef PQC_BASE64_HPP
#define PQC_BASE64_HPP

#include <string>

namespace pqc
{

std::string base64_encode(const std::string&);
std::string base64_decode(const std::string&);

}

#endif /* PQC_BASE64_HPP */
