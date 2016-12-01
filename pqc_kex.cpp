#include <cstring>

#include "weierstrass.cpp"
#include "pqc.hpp"

namespace pqc
{

static const struct {
	enum pqc_kex kex;
	const char *name;
} kex_table[] = {
	{ PQC_KEX_SIDHex, "SIDHex" },
	{ PQC_KEX_UNKNOWN, NULL }
};

enum pqc_kex kex::from_string(const char *str, size_t size)
{
	for (int i = 0; kex_table[i].name; ++i)
		if (!strncasecmp (str, kex_table[i].name, size))
			return kex_table[i].kex;

	return PQC_KEX_UNKNOWN;
}

const char *kex::to_string(enum pqc_kex kex)
{
	for (int i = 0; kex_table[i].name; ++i)
		if (kex == kex_table[i].kex)
			return kex_table[i].name;

	return nullptr;
}

kex::kex(mode mode_) :
	mode_(mode_)
{
}

class SIDHex : public kex
{
public:
	SIDHex(mode);

	std::string init();
	std::string fini(const std::string &);
private:
	const WeierstrassPoint& P, Q, P_peer, Q_peer;
	const int l, e, l_peer, e_peer;
	const Z& le, le_peer;

	static void initialize();
	static bool initialized_;
	static Z p, lea, leb;
	static WeierstrassCurvePtr E;
	static WeierstrassPoint Pa, Qa, Pb, Qb;
	static int la, ea, lb, eb;
};

SIDHex::SIDHex(mode mode_) :
	kex(mode_),
	P(mode_ == mode::SERVER ? Pa : Pb),
	Q(mode_ == mode::SERVER ? Qa : Qb),
	P_peer(mode_ == mode::SERVER ? Pb : Pa),
	Q_peer(mode_ == mode::SERVER ? Qb : Qa),
	l(mode_ == mode::SERVER ? la : lb),
	e(mode_ == mode::SERVER ? ea : eb),
	l_peer(mode_ == mode::SERVER ? lb : la),
	e_peer(mode_ == mode::SERVER ? eb : ea),
	le(mode_ == mode::SERVER ? lea : leb),
	le_peer(mode_ == mode::SERVER ? leb : lea)
{
	initialize();
}

std::string SIDHex::init()
{
	Z m, n;
	do {
		m = random_z_below(le);
		n = random_z_below(le);
	} while ((m % l) == 0 && (n % l) == 0);

	WeierstrassPoint generator = m*P + n*Q;
	WeierstrassIsogeny isogeny(generator, l, e);

	//isogeny(P_peer)
	//isogeny(Q_peer)

	return "";
}

std::string SIDHex::fini(const std::string& other)
{
	return "";
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
	E = std::make_shared<const WeierstrassCurve>(GF(p, 1), GF(p, 0));
	WeierstrassPoint
		Pa(E,
		   GF(p, "2524646701852396349308425328218203569693", "2374093068336250774107936421407893885897"),
		   GF(p, "1309099413211767078055232768460483417201", "1944869260414574206229153243510104781725")
		),
		Pb(E,
		   GF(p, "1747407329595165241335131647929866065215", "1556716033657530876728525059284431761206"),
		   GF(p, "1975912874247458572654720717155755005566", "3456956202852028835529419995475915388483")
		);
	WeierstrassPoint Qa = Pa.psi(), Qb = Pb.psi();

	initialized_ = true;
}

}
