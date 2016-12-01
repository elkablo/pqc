#include <iostream>
#include <string>
#include <nettle/base64.h>

#include "pqc.hpp"

namespace pqc
{

std::string base64_encode(const std::string& input)
{
	struct base64_encode_ctx ctx;
	size_t len = BASE64_ENCODE_LENGTH(input.size()) + BASE64_ENCODE_FINAL_LENGTH;
	std::string result;
	size_t r;

	result.resize(len);

	base64_encode_init(&ctx);
	r = base64_encode_update(
		&ctx,
		reinterpret_cast<unsigned char *>(&result[0]),
		input.size(),
		reinterpret_cast<const unsigned char *>(&input[0])
	);
	r += base64_encode_final(
		&ctx,
		reinterpret_cast<unsigned char *>(&result[0]) + r
	);
	result.resize(r);
	return result;
}

std::string base64_decode(const std::string& input)
{
	struct base64_decode_ctx ctx;
	std::string result;
	size_t len = BASE64_DECODE_LENGTH(input.size())+1;
	int r;

	result.resize(len);

	base64_decode_init(&ctx);
	r = base64_decode_update(&ctx, &len,
			     reinterpret_cast<unsigned char *>(&result[0]),
			     input.size(),
			     reinterpret_cast<const unsigned char *>(&input[0])
	);
	if (!r)
		return "";
	r = base64_decode_final(&ctx);
	if (!r)
		return "";

	result.resize(len+1);

	return result;
}

}
