#ifndef PQC_HANDSHAKE_HPP
#define PQC_HANDSHAKE_HPP

#include <pqc_enumset.hpp>

namespace pqc
{

struct handshake {
	int version;
	char *server_name;
	enum pqc_kex kex;
	enum pqc_auth auth;
	cipherset supported_ciphers;
	macset supported_macs;
	char *server_auth;

	char **client_auths;
	size_t client_auths_len;

	char *secret, *secret_auth;

	enum pqc_cipher cipher;
	enum pqc_mac mac;
	char *nonce, *secret_auth_reply;

	handshake();
	~handshake();
	bool supports_cipher(enum pqc_cipher);
	bool supports_mac(enum pqc_mac);
	const char * parse_init(const char *);
	const char * parse_fini(const char *);
};

}

#endif /* PQC_HANDSHAKE_HPP */
