#include <cstring>

#include "pqc.hpp"

#include <openssl/chacha.h>

namespace pqc
{

std::string cipher::encrypt(const std::string& input)
{
	std::string output;
	size_t to_reserve = input.size();

	output.resize(to_reserve);

	to_reserve = encrypt(&output[0], output.capacity(), input.c_str(), input.size());
	if (to_reserve > output.capacity())
		output.resize(to_reserve);
	encrypt(&output[0], output.capacity(), input.c_str(), input.size());

	return output;
}

std::string cipher::decrypt(const std::string& input)
{
	std::string output;
	size_t to_reserve = input.size();

	output.resize(to_reserve);

	to_reserve = decrypt(&output[0], output.capacity(), input.c_str(), input.size());
	if (to_reserve > output.capacity())
		output.resize(to_reserve);
	decrypt(&output[0], output.capacity(), input.c_str(), input.size());

	return output;
}

class cipher_chacha20 : public cipher
{
public:
	cipher_chacha20();

	size_t key_size() const;
	size_t nonce_size() const;

	void key(const void *);
	void nonce(const void *);

	size_t encrypt(void *, size_t, const void *, size_t);
	size_t decrypt(void *, size_t, const void *, size_t);

	virtual operator enum pqc_cipher () const;
private:
	uint8_t key_[32], nonce_[8];
	uint64_t counter_;
};

cipher * cipher::create(enum pqc_cipher type)
{
	switch (type) {
		case PQC_CIPHER_CHACHA20:
			return new cipher_chacha20();
		default:
			return nullptr;
	}
}

static const struct {
	enum pqc_cipher cipher;
	const char *name;
} ciphers_table[] = {
	{ PQC_CIPHER_CHACHA20, "ChaCha20" },
	{ PQC_CIPHER_PLAIN, "plain" },
	{ PQC_CIPHER_UNKNOWN, NULL }
};

enum pqc_cipher cipher::from_string(const char *str, size_t size)
{
	for (int i = 0; ciphers_table[i].name; ++i)
		if (!strncasecmp (str, ciphers_table[i].name, size))
			return ciphers_table[i].cipher;

	return PQC_CIPHER_UNKNOWN;
}

const char *cipher::to_string(enum pqc_cipher cipher)
{
	for (int i = 0; ciphers_table[i].name; ++i)
		if (cipher == ciphers_table[i].cipher)
			return ciphers_table[i].name;

	return nullptr;
}

cipher_chacha20::operator enum pqc_cipher() const {
	return PQC_CIPHER_CHACHA20;
}

cipher_chacha20::cipher_chacha20() : counter_(0)
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

void cipher_chacha20::key(const void *keyv)
{
	::memcpy(key_, keyv, 32);
}

void cipher_chacha20::nonce(const void *noncev)
{
	::memcpy(nonce_, noncev, 8);
}

size_t cipher_chacha20::encrypt(void *output, size_t outlen, const void *input, size_t inlen)
{
	if (outlen < inlen)
		return inlen;

	CRYPTO_chacha_20(reinterpret_cast<unsigned char *>(output),
			 reinterpret_cast<const unsigned char *>(input),
			 inlen, key_, nonce_, counter_);

	counter_ += inlen;
	return inlen;
}

size_t cipher_chacha20::decrypt(void *output, size_t outlen, const void *input, size_t inlen)
{
	return encrypt(output, outlen, input, inlen);
}

}
