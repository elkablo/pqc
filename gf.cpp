#include "gf.hpp"

std::string Z::serialize(size_t len) const
{
	unsigned char *buffer;
	std::string result;

	result.resize(len);
	buffer = reinterpret_cast<unsigned char *>(&result[0]);

	for (size_t i = 0; i < len; ++i) {
		buffer[i] = Z((*this >> (i*8)) & 0xff).get_si();
	}

	return result;
}

void Z::unserialize(const std::string& raw)
{
	const unsigned char *buffer;
	*this = 0;

	buffer = reinterpret_cast<const unsigned char *>(&raw[0]);
	for (ssize_t i = raw.size()-1; i >= 0; --i) {
		*this |= buffer[i];
		*this <<= 8;
	}
}

std::string GF::serialize() const
{

}

bool GF::unserialize(const std::string& raw)
{

}

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
