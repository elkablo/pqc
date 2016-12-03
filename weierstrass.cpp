#include <memory>
#include <vector>

#include "gf.hpp"

/* https://crypto.stanford.edu/pbc/notes/elliptic/explicit.html
   https://pendientedemigracion.ucm.es/BUCM/mat/doc8354.pdf

   Y² + a₁XY + a₃Y = X³ + a₂X² + a₄X + a₆
   b₂ = a₁² + 4a₂
   b₄ = 2a₄ + a₁a₃
   b₆ = a₃² + 4a₆
   b₈ = a₁²a₆ + 4a₂a₆ - a₁a₃a₄ + a₂a₃² - a₄²
   c₄ = b₂² - 24b₄
   c₆ = -b₂³ + 36b₂b₄ - 216b₆
   Δ = -b₂²b₈ - 8b₄³ - 27b₆² + 9b₂b₄b₆
   j = c₄³/Δ = 1728 + c₆²/Δ

   If the characteristic is not 2 nor 3, the curve can be written as
   Y² = X³ + aX + b
   Δ = -16·(4a³ + 27b²)
   j = (-48a)³/Δ = 48³a³ / 16·(4a³ + 27b²) = 6912a³ / (4a³ + 27b²)
   j = 1728 + 864²b²/Δ = 1728 - 216²b² / (4a³ + 27b²)

 */

class WeierstrassCurve;
class WeierstrassPoint;
class WeierstrassSmallIsogeny;

class WeierstrassCurve {
	GF a, b;
public:
	friend class WeierstrassPoint;
	friend class WeierstrassSmallIsogeny;

	WeierstrassCurve(const GF& _a, const GF& _b) : a(_a), b(_b) {}
	WeierstrassCurve(const Z& p) : a(p), b(p) {}

	GF j_invariant() const {
		GF a3m4 = 4*a.square()*a;
		return 1728 * a3m4 / (a3m4 + 27*b.square());
	}

	WeierstrassSmallIsogeny small_isogeny (const WeierstrassPoint& generator, int l) const;

	std::string serialize() const {
		return a.serialize() + b.serialize();
	}

	size_t size() const {
		return 2*a.size();
	}

	bool unserialize(const std::string& raw) {
		if (raw.size() != size())
			return false;
		return a.unserialize(raw.substr(0, a.size())) && b.unserialize(raw.substr(a.size()));
	}

	friend std::ostream& operator<<(std::ostream& os, const WeierstrassCurve& curve) {
		os << "y² = x³ + " << curve.a << "·x + " << curve.b;
		return os;
	}
};

typedef std::shared_ptr<WeierstrassCurve> WeierstrassCurvePtr;

class WeierstrassPoint {
	WeierstrassCurvePtr m_curve;
	GF x, y;
	bool identity;
public:
	friend class WeierstrassCurve;
	friend class WeierstrassSmallIsogeny;

	WeierstrassPoint() {}

	WeierstrassPoint(const WeierstrassCurvePtr& curve, const GF& x, const GF& y) :
		m_curve(curve), x(x), y(y), identity(false) {}

	WeierstrassPoint(const WeierstrassCurvePtr& curve, const GF& x) :
		m_curve(curve), x(x), identity(false) {

		y = (x.square() + m_curve->a)*x + m_curve->b;
		if (!y.is_square()) {
			m_curve.reset();
			return;
		}

		y.sqrt();
	}

	WeierstrassPoint(const WeierstrassCurvePtr& curve) : m_curve(curve), x(curve->a.get_p()), y(curve->a.get_p()), identity(true) {}

	const WeierstrassCurvePtr& curve() const {
		return m_curve;
	}

	bool check() const {
		return m_curve && identity || y.square() == ((x.square() + m_curve->a)*x + m_curve->b);
	}

	bool is_identity() const {
		return identity;
	}

	size_t size() const {
		return 1 + 2*x.size();
	}

	std::string serialize() const {
		if (identity) {
			return std::string(1 + size(), 0x00);
		} else {
			return ((char) 0x01) + x.serialize() + y.serialize();
		}
	}

	bool unserialize(const std::string& raw) {
		size_t half = x.size();
		if (raw.size() != 2*half+1) {
			return false;}
		if (raw[0] == 0x00) {
			identity = true;
			return true;
		} else if (raw[0] == 0x01) {
			identity = false;
			return x.unserialize(raw.substr(1, half)) && y.unserialize(raw.substr(1 + half));
		} else {
			return false;
		}
	}

	bool operator==(const WeierstrassPoint& other) const {
		if (identity)
			return other.identity;
		else if (other.identity)
			return false;
		else
			return x == other.x && y == other.y;
	}

