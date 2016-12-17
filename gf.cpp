#include <gf.hpp>

namespace pqc {

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
		*this <<= 8;
		*this |= buffer[i];
	}
}

std::string GF::serialize() const
{
	size_t half = size() / 2;
	return a.serialize(half) + b.serialize(half);
}

bool GF::unserialize(const std::string& raw)
{
	size_t half = size() / 2;
	if (raw.size() != 2*half)
		return false;
	a.unserialize(raw.substr(0, half));
	b.unserialize(raw.substr(half));
	return true;
}

Z GF::t1;
Z GF::t2;
Z GF::t3;

}
