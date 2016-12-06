#ifndef PQC_HPP
#define PQC_HPP

#include <cstdint>

enum pqc_cipher {
	PQC_CIPHER_UNKNOWN = 0,
	PQC_CIPHER_CHACHA20,
	PQC_CIPHER_PLAIN,

	PQC_CIPHER_FIRST = PQC_CIPHER_CHACHA20,
	PQC_CIPHER_LAST = PQC_CIPHER_PLAIN
};

#define for_each_pqc_cipher(var) \
	for (enum pqc_cipher var = PQC_CIPHER_FIRST; var <= PQC_CIPHER_LAST; ++*reinterpret_cast<int*>(&var))

enum pqc_auth {
	PQC_AUTH_UNKNOWN = 0,
	PQC_AUTH_SIDHex,

	PQC_AUTH_FIRST = PQC_AUTH_SIDHex,
	PQC_AUTH_LAST = PQC_AUTH_SIDHex
};

#define for_each_pqc_auth(var) \
	for (enum pqc_auth var = PQC_AUTH_FIRST; var <= PQC_AUTH_LAST; ++*reinterpret_cast<int*>(&var))

enum pqc_kex {
	PQC_KEX_UNKNOWN = 0,
	PQC_KEX_SIDHex,

	PQC_KEX_FIRST = PQC_KEX_SIDHex,
	PQC_KEX_LAST = PQC_KEX_SIDHex
};

#define for_each_pqc_kex(var) \
	for (enum pqc_kex var = PQC_KEX_FIRST; var <= PQC_KEX_LAST; ++*reinterpret_cast<int*>(&var))

enum pqc_mac {
	PQC_MAC_UNKNOWN = 0,
	PQC_MAC_HMAC_SHA256,
	PQC_MAC_HMAC_SHA512,

	PQC_MAC_FIRST = PQC_MAC_HMAC_SHA256,
	PQC_MAC_LAST = PQC_MAC_HMAC_SHA512
};

#define for_each_pqc_mac(var) \
	for (enum pqc_mac var = PQC_MAC_FIRST; var <= PQC_MAC_LAST; ++*reinterpret_cast<int*>(&var))

namespace pqc
{

typedef uint32_t ciphers_bitset;
typedef uint32_t macs_bitset;
typedef uint32_t kexes_bitset;
typedef uint32_t auths_bitset;

class cipher;
class kex;
class mac;
class session;

}

#endif /* PQC_HPP */
