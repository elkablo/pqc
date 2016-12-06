#ifndef PQC_GF_HPP
#define PQC_GF_HPP

#include <functional>
#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>

#include "gmpxx.h"

class Z;
template<typename T> struct is_z {
	static constexpr bool value = std::is_integral<T>::value;
};
template<> struct is_z<Z> {
	static constexpr bool value = true;
};

class GF;
template<typename T> struct is_z_or_GF {
	static constexpr bool value = is_z<T>::value;
};
template<> struct is_z_or_GF<GF> {
	static constexpr bool value = true;
};

class Z : public mpz_class {
public:
	template<typename... Args>
	Z(Args... args) : mpz_class(args...) {}

	operator mpz_ptr () {
		return get_mpz_t();
	}

	operator mpz_srcptr () const {
		return get_mpz_t();
	}

	Z& operator%=(const Z& other) {
		::mpz_mod(*this, *this, other);
		return *this;
	}

	Z& operator%=(const unsigned int other) {
		::mpz_mod_ui(*this, *this, other);
		return *this;
	}

	Z& addmul(const Z& a, const Z& b) {
		::mpz_addmul(*this, a, b);
		return *this;
	}

	Z& submul(const Z& a, const Z& b) {
		::mpz_submul(*this, a, b);
		return *this;
	}

	int invert(const Z& other) {
		return ::mpz_invert(*this, *this, other);
	}

	Z pow(unsigned long int exp) {
		Z res;
		::mpz_pow_ui(res, *this, exp);
		return res;
	}

	Z powmod(unsigned long int exp, const Z& mod) {
		Z res;
		::mpz_powm_ui(res, *this, exp, mod);
		return res;
	}

	Z powmod(const Z& exp, const Z& mod) {
		Z res;
		::mpz_powm(res, *this, exp, mod);
		return res;
	}

	bool probably_prime() const {
		return ::mpz_probab_prime_p(*this, 10) != 0;
	}

	Z& sqrtmod(const Z& p) {
		Z e = (p+1) >> 2;
		::mpz_powm(*this, *this, e, p);
		return *this;
	}

	bool is_square(const Z& p) const {
		Z e = (p-1) >> 1;
		::mpz_powm(e, *this, e, p);
		return e == 1;
	}

	std::size_t bit_length() const {
		return ::mpz_sizeinbase(*this, 2);
	}

	std::size_t size() const {
		return ::mpz_sizeinbase(*this, 256);
	}

	bool testbit (std::size_t index) const {
		return ::mpz_tstbit(*this, index);
	}

	std::string serialize(size_t) const;
	void unserialize(const std::string&);
};

class GF {
public:
	const Z* p;
	Z a, b;
	static Z t1, t2, t3;

public:
	static bool check(const Z& p) {
		return p.probably_prime() && p % 4 == 3;
	}

	const Z& get_p() const {
		return *p;
	}

	GF() : p(nullptr), a(0), b(0) {}

	GF(const Z& _p) : p(&_p), a(0), b(0) {}

	template<typename Ta, typename = std::enable_if_t<is_z<Ta>::value>>
	GF(const Z& _p, const Ta& _a) : p(&_p), a(_a), b(0) {
		a %= *p;
	}

	template<typename Ta, typename Tb,
		 typename = std::enable_if_t<is_z<Ta>::value>,
		 typename = std::enable_if_t<is_z<Tb>::value>>
	GF(const Z& _p, const Ta& _a, const Tb& _b) : p(&_p), a(_a), b(_b) {
		a %= *p;
		b %= *p;
	}

	GF(const Z& _p, const char *_a) : p(&_p), a(_a) {
		a %= *p;
	}

	GF(const Z& _p, const char *_a, const char *_b) : p(&_p), a(_a), b(_b) {
		a %= *p;
		b %= *p;
	}

