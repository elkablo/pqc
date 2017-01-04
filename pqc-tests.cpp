#include <cstring>
#include <iostream>
#include <chrono>
#include <vector>
#include <functional>
#include <pqc_random.hpp>
#include <pqc_weierstrass.hpp>
#include <pqc_sidh_params.hpp>

using namespace pqc;

void test_squaring () {
	Z p("3700444163740528325594401040305817124863");
	Z t1("3971395719089189613198579");
	Z t2("1987531981819750981750131");
	GF x(p, t1, t2);

	std::cout << "x = " << x << "\n";
	x *= x;
	std::cout << "x² = " << x << "\n";
	x.sqrt();
	std::cout << "sqrt(x²) = " << x << "\n";
	x *= x;
	std::cout << "sqrt(x²)² = " << x << "\n";
}

void measure(const std::string& str, int repeats, std::function<void()> f) {
	using namespace std::chrono;
	auto start = steady_clock::now();
	for (int i = 0; i < repeats; ++i)
		f();
	auto end = steady_clock::now();
	auto ms = duration_cast<milliseconds>(end - start).count();
	std::cout << str << " took " << ms << " ms (" << (ms/repeats) << " ms per iteration)\n";
}

void generate_mn(Z& m, Z& n, const Z& le, int l)
{
	if (random_u32_below(l+1)) {
		m = 1;
		n = random_z_below(le);
	} else {
		m = random_z_below(le/l)*l;
		n = 1;
	}
}

void generate_mn_alternative(Z& m, Z& n, const Z& le, int l)
{
	m = random_z_below(le/l)*l;
	n = 1;
}

void measure_time(
	std::function<void(Z&, Z&, const Z&, int)> mn_generator,
	WeierstrassPoint& Pa,
	WeierstrassPoint& Qa,
	WeierstrassPoint& Pb,
	WeierstrassPoint& Qb,
	Z& lea,
	int la,
	int ea,
	Z& leb,
	int lb,
	int eb,
	std::vector<int>& strategy
)
{
	Z ma, na, mb, nb;

	mn_generator(ma, na, lea, la);
	mn_generator(mb, nb, leb, lb);

	WeierstrassPoint gen_a = ma*Pa + na*Qa, gen_b = mb*Pb + nb*Qb;

	measure("A without strategy", 1, [&gen_a, la, ea, &Pb, &Qb]() {
		WeierstrassIsogeny iso_a(gen_a, la, ea);
		iso_a(Pb);
		iso_a(Qb);
	});
	measure("B without strategy", 1, [&gen_b, lb, eb, &Pa, &Qa]() {
		WeierstrassIsogeny iso_b(gen_b, lb, eb);
		iso_b(Pa);
		iso_b(Qa);
	});
	measure("A with strategy", 10, [&gen_a, la, ea, &Pb, &Qb, &strategy]() {
		WeierstrassIsogeny iso_a(gen_a, la, ea, strategy);
		iso_a(Pb);
		iso_a(Qb);
	});
	measure("B with strategy", 10, [&gen_b, lb, eb, &Pa, &Qa, &strategy]() {
		WeierstrassIsogeny iso_b(gen_b, lb, eb, strategy);
		iso_b(Pa);
		iso_b(Qa);
	});
}

void check_order(
	std::function<void(Z&, Z&, const Z&, int)> mn_generator,
	WeierstrassPoint& Pa,
	WeierstrassPoint& Qa,
	WeierstrassPoint& Pb,
	WeierstrassPoint& Qb,
	Z& lea,
	int la,
	int ea,
	Z& leb,
	int lb,
	int eb
)
{
	Z ma, na, mb, nb;

	mn_generator(ma, na, lea, la);
	mn_generator(mb, nb, leb, lb);

	WeierstrassPoint gen_a = ma*Pa + na*Qa, gen_b = mb*Pb + nb*Qb;

	Z base(1), mul(la);
	for (int i = 0; i <= ea; ++i) {
		if ((base * Pa).is_identity())
			std::cout << "Pa is of order " << mul << "^" << i << "\n";
		if ((base * Qa).is_identity())
			std::cout << "Qa is of order " << mul << "^" << i << "\n";
		if ((base * gen_a).is_identity())
			std::cout << "gen_a is of order " << mul << "^" << i << "\n";
		base *= mul;
	}

	base = 1;
	mul = lb;
	for (int i = 0; i <= eb; ++i) {
		if ((base * Pb).is_identity())
			std::cout << "Pb is of order " << mul << "^" << i << "\n";
		if ((base * Qb).is_identity())
			std::cout << "Qb is of order " << mul << "^" << i << "\n";
		if ((base * gen_b).is_identity())
			std::cout << "gen_b is of order " << mul << "^" << i << "\n";
		base *= mul;
	}
}

