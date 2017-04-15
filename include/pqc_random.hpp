#ifndef PQC_RANDOM_HPP
#define PQC_RANDOM_HPP

#include <cstddef>
#include <string>
#include <pqc_gf.hpp>

namespace pqc
{

void random_bytes(void *, size_t);
std::string random_string(size_t);
Z random_z(size_t);
Z random_z_below(const Z&);
uint32_t random_u32_below(uint32_t);

}

#endif /* PQC_RANDOM_HPP */
