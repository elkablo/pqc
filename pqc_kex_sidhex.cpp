#include <pqc_kex_sidhex.hpp>
#include <pqc_random.hpp>
#include <pqc_base64.hpp>

namespace pqc
{

SIDHex::SIDHex(mode mode_) :
	kex(mode_),
	P(mode_ == mode::SERVER ? Pa : Pb),
	Q(mode_ == mode::SERVER ? Qa : Qb),
	P_peer(mode_ == mode::SERVER ? Pb : Pa),
	Q_peer(mode_ == mode::SERVER ? Qb : Qa),
	le(mode_ == mode::SERVER ? lea : leb)
{
	initialize();
	if (mode_ == mode::SERVER) {
		l = la;
		e = ea;
	} else {
		l = lb;
		e = eb;
	}
}

std::string SIDHex::init()
{

	do {
		m = random_z_below(le);
		n = random_z_below(le);
	} while ((m % l) == 0 && (n % l) == 0);

	WeierstrassPoint generator = m*P + n*Q;
	WeierstrassIsogeny isogeny(generator, l, e);

//	std::cout << isogeny(Q_peer) << " init " << l << "\n";

	return base64_encode(isogeny.image()->serialize() + isogeny(P_peer).serialize() + isogeny(Q_peer).serialize());
}

std::string SIDHex::fini(const std::string& received_)
{
	std::string received(base64_decode(received_));

	if (!received.size())
		return std::string();

	WeierstrassCurvePtr peer_curve = std::make_shared<WeierstrassCurve>(p);
	size_t curve_size = peer_curve->size();

	if (!peer_curve->unserialize(received.substr(0, curve_size)))
		return std::string();

	WeierstrassPoint iso_P(peer_curve), iso_Q(peer_curve);
	size_t point_size = iso_P.size();

	if (!iso_P.unserialize(received.substr(curve_size, point_size)) ||
	    !iso_Q.unserialize(received.substr(curve_size + point_size, point_size)))
		return std::string();

//	std::cout << iso_Q << " fini " << l << "\n";

	WeierstrassPoint generator = m*iso_P + n*iso_Q;
	return WeierstrassIsogeny(generator, l, e).image()->j_invariant().serialize();
}

bool SIDHex::initialized_ = false;
Z SIDHex::p, SIDHex::lea, SIDHex::leb;
WeierstrassCurvePtr SIDHex::E;
WeierstrassPoint SIDHex::Pa, SIDHex::Qa, SIDHex::Pb, SIDHex::Qb;
int SIDHex::la, SIDHex::lb, SIDHex::ea, SIDHex::eb;

void SIDHex::initialize() {
	if (initialized_)
		return;

	la = 2;
	lb = 3;
	ea = 63;
	eb = 41;
	lea = Z("9223372036854775808"); // 2^63
	leb = Z("36472996377170786403"); // 3^41
	p = lea*leb*11-1;
	E = std::make_shared<WeierstrassCurve>(GF(p, 1), GF(p, 0));
	Pa = WeierstrassPoint(
		E,
		GF(p, "2524646701852396349308425328218203569693", "2374093068336250774107936421407893885897"),
		GF(p, "1309099413211767078055232768460483417201", "1944869260414574206229153243510104781725")
	);
	Pb = WeierstrassPoint(
		E,
		GF(p, "1747407329595165241335131647929866065215", "1556716033657530876728525059284431761206"),
		GF(p, "1975912874247458572654720717155755005566", "3456956202852028835529419995475915388483")
	);
	Qa = Pa.psi();
	Qb = Pb.psi();

	initialized_ = true;
}

}
