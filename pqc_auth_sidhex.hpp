#ifndef PQC_AUTH_SIDHEX_HPP
#define PQC_AUTH_SIDHEX_HPP

#include <string>
#include <memory>
#include <pqc_auth.hpp>
#include <pqc_sidh_key.hpp>
#include <pqc_mac.hpp>

namespace pqc
{

class auth_sidhex : public auth
{
public:
	auth_sidhex() = delete;
	auth_sidhex(const std::shared_ptr<mac>&);
	~auth_sidhex();

	std::shared_ptr<asymmetric_key> generate_key() const;

	bool set_request_key(const std::string&);
	bool set_sign_key(const std::string&);

	bool can_request() const;
	bool can_sign() const;

	std::string request(const std::string&);
	std::string sign(const std::string&, const std::string&);
	bool verify(const std::string&);

private:
	std::shared_ptr<mac> mac_;
	sidh_key request_key_, sign_key_;
	std::string secret_;
};

}

#endif /* PQC_AUTH_SIDHEX_HPP */
