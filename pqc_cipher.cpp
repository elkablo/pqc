#include <cstring>

#include <pqc_cipher.hpp>
#include <pqc_cipher_chacha20.hpp>

namespace pqc
{

size_t cipher::encrypt(void *out, size_t olen, const void *in, size_t ilen)
{
	if (olen > ilen)
		olen = ilen;

	memmove(out, in, olen);
	encrypt(out, olen);
	return olen;
}

size_t cipher::decrypt(void *out, size_t olen, const void *in, size_t ilen)
{
	if (olen > ilen)
		olen = ilen;

	memmove(out, in, olen);
	decrypt(out, olen);
	return olen;
}

std::string cipher::encrypt(const std::string& input)
{
	std::string output;
	size_t size = input.size();

	output.resize(size);

	size = encrypt(&output[0], output.size(), input.c_str(), input.size());
	if (size > output.size()) {
		output.resize(size);
		encrypt(&output[0], output.size(), input.c_str(), input.size());
	}

	return output;
}

std::string cipher::decrypt(const std::string& input)
{
	std::string output;
	size_t size = input.size();

	output.resize(size);

	size = decrypt(&output[0], output.size(), input.c_str(), input.size());
	if (size > output.size()) {
		output.resize(size);
		decrypt(&output[0], output.size(), input.c_str(), input.size());
	}

	return output;
}

void cipher::key(const std::string& val)
{
	key(val.c_str(), val.size());
}

std::shared_ptr<cipher> cipher::create(enum pqc_cipher type)
{
	switch (type) {
		case PQC_CIPHER_CHACHA20:
			return std::make_shared<cipher_chacha20>();
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

}
