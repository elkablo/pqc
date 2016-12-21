#include <pqc_sidh_params.hpp>

namespace pqc
{

sidh_params::sidh_params(side s) :
	s(s),
	l(s == side::A ? la : lb),
	e(s == side::A ? ea : eb),
	prime(p),
	P(s == side::A ? Pa : Pb),
	Q(s == side::A ? Qa : Qb),
	P_peer(s == side::A ? Pb : Pa),
	Q_peer(s == side::A ? Qb : Qa),
	le(s == side::A ? lea : leb),
	lem1(s == side::A ? leam1 : lebm1)
{
	initialize();
}

int sidh_params::la, sidh_params::ea, sidh_params::lb, sidh_params::eb;
Z sidh_params::p, sidh_params::lea, sidh_params::leam1, sidh_params::leb, sidh_params::lebm1;
WeierstrassCurvePtr sidh_params::E;
WeierstrassPoint sidh_params::Pa, sidh_params::Qa, sidh_params::Pb, sidh_params::Qb;

void sidh_params::initialize()
{
	static bool initialized = false;

	if (initialized)
		return;

	la = 2;
	ea = 63;
	lb = 3;
	eb = 41;
	lea = Z("9223372036854775808"); // 2^63
	leam1 = Z("4611686018427387904"); // 2^62
	leb = Z("36472996377170786403"); // 3^41
	lebm1 = Z("12157665459056928801"); // 3^40
	p = Z("3700444163740528325594401040305817124863"); // la^ea * lb^eb * 11 - 1
	E = std::make_shared<WeierstrassCurve>(GF(p, 1), GF(p, 0));
	Pa = WeierstrassPoint(
		E,
		GF(p, "2524646701852396349308425328218203569693", "2374093068336250774107936421407893885897"),
		GF(p, "1309099413211767078055232768460483417201", "1944869260414574206229153243510104781725")
	);
	Qa = Pa.psi();
	Pb = WeierstrassPoint(
		E,
		GF(p, "1747407329595165241335131647929866065215", "1556716033657530876728525059284431761206"),
		GF(p, "1975912874247458572654720717155755005566", "3456956202852028835529419995475915388483")
	);
	Qb = Pb.psi();

	initialized = true;
}

}
