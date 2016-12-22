#include <nettle/memxor.h>
#include <pqc_base64.hpp>
#include <pqc_auth_sidhex.hpp>

namespace pqc
{

auth_sidhex::auth_sidhex(const std::shared_ptr<mac>& mac) :
	mac_(mac),
	request_key_(sidh_params(sidh_params::side::A)),
	sign_key_(sidh_params(sidh_params::side::A))
{
}

auth_sidhex::~auth_sidhex()
{
}

std::shared_ptr<asymmetric_key> auth_sidhex::generate_key() const
{
	auto key = std::make_shared<sidh_key>(sidh_params::side::A);
	key->generate();
	return key;
}

bool auth_sidhex::set_request_key(const std::string& key)
{
	return request_key_.import(key) && request_key_.has_public();
}

bool auth_sidhex::set_sign_key(const std::string& key)
{
	return sign_key_.import(key) && sign_key_.has_private();
}

bool auth_sidhex::can_request() const
{
	return request_key_.has_public();
}

bool auth_sidhex::can_sign() const
{
	return sign_key_.has_private();
}

std::string auth_sidhex::request(const std::string& message)
{
	if (!request_key_.has_public())
		return std::string();

	sidh_key_basic priv_key(request_key_.get_params().other_side());
	priv_key.generate();

	mac_->key(request_key_.get_hash_seed());

	mac_->init();
	mac_->update(message);
	mac_->update(priv_key.compute_shared_secret(request_key_));
	secret_ = mac_->digest();

	return priv_key.export_public();
}

std::string auth_sidhex::sign(const std::string& message, const std::string& request)
{
	if (!sign_key_.has_private())
		return std::string();

	sidh_key_basic peer_key(sign_key_.get_params().other_side());

	if (!peer_key.import_public(request))
		return std::string();

	mac_->key(request_key_.get_hash_seed());

	mac_->init();
	mac_->update(message);
	mac_->update(sign_key_.compute_shared_secret(peer_key));
	return mac_->digest();
}

bool auth_sidhex::verify(const std::string& reply)
{
	return secret_.size() != 0 && reply == secret_;
}

}
