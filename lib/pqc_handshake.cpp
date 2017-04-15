#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <pqc_handshake.hpp>
#include <pqc_cipher.hpp>
#include <pqc_kex.hpp>
#include <pqc_mac.hpp>
#include <pqc_auth.hpp>

namespace pqc {

handshake::handshake() :
	version(-1),
	server_name(nullptr),
	kex(PQC_KEX_UNKNOWN),
	auth(PQC_AUTH_UNKNOWN),
	supported_ciphers(),
	supported_macs(),
	server_auth(nullptr),
	client_auths(nullptr),
	client_auths_len(0),
	secret(nullptr),
	auth_request(nullptr),
	cipher(PQC_CIPHER_UNKNOWN),
	mac(PQC_MAC_UNKNOWN),
	nonce(nullptr),
	auth_reply(nullptr)
{}

handshake::~handshake()
{
	if (server_name)
		free (server_name);
	if (server_auth)
		free (server_auth);
	if (client_auths) {
		for (size_t i = 0; i < client_auths_len; ++i)
			free (client_auths[i]);
		free (client_auths);
	}
	if (secret)
		free (secret);
	if (auth_request)
		free (auth_request);
	if (nonce)
		free (nonce);
	if (auth_reply)
		free (auth_reply);
}

static bool has_prefix (const char *s, const char *p, const char ** n)
{
	size_t l = strlen (p);
	bool r = !strncasecmp (s, p, l);
	if (r && n)
		*n = s + l;
	return r;
}

static cipherset pqc_supported_ciphers_parse (const char *str, size_t size)
{
	cipherset result;
	const char *ptr = str, *space, *next;
	while (ptr < str + size) {
		size_t len;
		space = (const char *) memchr (ptr, ' ', size);
		if (space) {
			next = space + 1;
			len = space - str;
		} else {
			next = str + size;
			len = str + size - ptr;
		}
		result.set(cipher::from_string (ptr, len));
		ptr = next;
	}
	return result;
}

static macset pqc_supported_macs_parse (const char *str, size_t size)
{
	macset result;
	const char *ptr = str, *space, *next;
	while (ptr < str + size) {
		size_t len;
		space = (const char *) memchr (ptr, ' ', size);
		if (space) {
			next = space + 1;
			len = space - str;
		} else {
			next = str + size;
			len = str + size - ptr;
		}
		result.set(mac::from_string (ptr, len));
		ptr = next;
	}
	return result;
}

bool handshake::supports_cipher (enum pqc_cipher c)
{
	return (supported_ciphers & (1 << c));
}

bool handshake::supports_mac (enum pqc_mac m)
{
	return (supported_macs & (1 << m));
}

const char * handshake::parse_init(const char * input)
{
	const char *ptr = input, *t;

	if (!has_prefix (ptr, "Post-quantum hello v", &ptr))
		return nullptr;

	version = strtoul (ptr, (char **) &ptr, 10);
	if (errno == ERANGE)
		return nullptr;

	if (*ptr == ',') {
		ptr++;
		if (*ptr++ != ' ')
			return nullptr;

		t = strchr (ptr, '\n');
		if (!t || t[-1] != '.')
			return nullptr;

		server_name = strndup (ptr, t - ptr - 1);
		ptr = t + 1;
	} else if (*ptr == '.') {
		ptr++;
		if (*ptr++ != '\n')
			return nullptr;
	} else {
		return nullptr;
	}

	bool has_kex = false, has_auth = false, has_ciphers = false, has_macs = false;

	while (*ptr != '\0' && *ptr != '\n') {
		const char *nl = strchr (ptr, '\n');

		if (!nl)
			return nullptr;

		if (has_prefix (ptr, "Key-exchange: ", &ptr)) {
			if (has_kex)
				return nullptr;
			kex = kex::from_string (ptr, nl - ptr);
			has_kex = true;
		} else if (has_prefix (ptr, "Auth-type: ", &ptr)) {
			if (has_auth)
				return nullptr;
			auth = auth::from_string (ptr, nl - ptr);
			has_auth = true;
		} else if (has_prefix (ptr, "Supported-ciphers: ", &ptr)) {
			if (has_ciphers)
				return nullptr;
			supported_ciphers = pqc_supported_ciphers_parse (ptr, nl - ptr);
			has_ciphers = true;
		} else if (has_prefix (ptr, "Supported-MACs: ", &ptr)) {
			if (has_macs)
				return nullptr;
			supported_macs = pqc_supported_macs_parse (ptr, nl - ptr);
			has_macs = true;
		} else if (has_prefix (ptr, "Server-auth: ", &ptr)) {
			if (server_auth)
				return nullptr;
			server_auth = strndup (ptr, nl - ptr);
		} else if (has_prefix (ptr, "Client-auth: ", &ptr)) {
			char *auth = strndup (ptr, nl - ptr);
			char **tmp = (char **) realloc (client_auths, sizeof (*client_auths) * (client_auths_len + 1));
			if (!tmp) {
				free (auth);
				return nullptr;
			}
			client_auths = tmp;
			client_auths[client_auths_len++] = auth;
		} else if (has_prefix (ptr, "Secret: ", &ptr)) {
			if (secret)
				return nullptr;
			secret = strndup (ptr, nl - ptr);
		} else if (has_prefix (ptr, "Auth-request: ", &ptr)) {
			if (auth_request)
				return nullptr;
			auth_request = strndup (ptr, nl - ptr);
		} else {
			return nullptr;
		}
		ptr = nl + 1;
	}

	if (*ptr++ != '\n')
		return nullptr;

	return ptr;
}

const char * handshake::parse_fini (const char *input)
{
	const char *ptr = input;

	if (!has_prefix (ptr, "KEX: OK\n", &ptr))
		return nullptr;

	bool has_cipher = false, has_mac = false;

	while (*ptr != '\0' && *ptr != '\n') {
		const char *nl = strchr (ptr, '\n');

		if (!nl)
			return nullptr;

		if (has_prefix (ptr, "Cipher: ", &ptr)) {
			if (has_cipher)
				return nullptr;
			cipher = cipher::from_string(ptr, nl - ptr);
			if (cipher == PQC_CIPHER_UNKNOWN)
				return nullptr;
			has_cipher = true;
		} else if (has_prefix (ptr, "MAC: ", &ptr)) {
			if (has_mac)
				return nullptr;
			mac = mac::from_string(ptr, nl - ptr);
			if (mac == PQC_MAC_UNKNOWN)
				return nullptr;
			has_mac = true;
		} else if (has_prefix (ptr, "Nonce: ", &ptr)) {
			if (nonce)
				return nullptr;
			nonce = strndup (ptr, nl - ptr);
		} else if (has_prefix (ptr, "Auth-reply: ", &ptr)) {
			if (auth_reply)
				return nullptr;
			auth_reply = strndup (ptr, nl - ptr);
		} else {
			return nullptr;
		}
		ptr = nl + 1;
	}

	if (*ptr++ != '\n')
		return nullptr;

	return ptr;
}

}
