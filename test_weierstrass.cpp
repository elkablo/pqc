#include <iostream>
#include <chrono>
#include <vector>
#include <functional>
#include <pqc_random.hpp>
#include <pqc_weierstrass.hpp>

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
	std::cout << str << " took " << ms << "ms (" << (ms/repeats) << " ms per iteration)\n";
}

void generate_mn(Z& m, Z& n, Z& le, int l)
{
	if (random_u32_below(l+1)) {
		m = 1;
		n = random_z_below(le);
	} else {
		m = random_z_below(le/l)*l;
		n = 1;
	}
}

void generate_mn_alternative(Z& m, Z& n, Z& le, int l)
{
	m = random_z_below(le/l)*l;
	n = 1;
}

void measure_time(
	std::function<void(Z&, Z&, Z&, int)> mn_generator,
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
	std::function<void(Z&, Z&, Z&, int)> mn_generator,
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
	std::function<void(Z&, Z&, Z&, int)> mn_generator,
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
	std::vector<int> strategy{
		0, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
		10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18,
		18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26,
		26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33, 34,
		34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40, 41, 41, 42,
		42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 50,
		50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 58,
		58, 59, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 66,
		66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 73, 74,
		74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 81, 82,
		82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90,
		90, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98,
		98, 99, 99, 100, 100, 101, 101, 102, 102, 103, 103, 104, 104,
		105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110,
		111, 111, 112, 112, 113, 113, 114, 114, 115, 115, 116, 116,
		117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122,
		123, 123, 124, 124, 125, 125, 126, 126, 127, 127, 128, 128,
		129, 129, 130, 130, 131, 131, 132, 132, 133, 133, 134, 134,
		135, 135, 136, 136, 137, 137, 138, 138, 139, 139, 140, 140,
		141, 141, 142, 142, 143, 143, 144, 144, 145, 145, 146, 146,
		147, 147, 148, 148, 149, 149, 150, 150, 151, 151, 152, 152,
		153, 153, 154, 154, 155, 155, 156, 156, 157, 157, 158, 158,
		159, 159, 160, 160, 161, 161, 162, 162, 163, 163, 164, 164,
		165, 165, 166, 166, 167, 167, 168, 168, 169, 169, 170, 170,
		171, 171, 172, 172, 173, 173, 174, 174, 175, 175, 176, 176,
		177, 177, 178, 178, 179, 179, 180, 180, 181, 181, 182, 182,
		183, 183, 184, 184, 185, 185, 186};

	// p = 2^372 * 3*239 - 1
	Z p("10354717741769305252977768237866805321427389645549071170116189679054678940682478846502882896561066713624553211618840202385203911976522554393044160468771151816976706840078913334358399730952774926980235086850991501872665651576831");

	// Pa = [3^239] (11, sqrt(11³ + 11))
	// Qa = ψ(Pa)
	// Pb = [2^372] (6, sqrt(6³ + 6))
	// Qb = ψ(Pb)

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
	if (false) {
		check_order(generate_mn, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb);
		check_order(generate_mn_alternative, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb);
		return;
	}

	// measure time
	if (true)
		return measure_time(generate_mn, Pa, Qa, Pb, Qb, lea, la, ea, leb, lb, eb, strategy);

	if (true) {
		int tries = 100, generate_mn_success = 0, generate_mn_alternative_success = 0;

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

int main (int argc, char ** argv) {
	test_squaring();
	test_serialization();
	test_weierstrass();
	return 0;
}
