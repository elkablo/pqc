#include <pqc_kex_sidhex.hpp>
#include <pqc_weierstrass.hpp>

namespace pqc
{

kex_sidhex::kex_sidhex(mode mode_) :
	kex(mode_),
	key_(sidh_params(mode_ == mode::SERVER ? sidh_params::side::A : sidh_params::side::B))
{
}

std::string kex_sidhex::init()
{
	key_.generate();
	return key_.export_public();
}

std::string kex_sidhex::fini(const std::string& received)
{
	sidh_key_basic peer_key(key_.get_params().other_side());

	if (!peer_key.import_public(received))
		return std::string();

	return key_.compute_shared_secret(peer_key);
}

}
