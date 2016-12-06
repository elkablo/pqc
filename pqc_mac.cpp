#include <cstring>

#include <pqc.hpp>
#include <pqc_mac.hpp>
#include <pqc_mac_hmac_sha.hpp>

namespace pqc
{

void mac::update(const std::string& input)
{
	update(input.c_str(), input.size());
}

std::string mac::digest()
{
	std::string result(size(), '\0');
	digest(&result[0]);
	return result;
}

void mac::compute(void *result, const void *input, size_t len)
{
	init();
	update(input, len);
	digest(result);
}

void mac::compute(void *result, const std::string& input)
{
	compute(result, input.c_str(), input.size());
}

std::string mac::compute(const void *input, size_t len)
{
	init();
	update(input, len);
	return digest();
}

std::string mac::compute(const std::string& input)
{
	return compute(input.c_str(), input.size());
}

void mac::key(const std::string& val)
{
	key(reinterpret_cast<const void *>(val.c_str()), val.size());
}

std::shared_ptr<mac> mac::create(enum pqc_mac type)
{
	switch (type) {
		case PQC_MAC_HMAC_SHA256:
			return std::make_shared<hmac_sha256>();
		case PQC_MAC_HMAC_SHA512:
			return std::make_shared<hmac_sha512>();
		default:
			return nullptr;
	}
}

static const struct {
	enum pqc_mac mac;
	const char *name;
} mac_table[] = {
	{ PQC_MAC_HMAC_SHA256, "sha256" },
	{ PQC_MAC_HMAC_SHA512, "sha512" },
	{ PQC_MAC_UNKNOWN, NULL }
};

enum pqc_mac mac::from_string(const char *str, size_t size)
{
	for (int i = 0; mac_table[i].name; ++i)
		if (!strncasecmp (str, mac_table[i].name, size))
			return mac_table[i].mac;

	return PQC_MAC_UNKNOWN;
}

const char *mac::to_string(enum pqc_mac mac)
{
	for (int i = 0; mac_table[i].name; ++i)
		if (mac == mac_table[i].mac)
			return mac_table[i].name;

	return nullptr;
}

}