	bool operator!=(const WeierstrassPoint& other) const {
		if (identity)
			return !other.identity;
		else if (other.identity)
			return true;
		else
			return x != other.x || y != other.y;
	}

	WeierstrassPoint operator-() const {
		if (identity)
			return *this;
		else
			return WeierstrassPoint(m_curve, x, -y);
	}

	WeierstrassPoint operator+(const WeierstrassPoint& other) const {
		const GF &x1 = x, &y1 = y, &x2 = other.x, &y2 = other.y;
		if (identity) {
			return other;
		} else if (other.identity) {
			return *this;
		} else if (x1 == x2) {
			if (y1 == -y2) {
				return WeierstrassPoint(m_curve);
			} else {
				GF lambda = (3*x1.square() + m_curve->a) / (2*y1);
				GF x3 = lambda.square() - x1 - x1;
				GF y3 = lambda*(x1 - x3) - y1;
				return WeierstrassPoint(m_curve, x3, y3);
			}
		} else {
			GF lambda = (y2 - y1) / (x2 - x1);
			GF x3 = lambda.square() - x1 - x2;
			GF y3 = lambda*(x1 - x3) - y1;
			return WeierstrassPoint(m_curve, x3, y3);
		}
	}

	WeierstrassPoint& operator+=(const WeierstrassPoint& other) {
		*this = *this + other;
		return *this;
	}

	WeierstrassPoint& operator-=(const WeierstrassPoint& other) {
		*this += -other;
		return *this;
	}

	WeierstrassPoint operator-(const WeierstrassPoint& other) const {
		WeierstrassPoint res(*this);
		res -= other;
		return res;
	}

	/* multiply by montgomery ladder */
	WeierstrassPoint operator*(const Z& n) const {
		WeierstrassPoint R0(m_curve), R1(*this);
		for (std::ptrdiff_t i = n.bit_length() - 1; i >= 0; --i) {
			if (n.testbit(i)) {
				R0 += R1;
				R1 += R1;
			} else {
				R1 += R0;
				R0 += R0;
			}
		}
		return R0;
	}

	WeierstrassPoint& operator*=(const Z& n) {
		*this = *this * n;
		return *this;
	}

	friend WeierstrassPoint operator*(const Z& n, const WeierstrassPoint& other) {
		return other * n;
	}

	WeierstrassPoint psi() const {
		return WeierstrassPoint(m_curve, -x, y * GF(y.get_p(), 0, 1));
	}

	friend std::ostream& operator<<(std::ostream& os, const WeierstrassPoint& point) {
		if (point.identity)
			os << "identity ∈ " << *point.m_curve;
		else
			os << "(" << point.x << ", " << point.y << ") ∈ " << *point.m_curve;
		return os;
	}
};

class WeierstrassSmallIsogeny {
	WeierstrassCurvePtr m_image;
	const WeierstrassPoint m_generator;
	const int m_degree;

	WeierstrassSmallIsogeny(const WeierstrassCurvePtr& image, const WeierstrassPoint& generator, int degree) :
		m_image(image), m_generator(generator), m_degree(degree) {}

public:
	friend class WeierstrassCurve;

	const WeierstrassCurvePtr& image() const {
		return m_image;
	}

	const WeierstrassPoint& generator() const {
		return m_generator;
	}

	Z degree() const {
		return Z(m_degree);
	}

	WeierstrassPoint operator()(const WeierstrassPoint& source) const {
		if (source.is_identity())
			return WeierstrassPoint(m_image);

		WeierstrassPoint from_kernel(m_generator);
		GF x(source.x), y(source.y), xx(source.x);

		for (int i = 0; i < m_degree-1; ++i) {
			WeierstrassPoint sum = (source + from_kernel);
			x += sum.x - from_kernel.x;
			y += sum.y - from_kernel.y;
			from_kernel += m_generator;
		}

		return WeierstrassPoint(m_image, x, y);
	}
};

WeierstrassSmallIsogeny WeierstrassCurve::small_isogeny (const WeierstrassPoint& generator, int l) const {
	GF t = a*(l-1), w = 2*(l-1)*b;
	if (l == 2) {
		/* σ  = x(generator)
		   σ₂ = 0
		   σ₃ = 0  */
		const GF& s = generator.x;
		GF ss = s.square();
		t += 3*ss;
		w += 3*a*s + 5*ss*s;
	} else if (l == 3) {
		/* σ  = 2·x(generator)
		   σ₂ = x(generator)²
		   σ₃ = 0  */
		GF s = 2*generator.x;
		GF s2 = generator.x.square();
		GF ss = 4 * s2;
		t += 3*(ss - 2*s2);
		w += 3*a*s + 5*(ss*s - 3*s*s2);
	} else {
		
	}
	return WeierstrassSmallIsogeny(std::make_shared<WeierstrassCurve>(a - 5*t, b - 7*w), generator, l);
}

