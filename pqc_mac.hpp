#ifndef PQC_MAC_HPP
#define PQC_MAC_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <pqc.hpp>

namespace pqc
{

class mac
{
public:
	virtual ~mac() {}
	virtual size_t size() const = 0;
	virtual void compute(void *, const void *, size_t) = 0;
	virtual void compute(void *, const std::string&);
	virtual std::string compute(const std::string&);
	virtual std::string compute(const void *, size_t);

	virtual void key(const void *, size_t len) = 0;
	virtual void key(const std::string&);

	virtual operator enum pqc_mac() const = 0;

	static std::shared_ptr<mac> create(enum pqc_mac);
	static enum pqc_mac from_string (const char *, size_t);
	static const char *to_string(enum pqc_mac);

	static constexpr enum pqc_mac get_default() { return PQC_MAC_HMAC_SHA512; }
	static constexpr macs_bitset enabled_default()
	{
		return (1 << PQC_MAC_HMAC_SHA256) | (1 << PQC_MAC_HMAC_SHA512);
	}
};

}

#endif /* PQC_MAC_HPP */
