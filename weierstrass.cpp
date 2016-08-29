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

class WeierstrassPoint;
class WeierstrassSmallIsogeny;

class WeierstrassCurve {
	GF a, b;
public:
	friend class WeierstrassPoint;
	friend class WeierstrassSmallIsogeny;

	WeierstrassCurve(const GF& _a, const GF& _b) : a(_a), b(_b) {}

	GF j_invariant() const {
		GF a3m4 = 4*a.square()*a;
		return 1728 * a3m4 / (a3m4 + 27*b.square());
	}

	WeierstrassSmallIsogeny small_isogeny (const WeierstrassPoint& generator, int l) const;

	friend std::ostream& operator<<(std::ostream& os, const WeierstrassCurve& curve) {
		os << "y² = x³ + (" << curve.a << ")·x + (" << curve.b << ")";
		return os;
	}
};

typedef std::shared_ptr<const WeierstrassCurve> WeierstrassCurvePtr;

class WeierstrassPoint {
	WeierstrassCurvePtr m_curve;
	GF x, y;
	bool identity;
public:
	friend class WeierstrassCurve;
	friend class WeierstrassSmallIsogeny;

	WeierstrassPoint(const WeierstrassCurvePtr& curve, const GF& x, const GF& y) :
		m_curve(curve), x(x), y(y), identity(false) {}

	WeierstrassPoint(const WeierstrassCurvePtr& curve) : m_curve(curve), identity(true) {}

	const WeierstrassCurvePtr& curve() const {
		return m_curve;
	}

	bool check() const {
		return identity || y.square() == ((x.square() + m_curve->a)*x + m_curve->b);
	}

	bool is_identity() const {
		return identity;
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
	}

	WeierstrassPoint& operator-=(const WeierstrassPoint& other) {
		*this += -other;
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
	return WeierstrassSmallIsogeny(std::make_shared<const WeierstrassCurve>(a - 5*t, b - 7*w), generator, l);
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

void test_weierstrass () {
	Z p("3700444163740528325594401040305817124863");
	WeierstrassCurvePtr E = std::make_shared<const WeierstrassCurve>(GF(p, 1), GF(p, 0));
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

	WeierstrassPoint gen_a = ma*Pa + na*Qa, gen_b = mb*Pb + nb*Qb;

	Z base(1), mul(2);
	for (int i = 0; i < 64; ++i) {
		if ((base * gen_a).is_identity()) {
			std::cout << "gen_a is of order " << mul << "^" << i << "\n";
		}
		base *= mul;
	}

	base = 1, mul = 3;
	for (int i = 0; i < 42; ++i) {
		if ((base * gen_b).is_identity()) {
			std::cout << "gen_b is of order " << mul << "^" << i << "\n";
		}
		base *= mul;
	}

	WeierstrassIsogeny iso_a(gen_a, 2, 63);
	WeierstrassIsogeny iso_b(gen_b, 3, 41);

	std::cout << "\n";
	std::cout << "E_A: " << *iso_a.image() << "\n";
	std::cout << "E_B: " << *iso_b.image() << "\n";
	std::cout << "\n";

	WeierstrassIsogeny iso_ab(ma*iso_b(Pa) + na*iso_b(Qa), 2, 63);
	WeierstrassIsogeny iso_ba(mb*iso_a(Pb) + nb*iso_a(Qb), 3, 41);

	std::cout << "\n";
	std::cout << "E_AB: " << *iso_ab.image() << "\n";
	std::cout << "E_BA: " << *iso_ba.image() << "\n";
	std::cout << "\n";

	std::cout << "\n";
	std::cout << "j(E_AB) = " << iso_ab.image()->j_invariant() << "\n";
	std::cout << "j(E_BA) = " << iso_ba.image()->j_invariant() << "\n";
	std::cout << "\n";

	int i;
	for (i = 0; i < iso_ab.isogenies().size(); ++i) {
		std::cout << iso_ab.isogenies()[i].image()->j_invariant();
		if (i < iso_ba.isogenies().size())
			std::cout << "   " << iso_ba.isogenies()[i].image()->j_invariant();
		std::cout << "\n";
	}
//	iso_ab.print_j_invariants();
}

int main (int argc, char ** argv) {
	test_weierstrass();
	return 0;
}
