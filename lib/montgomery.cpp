#include <montgomery.hpp>

namespace pqc {

MontgomeryPoint MontgomeryCurve::zero () {
	GF z(A.get_p());
	return MontgomeryPoint(*this, z, z);
}

GF MontgomeryPoint::t1;
GF MontgomeryPoint::t2;
GF MontgomeryPoint::t3;

}
