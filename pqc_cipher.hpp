#ifndef PQC_CIPHER_HPP
#define PQC_CIPHER_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <pqc_enumset.hpp>

namespace pqc
{

class cipher
{
public:
	virtual ~cipher() {}

	virtual size_t key_size() const = 0;
	virtual size_t nonce_size() const = 0;

	virtual void key(const void *, size_t) = 0;
	virtual void nonce(const void *, size_t) = 0;
	void key(const std::string&);
	void nonce(const std::string&);

	virtual void encrypt(void *, size_t) = 0;
	virtual void decrypt(void *, size_t) = 0;

	size_t encrypt(void *, size_t, const void *, size_t);
	size_t decrypt(void *, size_t, const void *, size_t);

	std::string encrypt(const std::string&);
	std::string decrypt(const std::string&);

	virtual operator pqc_cipher () const = 0;

	static std::shared_ptr<cipher> create(enum pqc_cipher);
	static enum pqc_cipher from_string(const char *str, size_t size);
	static const char *to_string(enum pqc_cipher);

	static constexpr enum pqc_cipher get_default() { return PQC_CIPHER_CHACHA20; }
	static constexpr cipherset enabled_default()
	{
		return cipherset(PQC_CIPHER_CHACHA20);
	}
};

}

#endif /* PQC_CIPHER_HPP */
