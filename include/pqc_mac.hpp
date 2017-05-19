#ifndef PQC_MAC_HPP
#define PQC_MAC_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <pqc_enumset.hpp>

namespace pqc
{

class mac
{
public:
	virtual ~mac() {}
	virtual size_t size() const = 0;

	virtual void init() = 0;
	virtual void update(const void *, size_t) = 0;
	virtual void digest(void *) = 0;

	void update(const std::string&);
	std::string digest();

	void compute(void *, const void *, size_t);
	void compute(void *, const std::string&);
	std::string compute(const std::string&);
	std::string compute(const void *, size_t);

	virtual void key(const void *, size_t len) = 0;
	void key(const std::string&);

	virtual operator enum pqc_mac() const = 0;

	static std::shared_ptr<mac> create(enum pqc_mac);
	static enum pqc_mac from_string (const char *, size_t);
	static const char *to_string(enum pqc_mac);

	static constexpr enum pqc_mac get_default() { return PQC_MAC_HMAC_SHA512; }
	static constexpr macset enabled_default()
	{
		return macset(PQC_MAC_HMAC_SHA256, PQC_MAC_HMAC_SHA512);
	}
};

}

#endif /* PQC_MAC_HPP */
