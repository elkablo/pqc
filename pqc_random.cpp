#include <unistd.h>
#include <sys/syscall.h>
#include <gf.hpp>
#include <pqc_chacha.hpp>

namespace pqc
{

void random_bytes(void *out, size_t size)
{
	static thread_local bool initialized = false;
	static thread_local chacha chacha;

	if (!initialized) {
		unsigned char key[40];
		syscall(SYS_getrandom, key, 40, 0);
		chacha.set_key(key);
		initialized = true;
	}

	chacha.generate(out, size);
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
	return (bits + GMP_LIMB_BITS - 1) / GMP_LIMB_BITS;
}

Z random_z(size_t bits)
{
	Z result(0);
	size_t n = bits2limbs(bits);
	mp_limb_t array[n];
	random_bytes(array, sizeof(array));
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
