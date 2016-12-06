#include <cstring>
#include <pqc_mac_hmac_sha.hpp>

namespace pqc
{

hmac_sha256::operator enum pqc_mac() const
{
	return PQC_MAC_HMAC_SHA256;
}

size_t hmac_sha256::size() const
{
	return SHA256_DIGEST_SIZE;
}

void hmac_sha256::key(const void *keyv, size_t len)
{
	uint64_t key[8];
	if (len > 64) {
		sha256_ctx ctx;

		sha256_init(&ctx);
		sha256_update(&ctx, len, reinterpret_cast<const uint8_t *>(keyv));
		sha256_digest(&ctx, 32, reinterpret_cast<uint8_t *>(key));

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

void hmac_sha256::init()
{
	sha256_init(&context_);
	sha256_update(&context_, 64, reinterpret_cast<const uint8_t *>(i_pad_));
}

void hmac_sha256::update(const void *input, size_t len)
{
	sha256_update(&context_, len, static_cast<const uint8_t *>(input));
}

void hmac_sha256::digest(void *digest)
{
	sha256_digest(&context_, 32, reinterpret_cast<uint8_t *>(digest));

	sha256_init(&context_);
	sha256_update(&context_, 64, reinterpret_cast<const uint8_t *>(o_pad_));
	sha256_update(&context_, 32, reinterpret_cast<const uint8_t *>(digest));
	sha256_digest(&context_, 32, reinterpret_cast<uint8_t *>(digest));
}

hmac_sha512::operator enum pqc_mac() const
{
	return PQC_MAC_HMAC_SHA512;
}

size_t hmac_sha512::size() const
{
	return SHA512_DIGEST_SIZE;
}

void hmac_sha512::key(const void *keyv, size_t len)
{
	uint64_t key[16];
	if (len > 128) {
		sha512_ctx ctx;

		sha512_init(&ctx);
		sha512_update(&ctx, len, reinterpret_cast<const uint8_t *>(keyv));
		sha512_digest(&ctx, 64, reinterpret_cast<uint8_t *>(key));

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

void hmac_sha512::init()
{
	sha512_init(&context_);
	sha512_update(&context_, 128, reinterpret_cast<const uint8_t *>(i_pad_));
}

void hmac_sha512::update(const void *input, size_t len)
{
	sha512_update(&context_, len, static_cast<const uint8_t *>(input));
}

void hmac_sha512::digest(void *digest)
{
	sha512_digest(&context_, 64, reinterpret_cast<uint8_t *>(digest));

	sha512_init(&context_);
	sha512_update(&context_, 128, reinterpret_cast<const uint8_t *>(o_pad_));
	sha512_update(&context_, 64, reinterpret_cast<const uint8_t *>(digest));
	sha512_digest(&context_, 64, reinterpret_cast<uint8_t *>(digest));
}

}
