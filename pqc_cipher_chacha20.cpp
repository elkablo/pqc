#include <cstring>
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
	return 40;
}

void cipher_chacha20::key(const void *keyv, size_t size)
{
	char key[40];

	if (size > 40)
		size = 40;

	::memcpy(key, keyv, size);

	if (size < 40)
		::memset(&key[size], 0, 40 - size);

	chacha_.set_key(key);
}

void cipher_chacha20::encrypt(void *data, size_t len)
{
	chacha_.crypt(data, len);
}

void cipher_chacha20::decrypt(void *data, size_t len)
{
	chacha_.crypt(data, len);
}

}
