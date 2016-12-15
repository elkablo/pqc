#include <sstream>

#include <pqc_session.hpp>
#include <pqc_handshake.hpp>
#include <pqc_cipher.hpp>
#include <pqc_kex.hpp>
#include <pqc_mac.hpp>
#include <pqc_random.hpp>
#include <pqc_base64.hpp>
#include <pqc_packet.hpp>

namespace pqc
{

session::session() :
	error_(error::NONE),
	state_(state::INIT),
	mode_(mode::NONE),
	peer_closed_(false),
	rekey_after_(1024*1024*1024),
	since_last_rekey_(0),
	since_last_peer_rekey_(0),
	enabled_ciphers_(cipher::enabled_default()),
	enabled_auths_(),
	enabled_macs_(mac::enabled_default()),
	enabled_kexes_(kex::enabled_default()),
	use_kex_(kex::get_default())
{}

session::~session()
{
}

void session::cipher_enable(enum pqc_cipher cipher, bool enable)
{
	enabled_ciphers_.set(cipher, enable);
}

bool session::is_cipher_enabled(enum pqc_cipher cipher) const
{
	return enabled_ciphers_.isset(cipher);
}

void session::mac_enable(enum pqc_mac mac, bool enable)
{
	enabled_macs_.set(mac, enable);
}

bool session::is_mac_enabled(enum pqc_mac mac) const
{
	return enabled_macs_.isset(mac);
}

void session::auth_enable(enum pqc_auth auth, bool enable)
{
	enabled_auths_.set(auth, enable);
}

bool session::is_auth_enabled(enum pqc_auth auth) const
{
	return enabled_auths_.isset(auth);
}

void session::kex_enable(enum pqc_kex kex, bool enable)
{
	enabled_kexes_.set(kex, enable);
}

bool session::is_kex_enabled(enum pqc_kex kex) const
{
	return enabled_kexes_.isset(kex);
}

const std::string& session::get_server_name() const {
	return server_name_;
}

void session::set_server_auth(const char *auth)
{
	server_auth_ = auth;
}

void session::set_auth(const char *auth)
{
	std::string tmp(auth);
	auth_callback_ = [tmp](const char *){
		return tmp.c_str();
	};
}

void session::set_auth_callback(const auth_callback_t& auth_callback)
{
	auth_callback_ = auth_callback;
}

void session::set_rekey_after(size_t set)
{
	rekey_after_ = set;
}

size_t session::get_rekey_after() const
{
	return rekey_after_;
}

session::error session::error_code() const
{
	return error_;
}

bool session::is_error() const
{
	return error_ != error::NONE;
}

size_t session::bytes_available() const
{
	return incoming_.size();
}

size_t session::bytes_outgoing_available() const
{
	return outgoing_.size();
}

bool session::is_handshaken() const
{
	return state_ >= state::NORMAL;
}

bool session::is_closed() const
{
	return state_ == state::CLOSED;
}

bool session::is_peer_closed() const
{
	return peer_closed_;
}

void session::set_error(error err)
{
	error_ = err;
	state_ = state::ERROR;
}

size_t session::since_last_rekey() const
{
	return since_last_rekey_;
}

size_t session::since_last_peer_rekey() const
{
	return since_last_peer_rekey_;
}

void session::handle_handshake(const char *buf, size_t size)
{
	incoming_handshake_.append(buf, size);

	while (incoming_handshake_.size()) {
		if (state_ == state::INIT) {
			bool has_nn = incoming_handshake_.find("\n\n") != std::string::npos;
			if (!has_nn && incoming_handshake_.size() > 4096)
				return set_error(error::BAD_HANDSHAKE);

			if (!has_nn)
				return;

			const char *ptr;
			handshake handshake;

			ptr = handshake.parse_init(incoming_handshake_.c_str());
			if (!ptr)
				return set_error(error::BAD_HANDSHAKE);
			incoming_handshake_.erase(0, ptr - incoming_handshake_.c_str());

			cipherset available_ciphers = handshake.supported_ciphers & enabled_ciphers_;
			macset available_macs = handshake.supported_macs & enabled_macs_;

			if (handshake.version != 1
				|| handshake.kex == PQC_KEX_UNKNOWN
				|| (mode_ == mode::CLIENT && use_kex_ != handshake.kex)
				|| !is_kex_enabled(handshake.kex)
				|| !available_ciphers
				|| !available_macs
				|| !handshake.encrypted_secret
			)
				return set_error(error::BAD_HANDSHAKE);

			cipher_ = cipher::create(*available_ciphers.begin());
			mac_ = mac::create(*available_macs.begin());

			std::string encrypted_secret;

			if (mode_ == mode::SERVER) {
				use_kex_ = handshake.kex;
				kex_ = kex::create(use_kex_, kex::mode::SERVER);
				encrypted_secret = kex_->init();
			}

			session_key_ = kex_->fini(handshake.encrypted_secret);
			if (!session_key_.size())
				return set_error(error::BAD_HANDSHAKE);

			std::string nonce(random_string(min_nonce_size));
			mac_->key(nonce);

			ephemeral_key_ = mac_->compute(session_key_);

			cipher_->key(ephemeral_key_);

			if (mode_ == mode::SERVER) {
				send_handshake_init(encrypted_secret);
			}
			send_handshake_fini(nonce);

			state_ = state::HANDSHAKING;
		} else if (state_ == state::HANDSHAKING) {
			bool has_nn = incoming_handshake_.find("\n\n") != std::string::npos;
			if (!has_nn && incoming_handshake_.size() > 4096)
				return set_error(error::BAD_HANDSHAKE);

			if (!has_nn)
				return;

			const char *ptr;
			handshake handshake;

			ptr = handshake.parse_fini(incoming_handshake_.c_str());
			if (!ptr)
				return set_error(error::BAD_HANDSHAKE);

			incoming_handshake_.erase(0, ptr - incoming_handshake_.c_str());

			if (handshake.cipher == PQC_CIPHER_UNKNOWN
				|| !is_cipher_enabled(handshake.cipher)
				|| handshake.mac == PQC_MAC_UNKNOWN
				|| !is_mac_enabled(handshake.mac)
				|| !handshake.nonce
			)
				return set_error(error::BAD_HANDSHAKE);

			peer_mac_ = mac::create(handshake.mac);
			peer_cipher_ = cipher::create(handshake.cipher);

			std::string peer_nonce = base64_decode(handshake.nonce);

			if (peer_nonce.size() < min_nonce_size)
				return set_error(error::BAD_HANDSHAKE);

			peer_mac_->key(peer_nonce);

			peer_ephemeral_key_ = peer_mac_->compute(session_key_);

			peer_cipher_->key(peer_ephemeral_key_);

			packet_reader_.set_mac(peer_mac_);
			packet_reader_.set_cipher(peer_cipher_);

			if (outgoing_.size() > 0)
				state_ = state::HANDSHAKING_TILL_SENT;
			else
				state_ = state::NORMAL;
		} else if (state_ == state::NORMAL) {
			handle_incoming(incoming_handshake_.c_str(), incoming_handshake_.size());
			incoming_handshake_.erase();
		} else {
			// TODO: assert(false);
			break;
		}
	}
}

void session::handle_incoming_close()
{
	peer_closed_ = true;
	if (state_ == state::CLOSING) {
		state_ = state::CLOSED;
		std::cout << "\tremote closed - all closed\n";
	} else {
		std::cout << "\tremote closed\n";
	}
}

void session::handle_incoming_data(const char *buf, size_t size)
{
	incoming_.append(buf, size);
	since_last_peer_rekey_ += size;
}

void session::handle_incoming_rekey(const char *buf, size_t size)
{
	if (size < min_nonce_size)
		return set_error(error::BAD_REKEY);

	std::cout << "\tREKEY peer\n";
	peer_mac_->key(buf, size);
	peer_ephemeral_key_ = peer_mac_->compute(peer_ephemeral_key_);
	peer_cipher_->key(peer_ephemeral_key_);

	since_last_peer_rekey_ = 0;
}

void session::handle_incoming(const char *buf, size_t size)
{
	packet_reader_.write_incoming(buf, size);

	for (const packet *pkt = packet_reader_.get_packet();
	     pkt != nullptr;
	     packet_reader_.pop_packet(), pkt = packet_reader_.get_packet()) {

		if (!pkt->verify())
			return set_error(error::BAD_MAC);

		switch (pkt->get_type()) {
		case packet::type::CLOSE:
			handle_incoming_close();
			break;
		case packet::type::DATA:
			handle_incoming_data(pkt->get_data(), pkt->get_data_size());
			break;
		case packet::type::REKEY:
			handle_incoming_rekey(pkt->get_data(), pkt->get_data_size());
			break;
		}

		if (is_error())
			return;
	}

	if (packet_reader_.is_error())
		set_error(error::BAD_PACKET);
}

void session::write_incoming(const char *buf, size_t size)
{
	if (peer_closed_)
		set_error(error::ALREADY_CLOSED);

	if (state_ == state::ERROR)
		return;

	if (state_ < state::NORMAL)
		handle_handshake(buf, size);
	else
		handle_incoming(buf, size);
}

void session::do_rekey()
{
	std::string nonce(random_string(min_nonce_size));

	rekey_packet pkt(outgoing_, mac_);

	pkt.set_data(nonce.c_str(), nonce.size());
	pkt.sign();
	pkt.encrypt(cipher_);

	mac_->key(nonce);
	ephemeral_key_ = mac_->compute(ephemeral_key_);
	cipher_->key(ephemeral_key_);

	std::cout << "\tREKEY\n";

	since_last_rekey_ = 0;
}

void session::write_packet(const char *buf, size_t size)
{
	data_packet pkt(outgoing_, mac_);

	pkt.set_data(buf, size);
	pkt.sign();
	pkt.encrypt(cipher_);

	since_last_rekey_ += size;

	if (since_last_rekey_ > rekey_after_)
		do_rekey();
}

void session::write(const char *buf, size_t size)
{
	if (state_ != state::NORMAL || !size)
		return;

	const size_t wrp_size = data_packet::header_size + mac_->size();
	const size_t pkt_count = (size + 65535) / 65536;

	outgoing_.reserve(outgoing_.size() + size + pkt_count*wrp_size);

	const char *pkt, *end = buf + size;

	for (pkt = buf; pkt + 65536 < end; pkt += 65536, size -= 65536)
		write_packet(pkt, 65536);

	if (size)
		write_packet(pkt, size);
}

ssize_t session::read(char *buf, size_t size)
{
	if (state_ < state::NORMAL || !incoming_.size())
		return -1;

	if (incoming_.size() < size)
		size = incoming_.size();

	::memcpy(buf, &incoming_[0], size);
	incoming_.erase(0, size);
	return size;
}

ssize_t session::read_outgoing(char *buf, size_t size)
{
	ssize_t res = std::min(size, outgoing_.size());
	::memcpy(buf, outgoing_.c_str(), res);
	outgoing_.erase(0, res);

	if (outgoing_.size() == 0) {
		if (state_ == state::HANDSHAKING_TILL_SENT)
			state_ = state::NORMAL;
		else if (state_ == state::CLOSING)
			state_ = state::CLOSED;
		else
			;
	}

	return res;
}

void session::send_handshake_init(const std::string& encrypted_secret)
{
	std::stringstream stream;

	if (mode_ == mode::SERVER)
		stream	<< "Post-quantum hello v1.\n";
	else
		stream	<< "Post-quantum hello v1, " << server_name_ << ".\n";

	stream	<< "Key-exchange: " << kex::to_string(use_kex_) << '\n'
		<< "Supported-ciphers:";

	for (auto c : enabled_ciphers_)
		stream << " " << cipher::to_string(c);

	stream << "\nSupported-MACs:";

	for (auto m : enabled_macs_)
		stream << " " << mac::to_string(m);

	stream << '\n';

	if (server_auth_.size())
		stream << "Server-auth: " << server_auth_ << '\n';

	/* Client-auth !!! */
	stream << "Encrypted-secret: " << encrypted_secret << "\n";

	stream << "\n";

	outgoing_ = stream.str();
}

void session::send_handshake_fini(const std::string& nonce)
{
	std::stringstream stream;
	stream	<< "KEX: OK\n"
		<< "Cipher: " << cipher::to_string(*cipher_) << '\n'
		<< "MAC: " << mac::to_string(*mac_) << '\n'
		<< "Nonce: " << base64_encode(nonce) << "\n\n";

	outgoing_.append(stream.str());
}

void session::start_server()
{
	if (mode_ != mode::NONE)
		return;

	mode_ = mode::SERVER;
}

void session::start_client(const char *server_name)
{
	if (mode_ != mode::NONE)
		return;

	mode_ = mode::CLIENT;
	kex_ = kex::create(use_kex_, kex::mode::CLIENT);

	server_name_ = server_name;
	send_handshake_init(kex_->init());
}

void session::close()
{
	if (state_ != state::NORMAL)
		return;

	close_packet pkt(outgoing_, mac_);

	pkt.set_data();
	pkt.sign();
	pkt.encrypt(cipher_);

	state_ = state::CLOSING;
}

}
