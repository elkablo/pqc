#ifndef PQC_AUTH_HPP
#define PQC_AUTH_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <utility>
#include <pqc_enumset.hpp>
#include <pqc_asymmetric_key.hpp>

namespace pqc
{

class auth
{
public:
	virtual ~auth();

	virtual std::shared_ptr<asymmetric_key> generate_key() const = 0;

	virtual bool set_request_key(const std::string&) = 0;
	virtual bool set_sign_key(const std::string&) = 0;

	virtual bool can_request() const = 0;
	virtual bool can_sign() const = 0;

	virtual std::string request(const std::string&) = 0;
	virtual std::string sign(const std::string&, const std::string&) = 0;
	virtual bool verify(const std::string&) = 0;

	static std::shared_ptr<auth> create(enum pqc_auth);
	static enum pqc_auth from_string(const char *, size_t);
	static const char *to_string(enum pqc_auth);

	static constexpr enum pqc_auth get_default() { return PQC_AUTH_SIDHex_SHA512; }
	static constexpr authset enabled_default()
	{
		return authset(PQC_AUTH_SIDHex_SHA512);
	}
};

}

#endif /* PQC_AUTH_HPP */
