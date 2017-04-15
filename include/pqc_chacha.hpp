#ifndef PQC_CHACHA_HPP
#define PQC_CHACHA_HPP

#include <nettle/chacha.h>

namespace pqc
{

class chacha {
public:
	chacha();
	chacha(const void *);
	void set_key(const void *);
	void generate(void *, size_t, bool x = false);
	void crypt(void *out, size_t size) { generate(out, size, true); }
private:
	size_t buffer_fill_;
	char buffer_[CHACHA_BLOCK_SIZE];
	struct chacha_ctx context_;
};

}

#endif /* PQC_CHACHA_HPP */