bool compare_j_invariants(
	std::function<void(Z&, Z&, const Z&, int)> mn_generator,
	WeierstrassPoint& Pa,
	WeierstrassPoint& Qa,
	WeierstrassPoint& Pb,
	WeierstrassPoint& Qb,
	Z& lea,
	int la,
	int ea,
	Z& leb,
	int lb,
	int eb,
	std::vector<int>& strategy
)
{
	Z ma, na, mb, nb;

	mn_generator(ma, na, lea, la);
	mn_generator(mb, nb, leb, lb);

	WeierstrassPoint gen_a = ma*Pa + na*Qa, gen_b = mb*Pb + nb*Qb;

	WeierstrassIsogeny iso_a(gen_a, la, ea, strategy);
	WeierstrassIsogeny iso_b(gen_b, lb, eb, strategy);

	WeierstrassPoint gen_ab = ma*iso_b(Pa) + na*iso_b(Qa);
	WeierstrassPoint gen_ba = mb*iso_a(Pb) + nb*iso_a(Qb);

	WeierstrassIsogeny iso_ab(gen_ab, la, ea, strategy);
	WeierstrassIsogeny iso_ba(gen_ba, lb, eb, strategy);

	return iso_ab.image()->j_invariant() == iso_ba.image()->j_invariant();
}

void test_weierstrass () {
	sidh_params params(sidh_params::side::A);
	std::vector<int> strategy = params.strategy;

	const Z &p = params.prime;

	WeierstrassCurvePtr E = std::make_shared<WeierstrassCurve>(GF(p, 1), GF(p, 0));

	int la = 2, lb = 3, ea = 372, eb = 239, f = 1;

	Z lea = Z(la).pow(ea);
	Z leb = Z(lb).pow(eb);

	auto PQa = E->basis(la, ea, lb, eb, f);
	auto PQb = E->basis(lb, eb, la, ea, f);

	auto Pa = PQa.first;
	auto Qa = PQa.second;
	auto Pb = PQb.first;
	auto Qb = PQb.second;

	// WeierstrassPoint Pa(E, GF(p, 11), GF(p, Z(11*11*11+11)).sqrt());
	// WeierstrassPoint Pb(E, GF(p, 6), GF(p, Z(6*6*6+6)).sqrt());
	// Pa *= leb;
	// Pb *= lea;
	// WeierstrassPoint Qa = Pa.psi(), Qb = Pb.psi();

	// check order
	if (true) {
		check_order(generate_mn, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb);
		check_order(generate_mn_alternative, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb);
	}

	// measure time
	if (true)
		measure_time(generate_mn, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb, strategy);

	if (true) {
		int tries = 3, generate_mn_success = 0, generate_mn_alternative_success = 0;

		std::cout << "Comparing j-invariants\n";

		for (int i = 1; i <= tries; ++i) {
			std::cout << "Round " << i << ": ";
			if (compare_j_invariants(generate_mn, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb, strategy)) {
				++generate_mn_success;
				std::cout << "T ";
			} else {
				std::cout << "F ";
			}
			if (compare_j_invariants(generate_mn_alternative, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb, strategy)) {
				++generate_mn_alternative_success;
				std::cout << "T\n";
			} else {
				std::cout << "F\n";
			}
		}

		std::cout << "j-invariants were same for both sides of key exchange:\n";
		std::cout << "\t" << generate_mn_success << " times of " << tries << " using default m,n-generator\n";
		std::cout << "\t" << generate_mn_alternative_success << " times of " << tries << " using alternative m,n-generator\n";
	}
}

