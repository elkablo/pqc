#include <nettle/sha2.h>

#include <pqc_cipher_chacha20.hpp>

namespace pqc
{

cipher_chacha20::operator enum pqc_cipher() const {
	return PQC_CIPHER_CHACHA20;
}

cipher_chacha20::cipher_chacha20()
{
}

size_t cipher_chacha20::key_size() const
{
	return 32;
}

size_t cipher_chacha20::nonce_size() const
{
	return 8;
}

void cipher_chacha20::key(const void *keyv, size_t size)
{
	if (size > 32) {
		char key[32];
		sha256_ctx ctx;

		sha256_init(&ctx);
		sha256_update(&ctx, size, static_cast<const uint8_t *>(keyv));
		sha256_digest(&ctx, 32, reinterpret_cast<uint8_t *>(key));

		chacha_.set_key(key);
	} else {
		chacha_.set_key(keyv);
	}
}

void cipher_chacha20::nonce(const void *noncev, size_t size)
{
	chacha_.set_nonce(noncev);
}

size_t cipher_chacha20::encrypt(void *output, size_t outlen, const void *input, size_t inlen)
{
	if (outlen < inlen)
		return inlen;

	memmove(output, input, outlen);
	chacha_.crypt(output, outlen);

	return inlen;
}

size_t cipher_chacha20::decrypt(void *output, size_t outlen, const void *input, size_t inlen)
{
	return encrypt(output, outlen, input, inlen);
}

}