class WeierstrassIsogeny {
	const WeierstrassPoint m_generator;
	const int m_base, m_exp;
	std::vector<WeierstrassSmallIsogeny> m_isogenies;
public:
	WeierstrassIsogeny(const WeierstrassPoint& generator, int base, int exp) :
		m_generator(generator), m_base(base), m_exp(exp)
	{
		Z zbase(base);
		WeierstrassCurvePtr curve = generator.curve();
		WeierstrassPoint R(generator);

		m_isogenies.reserve(exp);

		for (int i = 0; i < exp; ++i) {
			auto isogeny = curve->small_isogeny(zbase.pow(exp-i-1) * R, base);
			m_isogenies.push_back(isogeny);
			curve = isogeny.image();
			if (i < exp-1)
				R = isogeny(R);
		}
	}

	WeierstrassIsogeny(const WeierstrassPoint& generator, int base, int exp, const std::vector<int>& strategy) :
		m_generator(generator), m_base(base), m_exp(exp)
	{
		WeierstrassCurvePtr curve = generator.curve();
		std::vector<WeierstrassPoint> Rs{generator};
		std::vector<int> hs{exp};

		while (Rs.size()) {
			WeierstrassPoint tmp = Rs.back();
			int h = hs.back();
			int split = strategy[h];

			while (h > 1) {
				for (int i = 0; i < h - split; ++i)
					tmp *= base;
				Rs.push_back(tmp);
				hs.push_back(split);
				h = split;
				split = strategy[h];
			}

			tmp = Rs.back();
			Rs.pop_back();
			h = hs.back();
			hs.pop_back();

			auto isogeny = tmp.curve()->small_isogeny(tmp, base);

			for (size_t i = 0; i < Rs.size(); ++i) {
				Rs[i] = isogeny(Rs[i]);
				--hs[i];
			}

			m_isogenies.push_back(isogeny);
		}
	}

	Z degree() const {
		return Z(m_base).pow(m_exp);
	}

	const WeierstrassCurvePtr& image() const {
		return m_isogenies[m_exp-1].image();
	}

	const WeierstrassPoint& generator() const {
		return m_generator;
	}

	const std::vector<WeierstrassSmallIsogeny>& isogenies() const {
		return m_isogenies;
	}

	void print_j_invariants() const {
		for (auto isogeny : m_isogenies) {
			std::cout << isogeny.image()->j_invariant() << "\n";
		}
	}

	WeierstrassPoint operator()(const WeierstrassPoint& source) const {
		WeierstrassPoint res(source);
		for (auto isogeny : m_isogenies) {
			res = isogeny(res);
		}
		return res;
	}
};

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

#ifdef WEIERSTRASS_MAIN
#include <chrono>
#include <functional>
#include "pqc.hpp"