void test_serialization() {
	Z p("3700444163740528325594401040305817124863");
	WeierstrassCurvePtr curve = std::make_shared<WeierstrassCurve>(
		GF(p, "2524646701852396349308425328218203569693", "2374093068336250774107936421407893885897"),
		GF(p, "1309099413211767078055232768460483417201", "1944869260414574206229153243510104781725")
	);
	WeierstrassPoint identity(curve);

	WeierstrassCurvePtr unserialized_curve = std::make_shared<WeierstrassCurve>(p);
	unserialized_curve->unserialize(curve->serialize());

	std::cout << *curve << '\n';
	curve->unserialize(curve->serialize());
	std::cout << *curve << '\n';

	WeierstrassPoint point(
		curve,
		GF(p, "2524646701852396349308425328218203569693", "2374093068336250774107936421407893885897"),
		GF(p, "1309099413211767078055232768460483417201", "1944869260414574206229153243510104781725")
	);

	std::cout << point << '\n';
	point.unserialize(point.serialize());
	std::cout << point << '\n';

	point *= 0;

	std::cout << point << '\n';
	point.unserialize(point.serialize());
	std::cout << point << '\n';
}

#ifdef HAVE_MSR_SIDH
#define _AMD64_
#define __LINUX__
#include <msr-sidh/SIDH.h>

CRYPTO_STATUS random_bytes_for_msr(unsigned int n, unsigned char* o)
{
	random_bytes(o, n);
	return CRYPTO_SUCCESS;
}

typedef CRYPTO_STATUS (*KeyGeneration_t)(unsigned char*, unsigned char*, PCurveIsogenyStruct);
typedef CRYPTO_STATUS (*SecretAgreement_t)(unsigned char*, unsigned char*, unsigned char*, PCurveIsogenyStruct);

void keygen_libpqc(const sidh_params& params, Z *om, Z *on, WeierstrassPoint *oiso_P_peer, WeierstrassPoint *oiso_Q_peer)
{
	Z m, n;
	generate_mn(m, n, params.le, params.l);
	WeierstrassPoint gen = m*params.P + n*params.Q;
	WeierstrassIsogeny iso(gen, params.l, params.e, params.strategy);

	WeierstrassPoint iso_P_peer = iso(params.P_peer);
	WeierstrassPoint iso_Q_peer = iso(params.Q_peer);

	if (om)
		*om = m;
	if (on)
		*on = n;
	if (oiso_P_peer)
		*oiso_P_peer = iso_P_peer;
	if (oiso_Q_peer)
		*oiso_Q_peer = iso_Q_peer;
}

void keygen_sidhlib(KeyGeneration_t KeyGeneration_X, unsigned char *opriv, unsigned char *opub)
{
	size_t osize = (CurveIsogeny_SIDHp751.owordbits + 7)/8;
	size_t psize = (CurveIsogeny_SIDHp751.pwordbits + 7)/8;
	unsigned char priv[osize], pub[4*2*psize];
	PCurveIsogenyStruct iso = SIDH_curve_allocate(&CurveIsogeny_SIDHp751);
	SIDH_curve_initialize(iso, random_bytes_for_msr, &CurveIsogeny_SIDHp751);
	KeyGeneration_X(priv, pub, iso);
	if (opriv)
		::memcpy(opriv, priv, osize);
	if (opub)
		::memcpy(opub, pub, 4*2*psize);
}

void final_libpqc(const sidh_params& params, const Z& m, const Z& n, const WeierstrassPoint& iso_P, const WeierstrassPoint& iso_Q)
{
	WeierstrassPoint gen = m*iso_P + n*iso_Q;
	WeierstrassIsogeny iso(gen, params.l, params.e, params.strategy);
	iso.image()->j_invariant();
}

void final_sidhlib(SecretAgreement_t SecretAgreement_X, unsigned char *priv, unsigned char *pub_peer)
{
	size_t psize = (CurveIsogeny_SIDHp751.pwordbits + 7)/8;
	unsigned char shared[2*psize];
	PCurveIsogenyStruct iso = SIDH_curve_allocate(&CurveIsogeny_SIDHp751);
	SIDH_curve_initialize(iso, random_bytes_for_msr, &CurveIsogeny_SIDHp751);
	SecretAgreement_X(priv, pub_peer, shared, iso);
}

