#include <cstring>
#include <pqc_kex.hpp>
#include <pqc_kex_sidhex.hpp>

namespace pqc
{

static const struct {
	enum pqc_kex kex;
	const char *name;
} kex_table[] = {
	{ PQC_KEX_SIDHex, "SIDHex" },
	{ PQC_KEX_UNKNOWN, NULL }
};

enum pqc_kex kex::from_string(const char *str, size_t size)
{
	for (int i = 0; kex_table[i].name; ++i)
		if (!strncasecmp (str, kex_table[i].name, size))
			return kex_table[i].kex;

	return PQC_KEX_UNKNOWN;
}

const char *kex::to_string(enum pqc_kex kex)
{
	for (int i = 0; kex_table[i].name; ++i)
		if (kex == kex_table[i].kex)
			return kex_table[i].name;

	return nullptr;
}

kex::kex(mode mode_) :
	mode_(mode_)
{
}

std::shared_ptr<kex> kex::create(enum pqc_kex type, mode mode_)
{
	switch (type) {
		case PQC_KEX_SIDHex:
			return std::make_shared<kex_sidhex>(mode_);
		default:
			return nullptr;
	}
}

}
