#include "gf.hpp"

MontgomeryPoint MontgomeryCurve::zero () {
	GF z(A.get_p());
	return MontgomeryPoint(*this, z, z);
}

Z GF::t1;
Z GF::t2;
Z GF::t3;

GF MontgomeryPoint::t1;
GF MontgomeryPoint::t2;
GF MontgomeryPoint::t3;
