#ifndef PQC_AUTH_HPP
#define PQC_AUTH_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <utility>
#include <pqc_enumset.hpp>

namespace pqc
{

class auth
{
public:
	auth();
	virtual ~auth() {}

	virtual std::string request(const std::string&);

	static std::shared_ptr<auth> create(enum pqc_auth);
	static enum pqc_auth from_string(const char *, size_t);
	static const char *to_string(enum pqc_auth);

	static constexpr enum pqc_auth get_default() { return PQC_AUTH_SIDHex; }
	static constexpr authset enabled_default()
	{
		return authset(PQC_AUTH_SIDHex);
	}
};

}

#endif /* PQC_AUTH_HPP */
