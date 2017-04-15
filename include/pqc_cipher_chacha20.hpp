#ifndef PQC_CIPHER_CHACHA20_HPP
#define PQC_CIPHER_CHACHA20_HPP

#include <pqc_cipher.hpp>
#include <pqc_chacha.hpp>

namespace pqc
{

class cipher_chacha20 : public cipher
{
public:
	cipher_chacha20();

	size_t key_size() const;

	void key(const void *, size_t);

	void encrypt(void *, size_t);
	void decrypt(void *, size_t);

	operator enum pqc_cipher () const;
private:
	chacha chacha_;
};

}

#endif /* PQC_CIPHER_CHACHA20_HPP */
