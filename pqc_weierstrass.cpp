#include <pqc_weierstrass.hpp>
#include <pqc_random.hpp>

namespace pqc {

WeierstrassSmallIsogeny WeierstrassCurve::small_isogeny (const WeierstrassPoint& generator, int l) const
{
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

WeierstrassPoint WeierstrassCurve::random_point() const
{
	GF x(a.get_p()), y(a.get_p());

	do {
		x.a = random_z_below(x.get_p());
		x.b = random_z_below(x.get_p());
		y = (x.square() + a)*x + b;
	} while (!y.is_square());

	y.sqrt();

	if (random_u32_below(2))
		y = -y;

	return WeierstrassPoint(shared_from_this(), x, y);
}

WeierstrassPoint WeierstrassCurve::torsion_point(const Z& cofactor, const Z& factor_div_p) const
{
	WeierstrassPoint P;
	do {
		P = random_point() * cofactor;
	} while ((P * factor_div_p).is_identity());
	return P;
}

std::pair<WeierstrassPoint, WeierstrassPoint> WeierstrassCurve::basis(int la, int ea, int lb, int eb, int f) const
{
	Z cofactor = Z(lb).pow(eb)*f;
	Z factor_div_p = Z(la).pow(ea-1);
	Z factor = factor_div_p * la;

	WeierstrassPoint P = torsion_point(cofactor, factor_div_p), Q;
	do {
		Q = torsion_point(cofactor, factor_div_p);
	} while (P.weil_pairing(Q, factor).pow(factor_div_p) == 1);
	return std::make_pair(P, Q);
}

GF WeierstrassPoint::line(const WeierstrassPoint& R, const WeierstrassPoint& Q) const
{
	const Z& p = m_curve->a.get_p();
	const WeierstrassPoint& P = *this;

	if (Q.is_identity())
		return GF(p);

	if (P.is_identity() || R.is_identity()) {
		if (P == R)
			return GF(p, 1);
		else if (P.is_identity())
			return Q.x - R.x;
		else
			return Q.x - P.x;
	} else if (P != R) {
		if (P.x == R.x) {
			return Q.x - P.x;
		} else {
			GF l = (R.y - P.y) / (R.x - P.x);
			return (Q.y - P.y) - l*(Q.x - P.x);
		}
	} else {
		if (P.y == 0) {
			return Q.x - P.x;
		} else {
			GF l = (3*P.x.square() + m_curve->a) / (2*P.y);
			return (Q.y - P.y) - l*(Q.x - P.x);
		}
	}
}

GF WeierstrassPoint::miller(const WeierstrassPoint& Q, Z n) const
{
	const Z& p = m_curve->a.get_p();
	const WeierstrassPoint& P = *this;

	if (Q.is_identity() || n == 0)
		return GF(p);

	bool neg = n < 0;
	if (neg)
		n = -n;

	GF t(p, 1), l, v;
	WeierstrassPoint V(P), S(2*V);

	for (std::ptrdiff_t i = n.bit_length() - 2; i >= 0; --i) {
		S = 2*V;
		l = V.line(V, Q);
		v = S.line(-S, Q);
		t = t.square() * (l/v);
		V = S;
		if (n.testbit(i)) {
			S = V + P;
			l = V.line(P, Q);
			v = S.line(-S, Q);
			t *= (l/v);
			V = S;
		}
	}

	if (neg) {
		v = V.line(-V, Q);
		t = (t*v).inverse();
	}

	return t;
}

GF WeierstrassPoint::weil_pairing(const WeierstrassPoint& Q, const Z& n) const
{
	const Z& p = m_curve->a.get_p();
	const WeierstrassPoint& P = *this;

	if (!(P*n).is_identity() || !(Q*n).is_identity())
		return GF(p);

	if (P == Q || P.is_identity() || Q.is_identity())
		return GF(p, 1);

	GF denominator = Q.miller(P, n);
	if (denominator == 0)
		return GF(p, 1);

	GF numerator = P.miller(Q, n);
	if (n.testbit(0))
		numerator = -numerator;
	return numerator / denominator;
}

}
