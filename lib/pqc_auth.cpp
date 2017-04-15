#include <cstring>
#include <pqc_auth.hpp>
#include <pqc_auth_sidhex.hpp>
#include <pqc_mac.hpp>

namespace pqc
{

auth::~auth()
{
}

std::shared_ptr<auth> auth::create(enum pqc_auth type)
{
	switch (type) {
		case PQC_AUTH_SIDHex_SHA512:
			return std::make_shared<auth_sidhex>(mac::create(PQC_MAC_HMAC_SHA512));
		default:
			return nullptr;
	}
}

static const struct {
	enum pqc_auth auth;
	const char *name;
} auth_table[] = {
	{ PQC_AUTH_SIDHex_SHA512, "SIDHex-sha512" },
	{ PQC_AUTH_UNKNOWN, NULL }
};

enum pqc_auth auth::from_string(const char *str, size_t size)
{
	for (int i = 0; auth_table[i].name; ++i)
		if (!strncasecmp (str, auth_table[i].name, size))
			return auth_table[i].auth;

	return PQC_AUTH_UNKNOWN;
}

enum pqc_auth auth::from_string(const char *str)
{
	return from_string(str, ::strlen(str));
}

const char *auth::to_string(enum pqc_auth auth)
{
	for (int i = 0; auth_table[i].name; ++i)
		if (auth == auth_table[i].auth)
			return auth_table[i].name;

	return nullptr;
}

}
