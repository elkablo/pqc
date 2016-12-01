#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/syscall.h>

#include "pqc.hpp"

#include <openssl/chacha.h>

namespace pqc
{

void random_bytes(char *buf, size_t size)
{
	static thread_local bool initialized = false;
	static thread_local unsigned char key[40];
	static thread_local uint64_t counter = 0;

	if (!initialized) {
		syscall(SYS_getrandom, buf, 40, 0);
		printf ("random seed = %i\n", *(int *) buf);
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

}
