#ifndef PQC_SIDH_KEY_HPP
#define PQC_SIDH_KEY_HPP

#include <cstddef>
#include <pqc_sidh_key_basic.hpp>

namespace pqc
{

class sidh_key : public sidh_key_basic
{
public:
	static const size_t hash_seed_size = 32;

	sidh_key(const sidh_params&);
	~sidh_key();

	std::string export_private() const;
	std::string export_public() const;
	std::string export_both() const;

	bool import_private(const std::string&);
	bool import_public(const std::string&);
	bool import(const std::string&);

	bool generate_private();
	bool generate_public();
	void generate();

	const std::string& get_hash_seed() const;
private:
	void generate_hash_seed();

	std::string hash_seed_;
};

}

#endif /* PQC_SIDH_KEY_HPP */
