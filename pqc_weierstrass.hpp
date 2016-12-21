#ifndef PQC_WEIERSTRASS_HPP
#define PQC_WEIERSTRASS_HPP

#include <memory>
#include <vector>
#include <gf.hpp>

namespace pqc {

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

	WeierstrassPoint(const Z& p) :
		m_curve(std::make_shared<WeierstrassCurve>(p)), x(p), y(p), identity(true) {}

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

class WeierstrassIsogeny {
	WeierstrassPoint m_generator;
	int m_base, m_exp;
	std::vector<WeierstrassSmallIsogeny> m_isogenies;
public:
	WeierstrassIsogeny() {}

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

}

#endif /* PQC_WEIERSTRASS_HPP */
