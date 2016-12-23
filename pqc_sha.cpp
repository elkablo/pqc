#include <nettle/sha.h>
#include <pqc_sha.hpp>

namespace pqc
{

static void hexify(void *ptr, size_t len)
{
	static const uint8_t hexchars[16] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
	};

	uint8_t *p = reinterpret_cast<uint8_t *>(ptr);
	uint8_t *in = p + len - 1;
	uint8_t *out = p + 2*len - 2;

	for (int i = 0; i < len; ++i, --in, out -= 2) {
		uint8_t t = *in;
		out[0] = hexchars[t >> 4];
		out[1] = hexchars[t & 15];
	}
}

std::string bin2hex(const std::string& input)
{
	std::string result(input);
	result.resize(input.size()*2);
	hexify(&result[0], input.size());
	return result;
}

std::string sha256(const std::string& input, bool hex)
{
	std::string digest;
	sha256_ctx ctx;

	digest.resize(hex ? SHA256_DIGEST_SIZE * 2 : SHA256_DIGEST_SIZE);

	sha256_init(&ctx);
	sha256_update(&ctx, input.size(), reinterpret_cast<const uint8_t *>(input.c_str()));
	sha256_digest(&ctx, SHA256_DIGEST_SIZE, reinterpret_cast<uint8_t *>(&digest[0]));

	if (hex)
		hexify(&digest[0], SHA256_DIGEST_SIZE);

	return digest;
}

std::string sha512(const std::string& input, bool hex)
{
	std::string digest;
	sha512_ctx ctx;

	digest.resize(hex ? SHA512_DIGEST_SIZE * 2 : SHA512_DIGEST_SIZE);

	sha512_init(&ctx);
	sha512_update(&ctx, input.size(), reinterpret_cast<const uint8_t *>(input.c_str()));
	sha512_digest(&ctx, SHA512_DIGEST_SIZE, reinterpret_cast<uint8_t *>(&digest[0]));

	if (hex)
		hexify(&digest[0], SHA512_DIGEST_SIZE);

	return digest;
}

}