void test_msr_sidh()
{
	sidh_params paramsA(sidh_params::side::A);
	sidh_params paramsB(sidh_params::side::A);

	int iters = 10;

	measure("key generation A [libpqc]                 ", iters, [&paramsA]() {
		keygen_libpqc(paramsA, nullptr, nullptr, nullptr, nullptr);
	});

	measure("key generation A [SIDH library]           ", iters, []() {
		keygen_sidhlib(KeyGeneration_A, nullptr, nullptr);
	});

	std::cout << std::endl;

	measure("key generation B [libpqc]                 ", iters, [&paramsB]() {
		keygen_libpqc(paramsB, nullptr, nullptr, nullptr, nullptr);
	});

	measure("key generation B [SIDH library]           ", iters, []() {
		keygen_sidhlib(KeyGeneration_B, nullptr, nullptr);
	});

	std::cout << std::endl;

	size_t osize = (CurveIsogeny_SIDHp751.owordbits + 7)/8;
	size_t psize = (CurveIsogeny_SIDHp751.pwordbits + 7)/8;
	unsigned char privA[osize], privB[osize], pubA[4*2*psize], pubB[4*2*psize];

	Z ma, na, mb, nb;
	WeierstrassPoint iso_PA, iso_QA, iso_PB, iso_QB;
	keygen_libpqc(paramsA, &ma, &na, &iso_PB, &iso_QB);
	keygen_libpqc(paramsB, &mb, &nb, &iso_PA, &iso_QA);

	keygen_sidhlib(KeyGeneration_A, privA, pubA);
	keygen_sidhlib(KeyGeneration_B, privB, pubB);

	measure("shared secret computation A [libpqc]      ", iters, [&paramsA, &ma, &na, &iso_PB, &iso_QB]() {
		final_libpqc(paramsA, ma, na, iso_PB, iso_QB);
	});

	measure("shared secret computation A [SIDH library]", iters, [&privA, &pubB]() {
		final_sidhlib(SecretAgreement_A, privA, pubB);
	});

	std::cout << std::endl;

	measure("shared secret computation B [libpqc]      ", iters, [&paramsB, &mb, &nb, &iso_PA, &iso_QA]() {
		final_libpqc(paramsB, mb, nb, iso_PA, iso_QA);
	});

	measure("shared secret computation B [SIDH library]", iters, [&privB, &pubA]() {
		final_sidhlib(SecretAgreement_B, privB, pubA);
	});

	std::cout << std::endl;
}
#endif /* HAVE_MSR_SIDH */

int usage()
{
	std::cerr << "usage: pqc-tests [squaring|serialization|weierstrass";
#ifdef HAVE_MSR_SIDH
	std::cerr << "|msr_sidh";
#endif /* HAVE_MSR_SIDH */
	std::cerr << "]" << std::endl << std::endl;
	return 1;
}

int main (int argc, char ** argv) {
	bool squaring = false, serialization = false, weierstrass = false, msr_sidh = false;

	for (int i = 1; i < argc; ++i) {
		if (!strcasecmp(argv[i], "squaring"))
			squaring = true;
		else if (!strcasecmp(argv[i], "serialization"))
			serialization = true;
		else if (!strcasecmp(argv[i], "weierstrass"))
			weierstrass = true;
#ifdef HAVE_MSR_SIDH
		else if (!strcasecmp(argv[i], "msr_sidh"))
			msr_sidh = true;
#endif /* HAVE_MSR_SIDH */
		else
			return usage();
	}

	if (!squaring && !serialization && !weierstrass && !msr_sidh)
		return usage();

	if (squaring)
		test_squaring();
	if (serialization)
		test_serialization();
	if (weierstrass)
		test_weierstrass();
#ifdef HAVE_MSR_SIDH
	if (msr_sidh)
		test_msr_sidh();
#endif /* HAVE_MSR_SIDH */
	return 0;
}
