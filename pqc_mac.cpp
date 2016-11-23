#include <cstring>
#include <openssl/sha.h>

#include "pqc.hpp"

namespace pqc
{

void mac::compute(void *digest, const std::string& input)
{
	compute(digest, input.c_str(), input.size());
}

class hmac_sha256 : public mac
{
public:
	std::size_t size() const;
	void compute(void *, const void *, std::size_t);
	void key(const void *, std::size_t);
private:
	uint64_t o_pad_[8], i_pad_[8];
};

mac * mac::create_mac(enum pqc_mac type)
{
	switch (type) {
		case PQC_MAC_HMAC_SHA256:
			return new hmac_sha256();
		default:
			return nullptr;
	}
}

std::size_t hmac_sha256::size() const
{
	return 32;
}

void hmac_sha256::key(const void *keyv, std::size_t len)
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

void hmac_sha256::compute(void *digest, const void *input, std::size_t len)
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

}
