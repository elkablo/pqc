#include <cstring>
#include <openssl/sha.h>

#include <pqc.hpp>
#include <pqc_mac.hpp>

namespace pqc
{

void mac::compute(void *digest, const std::string& input)
{
	compute(digest, input.c_str(), input.size());
}

std::string mac::compute(const void *input, size_t len)
{
	std::string result(size(), 0);
	compute(&result[0], input, len);
	return result;
}

std::string mac::compute(const std::string& input)
{
	std::string result(size(), 0);
	compute(&result[0], input.c_str(), input.size());
	return result;
}

void mac::key(const std::string& val)
{
	key(reinterpret_cast<const void *>(val.c_str()), val.size());
}

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

std::shared_ptr<mac> mac::create(enum pqc_mac type)
{
	switch (type) {
		case PQC_MAC_HMAC_SHA256:
			return std::make_shared<hmac_sha256>();
		case PQC_MAC_HMAC_SHA512:
			return std::make_shared<hmac_sha512>();
		default:
			return nullptr;
	}
}

static const struct {
	enum pqc_mac mac;
	const char *name;
} mac_table[] = {
	{ PQC_MAC_HMAC_SHA256, "sha256" },
	{ PQC_MAC_HMAC_SHA512, "sha512" },
	{ PQC_MAC_UNKNOWN, NULL }
};

enum pqc_mac mac::from_string(const char *str, size_t size)
{
	for (int i = 0; mac_table[i].name; ++i)
		if (!strncasecmp (str, mac_table[i].name, size))
			return mac_table[i].mac;

	return PQC_MAC_UNKNOWN;
}

const char *mac::to_string(enum pqc_mac mac)
{
	for (int i = 0; mac_table[i].name; ++i)
		if (mac == mac_table[i].mac)
			return mac_table[i].name;

	return nullptr;
}

hmac_sha256::operator enum pqc_mac() const
{
	return PQC_MAC_HMAC_SHA256;
}

size_t hmac_sha256::size() const
{
	return 32;
}

void hmac_sha256::key(const void *keyv, size_t len)
{
	uint64_t key[8];
	if (len > 64) {
		SHA256_CTX ctx;

		SHA256_Init(&ctx);
		SHA256_Update(&ctx, keyv, len);
		SHA256_Final(reinterpret_cast<unsigned char *>(key), &ctx);

		len = 32;
	} else {
		::memcpy(key, keyv, len);
	}

	if (len < 64) {
		::memset(&reinterpret_cast<uint8_t *>(key)[len], 0, 64-len);
	}

	for (int i = 0; i < 8; ++i) {
		o_pad_[i] = key[i] ^ 0x5c5c5c5c5c5c5c5cULL;
		i_pad_[i] = key[i] ^ 0x3636363636363636ULL;
	}
}

void hmac_sha256::compute(void *digest, const void *input, size_t len)
{
	SHA256_CTX ctx;

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, reinterpret_cast<const void *>(i_pad_), 64);
	SHA256_Update(&ctx, input, len);
	SHA256_Final(reinterpret_cast<unsigned char *>(digest), &ctx);

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, reinterpret_cast<const void *>(o_pad_), 64);
	SHA256_Update(&ctx, digest, 32);
	SHA256_Final(reinterpret_cast<unsigned char *>(digest), &ctx);
}

hmac_sha512::operator enum pqc_mac() const
{
	return PQC_MAC_HMAC_SHA512;
}

size_t hmac_sha512::size() const
{
	return 64;
}

void hmac_sha512::key(const void *keyv, size_t len)
{
	uint64_t key[16];
	if (len > 128) {
		SHA512_CTX ctx;

		SHA512_Init(&ctx);
		SHA512_Update(&ctx, keyv, len);
		SHA512_Final(reinterpret_cast<unsigned char *>(key), &ctx);

		len = 64;
	} else {
		::memcpy(key, keyv, len);
	}

	if (len < 128) {
		::memset(&reinterpret_cast<uint8_t *>(key)[len], 0, 128-len);
	}

	for (int i = 0; i < 16; ++i) {
		o_pad_[i] = key[i] ^ 0x5c5c5c5c5c5c5c5cULL;
		i_pad_[i] = key[i] ^ 0x3636363636363636ULL;
	}
}

void hmac_sha512::compute(void *digest, const void *input, size_t len)
{
	SHA512_CTX ctx;

	SHA512_Init(&ctx);
	SHA512_Update(&ctx, reinterpret_cast<const void *>(i_pad_), 128);
	SHA512_Update(&ctx, input, len);
	SHA512_Final(reinterpret_cast<unsigned char *>(digest), &ctx);

	SHA512_Init(&ctx);
	SHA512_Update(&ctx, reinterpret_cast<const void *>(o_pad_), 128);
	SHA512_Update(&ctx, digest, 64);
	SHA512_Final(reinterpret_cast<unsigned char *>(digest), &ctx);
}

}
