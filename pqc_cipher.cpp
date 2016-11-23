#include <cstring>

#include "pqc.hpp"

#include <openssl/chacha.h>

namespace pqc
{

std::string cipher::encrypt(const std::string& input)
{
	std::string output;
	std::size_t to_reserve = input.size();

	output.reserve(to_reserve);

	to_reserve = encrypt(&output[0], output.capacity(), input.c_str(), input.size());
	if (to_reserve > output.capacity())
		output.reserve(to_reserve);
	encrypt(&output[0], output.capacity(), input.c_str(), input.size());

	return output;
}

std::string cipher::decrypt(const std::string& input)
{
	std::string output;
	std::size_t to_reserve = input.size();

	output.reserve(to_reserve);

	to_reserve = decrypt(&output[0], output.capacity(), input.c_str(), input.size());
	if (to_reserve > output.capacity())
		output.reserve(to_reserve);
	decrypt(&output[0], output.capacity(), input.c_str(), input.size());

	return output;
}

class cipher_chacha20 : public cipher
{
public:
	cipher_chacha20();

	std::size_t key_size() const;
	std::size_t nonce_size() const;

	void key(const void *);
	void nonce(const void *);

	std::size_t encrypt(void *, std::size_t, const void *, std::size_t);
	std::size_t decrypt(void *, std::size_t, const void *, std::size_t);
private:
	uint8_t key_[32], nonce_[8];
	uint64_t counter_;
};

cipher * cipher::create_cipher(enum pqc_cipher type)
{
	switch (type) {
		case PQC_CIPHER_CHACHA20:
			return new cipher_chacha20();
		default:
			return nullptr;
	}
}

cipher_chacha20::cipher_chacha20() : counter_(0)
{
}

std::size_t cipher_chacha20::key_size() const
{
	return 32;
}

std::size_t cipher_chacha20::nonce_size() const
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

std::size_t cipher_chacha20::encrypt(void *output, std::size_t outlen, const void *input, std::size_t inlen)
{
	if (outlen < inlen)
		return inlen;

	CRYPTO_chacha_20(reinterpret_cast<unsigned char *>(output),
			 reinterpret_cast<const unsigned char *>(input),
			 inlen, key_, nonce_, counter_);

	counter_ += inlen;
	return inlen;
}

std::size_t cipher_chacha20::decrypt(void *output, std::size_t outlen, const void *input, std::size_t inlen)
{
	return encrypt(output, outlen, input, inlen);
}

}
