#ifndef PQC_SHA_HPP
#define PQC_SHA_HPP

#include <string>

namespace pqc
{

std::string bin2hex(const std::string&);
std::string sha256(const std::string&, bool = true);
std::string sha512(const std::string&, bool = true);

}

#endif /* PQC_SHA_HPP */
