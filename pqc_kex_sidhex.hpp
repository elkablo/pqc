#ifndef PQC_KEX_SIDHEX_HPP
#define PQC_KEX_SIDHEX_HPP

#include <pqc_kex.hpp>
#include <pqc_sidh_key.hpp>

namespace pqc
{

class kex_sidhex : public kex
{
public:
	kex_sidhex(mode);

	std::string init();
	std::string fini(const std::string &);
private:
	sidh_key key_;
};

}

#endif /* PQC_KEX_SIDHEX_HPP */