	GF& operator+=(const GF& other) {
		a += other.a;
		a %= *p;
		b += other.b;
		b %= *p;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	GF& operator+=(const T& other) {
		a += other;
		a %= *p;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z_or_GF<T>::value>>
	GF operator+(const T& other) const {
		GF res(*this);
		res += other;
		return res;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend GF operator+(const T& other, const GF& self) {
		return self + other;
	}

	GF& operator+() {
		return *this;
	}

	GF& operator-=(const GF& other) {
		a -= other.a;
		b -= other.b;
		a %= *p;
		b %= *p;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	GF& operator-=(const T& other) {
		a -= other;
		a %= *p;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z_or_GF<T>::value>>
	GF operator-(const T& other) const {
		GF res(*this);
		res -= other;
		return res;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend GF operator-(const T& other, const GF& self) {
		return self - other;
	}

	GF& negate() {
		if (sgn(a) != 0) {
			/* This is equivalent to a = p - a,
			   but does everything inplace. */
			a -= *p;
			a = -a;
		}
		if (sgn(b) != 0) {
			b -= *p;
			b = -b;
		}
		return *this;
	}

	GF operator-() const {
		GF res(*this);
		res.negate();
		return res;
	}

	inline GF& operator++() {
		++a;
		a %= *p;
		return *this;
	}

	inline GF operator++(int) {
		GF res(*this);
		++res;
		return res;
	}

	inline GF& operator--() {
		--a;
		a %= *p;
		return *this;
	}

	inline GF operator--(int) {
		GF res(*this);
		--res;
		return res;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	GF& operator*=(const T& other) {
		a *= other;
		a %= *p;
		b *= other;
		b %= *p;
		return *this;
	}

	/* We want to compute the remainder of this·other modulo (x² + 1),
	   where this = a + bx
	   and  other = c + dx.
	
	   this·other = (a + bx)·(c + dx) = bdx² + (ad + bc)·x + ac.
	
	   Now divide
	     (bdx² + (ad + bc)·x + ac) : (x² + 1) = bd + ...
	     -bdx²               - bd
	     -------------------------
	       (ad + bc)·x + (ac - bd)
	
	   Thus the result is   (ac - bd) + (ad + bc)·x
	   Computing (ac - bd) and (ad + bc) trivially needs 4 multiplications,
	   but we can compute T = (a-b)·(c+d) = (ac + ad - bc - bd) with one
	   multiplication, X = ad and Y = bc with 2 others, and then we have
	   the degree 0 coefficient
	     ac - bd = T - X + Y,
	   and the degree 1 coefficient 
	     ad + bc = X + Y.
	   */
	GF& operator*=(const GF& other) {
		const Z &c = other.a, &d = other.b;

		if (0) {
			t1 = a*c - b*d;
			t2 = a*d + b*c;
			a = t1;
			a %= *p;
			b = t2;
			b %= *p;
			return *this;
		}

		t1 = a - b;
		t2 = c + d;
		t3 = (t1 * t2);
		t1 = a * d;
		t2 = b * c;
		a = t3 - t1 + t2;
		a %= *p;
		b = t1 + t2;
		b %= *p;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z_or_GF<T>::value>>
	GF operator*(const T& other) const {
		GF res(*this);
		res *= other;
		return res;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend GF operator*(const T& other, const GF& self) {
		return self * other;
	}

	/* Substituting into the result of multiplication the result should be
	     2abx + (a²-b²) = (ab + ab)·x + (a+b)·(a-b) = (X + X)·x + Y·Z,
	   where
	     X = ab, Y = (a + b) and Z = (a - b),
	   thus we need only two multiplications.  */
	GF& square_inplace() {
		t1 = a + b;
		t2 = a - b;
		b *= a;
		b += b;
		b %= *p;
		a = t1*t2;
		a %= *p;
		return *this;
	}

	GF square() const {
		GF res(*this);
		res.square_inplace();
		return res;
	}

	/* We want to compute 1/(a+bx)
	     1 / (a + bx) = (a - bx) / [(a + bx)·(a - bx)]
	                  = (a - bx) / (a² - b²x²)
	
	   Now the denominator modulo (x² + 1) is
	     (-b²x² + a²) : (x² + 1) = -b² + ...
	      +b²x² + b²
	     ------------
	         a² + b²
	
	   Thus the result is (a - bx) / (a² + b²).  */
	bool inverse_inplace() {
		t1 = a*a;
		/* t1 += b*b;   addmul is faster */
		t1.addmul(b, b);
		t2 = t1;
		if (t2.invert(*p) == 0)
			return false;

		a *= t2;
		a %= *p;
		b = -b;
		b *= t2;
		b %= *p;
		return true;
	}

	GF inverse() const {
		GF res(*this);
		res.inverse_inplace();
		return res;
	}

	GF& operator/=(const GF& other) {
		*this *= other.inverse();
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	GF& operator/=(const T& other) {
		Z d(other);
		d.invert(*p);
		*this *= d;
		return *this;
	}

	template<typename T, typename = std::enable_if_t<is_z_or_GF<T>::value>>
	GF operator/(const T& other) const {
		GF res(*this);
		res /= other;
		return res;
	}

	/* We have
	     (a + bx)² = c + dx,
	   that is
	     a² + 2abx + b²x² = c + dx.
	
	   To get rid of x² we compute the remainder by (x² + 1)
	      b²x²      : (x² + 1) = b² + ...
	     -b²x² - b²
	     ----------
	           - b².
	
	   So our equation is
	     a² - b² + 2abx = c + dx.
	   Comparing coefficients gives
	     a² - b² = c
	         2ab = d
	   Substituting b = d/(2a) into the first equation leads to
	     a² - d²/(4a²) = c
	     4a⁴ - 4ca² - d² = 0
	   This has the solutions
	     a² = (c ± sqrt(c² + d²)) / 2
	   Thus we at first try
	     a² = (c + sqrt(c² + d²)) / 2,
	   and if this is not a square in the field over p, we try
	     a² = (c - sqrt(c² + d²)) / 2.
	   */
	GF& sqrt() {
		t1 = a*a;
		t1.addmul(b, b);
		t1.sqrtmod(*p);
		t3 = 2;
		t3.invert(*p);
		t2 = ((t1 + a) * t3) % *p;
		if (!t2.is_square(*p))
			t2 = ((t1 - a) * t3) % *p;
		t2.sqrtmod(*p);
		a = t2;
		t2 <<= 1;
		t2.invert(*p);
		b *= t2;
		b %= *p;
		return *this;
	}

	GF pow(const Z& exp) const {
		GF q(*this);
		GF res = exp.testbit(0) ? *this : GF(*p, 1, 0);
		for (std::size_t i = 1; i < exp.bit_length(); ++i) {
			q.square_inplace();
			if (exp.testbit(i))
				res *= q;
		}
		return res;
	}

	bool is_square() const {
		Z exp = (*p * *p - 1) >> 1;
		return this->pow(exp) == 1;
	}

	size_t size() const {
		return 2 * p->size();
	}

	std::string serialize() const;
	bool unserialize(const std::string&);

	inline operator bool() const {
		return sgn(a) != 0 || sgn(b) != 0;
	}

	inline bool operator!() const {
		return sgn(a) == 0 && sgn(b) == 0;
	}

	inline bool operator==(const GF& other) const {
		return p == other.p && a == other.a && b == other.b;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	inline bool operator==(const T& other) const {
		return sgn(b) == 0 && a == other;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend inline bool operator==(const T& other, const GF& self) {
		return self == other;
	}

	inline bool operator!=(const GF& other) const {
		return p != other.p || a != other.a || b != other.b;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	inline bool operator!=(const T& other) const {
		return sgn(b) != 0 || a != other;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend inline bool operator!=(const T& other, const GF& self) {
		return self != other;
	}

	inline bool operator>=(const GF& other) const {
		return b >= other.b || (b == other.b && a >= other.a);
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	inline bool operator>=(const T& other) const {
		return sgn(b) > 0 || (sgn(b) == 0 && a >= other);
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend inline bool operator<=(const T& other, const GF& self) {
		return sgn(self.b) > 0 || (sgn(self.b) == 0 && self.a >= other);
	}

	inline bool operator>(const GF& other) const {
		return b > other.b || (b == other.b && a > other.a);
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	inline bool operator>(const T& other) const {
		return sgn(b) > 0 || (sgn(b) == 0 && a > other);
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend inline bool operator<(const T& other, const GF& self) {
		return sgn(self.b) > 0 || (sgn(self.b) == 0 && self.a > other);
	}

	inline bool operator<=(const GF& other) const {
		return b <= other.b || (b == other.b && a <= other.a);
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	inline bool operator<=(const T& other) const {
		return sgn(b) == 0 && a <= other;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend inline bool operator>=(const T& other, const GF& self) {
		return self.b == 0 && self.a <= other;
	}

	inline bool operator<(const GF& other) const {
		return b < other.b || (b == other.b && a < other.a);
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	inline bool operator<(const T& other) const {
		return sgn(b) == 0 && a < other;
	}

	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	friend inline bool operator>(const T& other, const GF& self) {
		return sgn(self.b) == 0 && self.a < other;
	}

	friend std::ostream& operator<<(std::ostream& os, const GF& gf) {
		if (gf.a && gf.b)
			os << "(" << gf.a << " + " << gf.b << "·i)";
		else if (gf.a)
			os << gf.a;
		else if (gf.b)
			os << gf.b << "·i";
		else
			os << "0";
		return os;
	}
};

class MontgomeryPoint;

class MontgomeryCurve {
	GF A, B, A24;
public:
	MontgomeryCurve(const GF& _A, const GF& _B) : A(_A), B(_B), A24(A) {
		A24 += 2;
		A24 /= 4;
	}

	MontgomeryPoint zero();

	friend std::ostream& operator<<(std::ostream& os, const MontgomeryCurve& curve) {
		os << curve.B << " y² = x³ + " << curve.A << "·x² + x";
		return os;
	}

	friend class MontgomeryPoint;
};

class MontgomeryPoint {
	const MontgomeryCurve& curve;
	GF X, Z;
	static GF t1, t2, t3;
public:
	MontgomeryPoint(const MontgomeryCurve& _curve, const GF& _X, const GF& _Z) :
		curve(_curve), X(_X), Z(_Z) {}

private:
	/* http://wstein.org/edu/124/misc/montgomery.pdf strana 261
	   also from wikipedia https://en.wikipedia.org/wiki/Montgomery_curve
	   */
	MontgomeryPoint& do_diff_add(const MontgomeryPoint& other, const MontgomeryPoint& diff) {
		t1 = (X + Z)*(other.X - other.Z);
		t2 = (X - Z)*(other.X + other.Z);
		X = diff.Z * (t1 + t2).square_inplace();
		Z = diff.X * (t1 - t2).square_inplace();
		return *this;
	}

	MontgomeryPoint& do_double() {
		t1 = (X + Z).square_inplace();
		t2 = (X - Z).square_inplace();
		t3 = t1 - t2;
		X = t1 * t2;
		Z = t3 * (t2 + curve.A24 * t3);
		return *this;
	}

public:
	template<typename T, typename = std::enable_if_t<is_z<T>::value>>
	MontgomeryPoint& operator*=(const T& n) {
		MontgomeryPoint t1, t2;

		if (!Z)
			return *this;

		if (!n) {
			Z = 0;
		} else if (n == 2) {
			do_double();
		} else if (n == 3) {
			t1 = *this;          // t1 = P
			do_double();         // this = 2P
			do_diff_add(t1, t1); // this = this+t1 = 2P+P = 3P
		} else if (n == 5) {
			t1 = *this;          // t1 = P
			do_double();         // this = 2P
			t2 = *this;          // t2 = 2P
			do_diff_add(t1, t1); // this = this + t1 = 2P+P = 3P
			do_diff_add(t2, t1); // this = this + t2 = 3P+2P = 5P
		} else {
			// mont_3ladder, pretoze chceme pocitat P + m¯¹nQ
			// takze cely tento kod pojde dopice
		}
		return *this;
	}
};

#endif /* PQC_GF_HPP */
