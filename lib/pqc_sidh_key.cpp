#include <pqc_random.hpp>
#include <pqc_sidh_key.hpp>

namespace pqc
{

sidh_key::sidh_key(const sidh_params& params) :
	sidh_key_basic(params)
{
}

sidh_key::~sidh_key()
{
}

const std::string& sidh_key::get_hash_seed() const
{
	return hash_seed_;
}

void sidh_key::generate_hash_seed()
{
	hash_seed_.resize(hash_seed_size);
	random_bytes(&hash_seed_[0], hash_seed_size);
}

bool sidh_key::generate_private()
{
	if (sidh_key_basic::generate_private()) {
		generate_hash_seed();
		return true;
	} else {
		return false;
	}
}

bool sidh_key::generate_public()
{
	return sidh_key_basic::generate_public();
}

void sidh_key::generate()
{
	sidh_key_basic::generate();
	generate_hash_seed();
}

std::string sidh_key::export_private() const
{
	std::string basic(sidh_key_basic::export_private());

	if (!basic.size())
		return basic;
	else
		return basic + hash_seed_;
}

std::string sidh_key::export_public() const
{
	std::string basic(sidh_key_basic::export_public());

	if (!basic.size())
		return basic;
	else
		return basic + hash_seed_;
}

std::string sidh_key::export_both() const
{
	std::string basic(sidh_key_basic::export_both());

	if (!basic.size())
		return basic;
	else
		return basic + hash_seed_;
}

bool sidh_key::import_private(const std::string& input)
{
	if (input.size() <= hash_seed_size)
		return false;

	size_t basic_size = input.size() - hash_seed_size;

	if (!sidh_key_basic::import_private(input.substr(0, basic_size)))
		return false;

	hash_seed_ = input.substr(basic_size);
	return true;
}

bool sidh_key::import_public(const std::string& input)
{
	if (input.size() <= hash_seed_size)
		return false;

	size_t basic_size = input.size() - hash_seed_size;

	if (!sidh_key_basic::import_public(input.substr(0, basic_size)))
		return false;

	hash_seed_ = input.substr(basic_size);
	return true;
}

bool sidh_key::import(const std::string& input)
{
	if (input.size() <= hash_seed_size)
		return false;

	size_t basic_size = input.size() - hash_seed_size;

	if (!sidh_key_basic::import(input.substr(0, basic_size)))
		return false;

	hash_seed_ = input.substr(basic_size);
	return true;
}

}
