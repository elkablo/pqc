#ifndef PQC_KEX_HPP
#define PQC_KEX_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <pqc_enumset.hpp>

namespace pqc
{

class kex
{
public:
	enum class mode {
		SERVER,
		CLIENT
	};

	kex(mode);
	virtual ~kex() {}

	virtual std::string init() = 0;
	virtual std::string fini(const std::string&) = 0;

	static std::shared_ptr<kex> create(enum pqc_kex, mode);
	static enum pqc_kex from_string (const char *, size_t);
	static const char *to_string(enum pqc_kex);

	static constexpr enum pqc_kex get_default() { return PQC_KEX_SIDHex; }
	static constexpr kexset enabled_default()
	{
		return kexset(PQC_KEX_SIDHex);
	}
protected:
	mode mode_;
};

}

#endif /* PQC_KEX_HPP */
