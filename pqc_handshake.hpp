#ifndef PQC_HANDSHAKE_HPP
#define PQC_HANDSHAKE_HPP

#include <pqc.hpp>

namespace pqc
{

struct handshake {
	int version;
	char *server_name;
	enum pqc_kex kex;
	ciphers_bitset supported_ciphers;
	macs_bitset supported_macs;
	char *server_auth;

	char **client_auths;
	size_t client_auths_len;

	char *encrypted_secret;

	enum pqc_cipher cipher;
	enum pqc_mac mac;
	char *nonce;

	handshake();
	~handshake();
	bool supports_cipher(enum pqc_cipher);
	bool supports_mac(enum pqc_mac);
	const char * parse_init(const char *);
	const char * parse_fini(const char *);
};

}

#endif /* PQC_HANDSHAKE_HPP */
