#ifndef PQC_MAC_HMAC_SHA_HPP
#define PQC_MAC_HMAC_SHA_HPP

#include <cstring>
#include <openssl/sha.h>

#include <pqc_mac.hpp>

namespace pqc
{

class hmac_sha256 : public mac
{
public:
	size_t size() const;
	void compute(void *, const void *, size_t);
	void key(const void *, size_t);

	virtual operator enum pqc_mac() const;
private:
	uint64_t o_pad_[8], i_pad_[8];
};

class hmac_sha512 : public mac
{
public:
	size_t size() const;
	void compute(void *, const void *, size_t);
	void key(const void *, size_t);

	virtual operator enum pqc_mac() const;
private:
	uint64_t o_pad_[16], i_pad_[16];
};

}

#endif /* PQC_MAC_HMAC_SHA_HPP */