void measure(const std::string& str, int repeats, std::function<void()> f) {
	using namespace std::chrono;
	auto start = steady_clock::now();
	for (int i = 0; i < repeats; ++i)
		f();
	auto end = steady_clock::now();
	std::cout << str << " took " << duration_cast<milliseconds>(end - start).count() << "ms\n";
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

/*
	Z p("3700444163740528325594401040305817124863");
	WeierstrassCurvePtr E = std::make_shared<WeierstrassCurve>(GF(p, 1), GF(p, 0));
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

	Z ma("2575042839726612324"), na("8801426132580632841"), mb("4558164392438856871"), nb("20473135767366569910");
*/
	Z p("10354717741769305252977768237866805321427389645549071170116189679054678940682478846502882896561066713624553211618840202385203911976522554393044160468771151816976706840078913334358399730952774926980235086850991501872665651576831");

	WeierstrassCurvePtr E = std::make_shared<WeierstrassCurve>(GF(p, 1), GF(p, 0));
	WeierstrassPoint Pa(E, GF(p, 11), GF(p, Z(11*11*11+11)).sqrt());
	WeierstrassPoint Pb(E, GF(p, 6), GF(p, Z(6*6*6+6)).sqrt());
	int la = 2, lb = 3, ea = 372, eb = 239;
	Z lea = Z(la).pow(ea);
	Z leb = Z(lb).pow(eb);
	Pa *= leb;
	Pb *= lea;
	WeierstrassPoint Qa = Pa.psi(), Qb = Pb.psi();

	Z ma, na, mb, nb;

	if (true) {
		ma = pqc::random_z_below(lea/2)*la;
		mb = pqc::random_z_below(leb/2)*lb;
		na = 1;
		nb = 1;
	} else {
		do {
			ma = pqc::random_z_below(lea);
			na = pqc::random_z_below(lea);
		} while ((ma % la) == 0 && (na % la) == 0);
		do {
			mb = pqc::random_z_below(leb);
			nb = pqc::random_z_below(leb);
		} while ((mb % lb) == 0 && (nb % lb) == 0);
	}
	
	WeierstrassPoint gen_a = ma*Pa + na*Qa, gen_b = mb*Pb + nb*Qb;

	if (false) {
		Z base(1), mul(la);
		for (int i = 0; i <= ea; ++i) {
			if ((base * Pa).is_identity()) {
				std::cout << "Pa is of order " << mul << "^" << i << "\n";
			}
			if ((base * Qa).is_identity()) {
				std::cout << "Qa is of order " << mul << "^" << i << "\n";
			}
			if ((base * gen_a).is_identity()) {
				std::cout << "gen_a is of order " << mul << "^" << i << "\n";
			}
			base *= mul;
		}

		base = 1;
		mul = lb;
		for (int i = 0; i <= eb; ++i) {
			if ((base * Pb).is_identity()) {
				std::cout << "Pb is of order " << mul << "^" << i << "\n";
			}
			if ((base * Qb).is_identity()) {
				std::cout << "Qb is of order " << mul << "^" << i << "\n";
			}
			if ((base * gen_b).is_identity()) {
				std::cout << "gen_b is of order " << mul << "^" << i << "\n";
			}
			base *= mul;
		}

		return;
	}

	if (false) {
		measure("A without strategy", 10, [&gen_a, la, ea, &Pb, &Qb]() {
			WeierstrassIsogeny iso_a(gen_a, la, ea);
			iso_a(Pb);
			iso_a(Qb);
		});
		measure("B without strategy", 10, [&gen_b, lb, eb, &Pa, &Qa]() {
			WeierstrassIsogeny iso_b(gen_b, lb, eb);
			iso_b(Pa);
			iso_b(Qa);
		});
		measure("A with strategy", 10, [&gen_a, la, ea, &Pb, &Qb, &strategy]() {
			WeierstrassIsogeny iso_a(gen_a, la, ea, strategy);
			iso_a(Pb);
			iso_a(Qb);
		});
		measure("V with strategy", 10, [&gen_b, lb, eb, &Pa, &Qa, &strategy]() {
			WeierstrassIsogeny iso_b(gen_b, lb, eb, strategy);
			iso_b(Pa);
			iso_b(Qa);
		});
		return;
	}

	WeierstrassIsogeny iso_a(gen_a, la, ea, strategy);
	WeierstrassIsogeny iso_b(gen_b, lb, eb, strategy);

	std::cout << "\n";
	std::cout << "E_A: " << *iso_a.image() << "\n";
	std::cout << "phi_A(P_B) = " << iso_a(Pb) << "\n";
	std::cout << "phi_A(Q_B) = " << iso_a(Qb) << "\n\n";
	std::cout << "E_B: " << *iso_b.image() << "\n";
	std::cout << "phi_B(P_A) = " << iso_b(Pa) << "\n";
	std::cout << "phi_B(Q_A) = " << iso_b(Qa) << "\n";
	std::cout << "\n";

	WeierstrassPoint gen_ab = ma*iso_b(Pa) + na*iso_b(Qa);
	WeierstrassPoint gen_ba = mb*iso_a(Pb) + nb*iso_a(Qb);

	std::cout << '\n';
	std::cout << "generator_ab = " << gen_ab << '\n';
	std::cout << "generator_ba = " << gen_ba << '\n';
	std::cout << '\n';

	WeierstrassIsogeny iso_ab(gen_ab, la, ea, strategy);
	WeierstrassIsogeny iso_ba(gen_ba, lb, eb, strategy);


	std::cout << "\n";
	std::cout << "E_AB: " << *iso_ab.image() << "\n";
	std::cout << "E_BA: " << *iso_ba.image() << "\n";
	std::cout << "\n";
	std::cout << "\n";
	std::cout << "j(E_AB) = " << iso_ab.image()->j_invariant() << "\n";
	std::cout << "j(E_BA) = " << iso_ba.image()->j_invariant() << "\n";
	std::cout << "\n";

/*	int i;
	for (i = 0; i < iso_ab.isogenies().size(); ++i) {
		std::cout << iso_ab.isogenies()[i].image()->j_invariant();
		if (i < iso_ba.isogenies().size())
			std::cout << "   " << iso_ba.isogenies()[i].image()->j_invariant();
		std::cout << "\n";
	}*/
//	iso_ab.print_j_invariants();
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
	//test_serialization();
	test_weierstrass();
	return 0;
}
#endif
