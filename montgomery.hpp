#ifndef PQC_MONTGOMERY_HPP
#define PQC_MONTGOMERY_HPP

#include <iostream>

#include <gf.hpp>

namespace pqc {

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

}

#endif /* PQC_MONTGOMERY_HPP */
