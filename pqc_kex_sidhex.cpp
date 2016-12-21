#include <pqc_kex_sidhex.hpp>
#include <pqc_base64.hpp>
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
	return base64_encode(key_.export_public());
}

std::string kex_sidhex::fini(const std::string& received)
{
	sidh_key peer_key(sidh_params(mode_ == mode::SERVER ? sidh_params::side::B : sidh_params::side::A));

	if (!peer_key.import_public(base64_decode(received)))
		return std::string();

	const Z &m = key_.get_m();
	const Z &n = key_.get_n();
	int l = key_.get_params().l;
	int e = key_.get_params().e;
	const WeierstrassPoint& P_image = peer_key.get_P_image();
	const WeierstrassPoint& Q_image = peer_key.get_Q_image();

	WeierstrassPoint generator = m*P_image + n*Q_image;
	return WeierstrassIsogeny(generator, l, e).image()->j_invariant().serialize();
}

}
