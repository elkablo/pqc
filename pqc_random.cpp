#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/syscall.h>

#include "pqc.hpp"
#include "gf.hpp"

#include <openssl/chacha.h>

namespace pqc
{

void random_bytes(char *buf, size_t size)
{
	static thread_local bool initialized = false;
	static thread_local unsigned char key[40];
	static thread_local uint64_t counter = 0;

	if (!initialized) {
		syscall(SYS_getrandom, key, 40, 0);
		initialized = true;
	}

	memset (buf, 0, size);

	CRYPTO_chacha_20(reinterpret_cast<unsigned char *>(buf),
			 reinterpret_cast<const unsigned char *>(buf),
			 size, key, key + 32, counter);

	counter += size;
}

std::string random_string(size_t size)
{
	std::string result;
	result.resize(size);
	random_bytes(&result[0], size);
	return result;
}

static inline size_t bits2limbs (size_t bits)
{
	return (((bits + (size_t) (GMP_LIMB_BITS-1)) & ~(size_t) (GMP_LIMB_BITS-1)) / GMP_LIMB_BITS);
}

Z random_z(size_t bits)
{
	Z result(0);
	size_t n = bits2limbs(bits);
	mp_limb_t array[n];
	random_bytes(reinterpret_cast<char *>(array), sizeof(array));
	for (size_t i = 0; i < n; ++i) {
		result <<= sizeof(mp_limb_t)*8;
		result |= array[i];
	}
	result = result >> (n*GMP_LIMB_BITS - bits);
	return result;
}

Z random_z_below(const Z& limit)
{
	size_t k = limit.bit_length();
	Z result;
	do {
		result = random_z(k);
	} while (result >= limit);
	return result;
}

}
