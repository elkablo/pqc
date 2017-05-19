#include <iostream>
#include <cstdio>
#include <pqc_gf.hpp>
#include <pqc_chacha.hpp>

namespace pqc
{

static bool get_system_entropy(void *out, size_t size)
{
	// TODO: use SYS_getrandom on Linux, genentropy on OpenBSD, ...
	FILE *fp = std::fopen("/dev/urandom", "rb");
	if (!fp)
		return false;
	size_t ret = std::fread(out, size, 1, fp);
	std::fclose(fp);
	return ret == 1;
}

void random_bytes(void *out, size_t size)
{
	static thread_local bool initialized = false;
	static thread_local chacha chacha;

	if (!initialized) {
		unsigned char key[40];
		if (!get_system_entropy(key, 40)) {
			std::cerr << "Unable to read from system entropy" << std::endl;
			std::abort();
		}
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
	if (limit <= 0)
		return -1;

	size_t k = limit.bit_length();
	Z result;
	do {
		result = random_z(k);
	} while (result >= limit);
	return result;
}

uint32_t random_u32_below(uint32_t limit)
{
	if (!limit) {
		// just no
		return 0;
	}

	uint32_t result;
	int clz = __builtin_clz(limit);

	do {
		random_bytes(&result, sizeof(result));
		result >>= clz;
	} while (result >= limit);
	return result;
}

}
