#ifndef PQC_KEX_SIDHEX_HPP
#define PQC_KEX_SIDHEX_HPP

#include <pqc_kex.hpp>
#include <pqc_weierstrass.hpp>

namespace pqc
{

class SIDHex : public kex
{
public:
	SIDHex(mode);

	std::string init();
	std::string fini(const std::string &);
private:
	Z m, n;
	const WeierstrassPoint &P, &Q, &P_peer, &Q_peer;
	const Z &le;
	int l, e;

	static void initialize();
	static bool initialized_;
	static Z p, lea, leb;
	static WeierstrassCurvePtr E;
	static WeierstrassPoint Pa, Qa, Pb, Qb;
	static int la, ea, lb, eb;
};

}

#endif /* PQC_KEX_SIDHEX_HPP */
