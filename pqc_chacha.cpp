#include <cstring>
#include <nettle/chacha.h>
#include <nettle/memxor.h>

#include "pqc_chacha.hpp"

namespace pqc
{

chacha::chacha() :
	buffer_fill_(0)
{
}

chacha::chacha(const void *key) :
	buffer_fill_(0)
{
	set_key(key);
}

void chacha::set_key(const void *key)
{
	buffer_fill_ = 0;
	chacha_set_key(&context_, static_cast<const uint8_t *>(key));
	chacha_set_nonce(&context_, &static_cast<const uint8_t *>(key)[32]);
}

static inline void mem_xor_or_copy(void *d, const void *s, size_t l, bool x) {
	if (x)
		memxor(d, s, l);
	else
		memcpy(d, s, l);
}

void chacha::generate(void *_out, size_t size, bool x)
{
	char *out = static_cast<char *>(_out);

	if (!size)
		return;

	if (buffer_fill_) {
		if (size <= buffer_fill_) {
			mem_xor_or_copy(out, buffer_, size, x);
			buffer_fill_ -= size;
			memmove(buffer_, buffer_ + size, buffer_fill_);
			return;
		} else {
			mem_xor_or_copy(out, buffer_, buffer_fill_, x);
			out += buffer_fill_;
			size -= buffer_fill_;
			buffer_fill_ = 0;
		}
	}

	while (size > 0) {
		uint32_t state[_CHACHA_STATE_LENGTH];

		_chacha_core(state, context_.state, 20);

		/* increment the counter */
		++context_.state[12];
		if (!context_.state[12])
			++context_.state[13];

		if (size < CHACHA_BLOCK_SIZE) {
			mem_xor_or_copy(out, state, size, x);
			buffer_fill_ = CHACHA_BLOCK_SIZE - size;
			memcpy(buffer_, reinterpret_cast<uint8_t *>(state) + size, buffer_fill_);
			return;
		}

		mem_xor_or_copy(out, state, CHACHA_BLOCK_SIZE, x);
		out += CHACHA_BLOCK_SIZE;
		size -= CHACHA_BLOCK_SIZE;
	}
}

}
