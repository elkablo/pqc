#include <sstream>

#include <pqc_session.hpp>
#include <pqc_handshake.hpp>
#include <pqc_cipher.hpp>
#include <pqc_kex.hpp>
#include <pqc_mac.hpp>
#include <pqc_random.hpp>
#include <pqc_base64.hpp>

namespace pqc
{

session::session() :
	error_(error::NONE),
	state_(state::INIT),
	mode_(mode::NONE),
	remote_closed_(false),
	rekey_after_(1024*1024*1024),
	since_last_rekey_(0),
	encrypted_need_size_(0),
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

void session::set_error(error err)
{
	error_ = err;
	state_ = state::ERROR;
}

void session::write_incoming(const char *buf, size_t size)
{
	encrypted_incoming_.append(buf, size);

	while (encrypted_incoming_.size()) {
	if (state_ == state::INIT) {
		bool has_nn = encrypted_incoming_.find("\n\n") != std::string::npos;
		if (!has_nn && encrypted_incoming_.size() > 4096)
			return set_error(error::HANDSHAKE);

		if (!has_nn)
			return;

		const char *ptr;
		handshake handshake;

		ptr = handshake.parse_init(encrypted_incoming_.c_str());
		if (!ptr)
			return set_error(error::HANDSHAKE);
		encrypted_incoming_.erase(0, ptr - encrypted_incoming_.c_str());

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
			return set_error(error::HANDSHAKE);

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
			return set_error(error::HANDSHAKE);

		std::string nonce(random_string(std::max((size_t) 32, cipher_->nonce_size())));
		mac_->key(nonce);

		ephemeral_key_ = mac_->compute(session_key_);

		cipher_->nonce(nonce);
		cipher_->key(ephemeral_key_);

		if (mode_ == mode::SERVER) {
			send_handshake_init(encrypted_secret);
		}
		send_handshake_fini(nonce);

		state_ = state::HANDSHAKING;
	} else if (state_ == state::HANDSHAKING) {
		bool has_nn = encrypted_incoming_.find("\n\n") != std::string::npos;
		if (!has_nn && encrypted_incoming_.size() > 4096)
			return set_error(error::HANDSHAKE);

		if (!has_nn)
			return;

		const char *ptr;
		handshake handshake;

		ptr = handshake.parse_fini(encrypted_incoming_.c_str());
		if (!ptr)
			return set_error(error::HANDSHAKE);

		encrypted_incoming_.erase(0, ptr - encrypted_incoming_.c_str());

		if (handshake.cipher == PQC_CIPHER_UNKNOWN
			|| !is_cipher_enabled(handshake.cipher)
			|| handshake.mac == PQC_MAC_UNKNOWN
			|| !is_mac_enabled(handshake.mac)
			|| !handshake.nonce
		)
			return set_error(error::HANDSHAKE);

		peer_mac_ = mac::create(handshake.mac);
		peer_cipher_ = cipher::create(handshake.cipher);

		std::string peer_nonce = base64_decode(handshake.nonce);

		if (peer_nonce.size() < std::max((size_t) 32, peer_cipher_->nonce_size()))
			return set_error(error::HANDSHAKE);

		peer_mac_->key(peer_nonce);

		peer_ephemeral_key_ = peer_mac_->compute(session_key_);

		peer_cipher_->nonce(peer_nonce);
		peer_cipher_->key(peer_ephemeral_key_);

		if (outgoing_.size() > 0)
			state_ = state::HANDSHAKING_TILL_SENT;
		else
			state_ = state::NORMAL;
	} else if (state_ == state::NORMAL) {
		return handle_incoming_data();
	} else {
		break;
	}
	}
}

void session::decrypt_raw(size_t size)
{
	size_t decrypted_size = decrypted_incoming_.size();

	if (!size || encrypted_incoming_.size() < size)
		size = encrypted_incoming_.size();

	decrypted_incoming_.resize(decrypted_size + size);
	peer_cipher_->decrypt(reinterpret_cast<void *>(&decrypted_incoming_[decrypted_size]), size, &encrypted_incoming_[0], size);
	encrypted_incoming_.erase(0, size);
}

bool session::decrypt_needed()
{
	if (encrypted_incoming_.size() < encrypted_need_size_) {
		encrypted_need_size_ -= encrypted_incoming_.size();
		decrypt_raw();
		if (encrypted_need_size_)
			return false;
	} else {
		decrypt_raw(encrypted_incoming_.size());
		encrypted_need_size_ = 0;
	}
	return true;
}

session::packettype session::decrypt_next_packet(std::string& data)
{
	const size_t min_size = 1 + peer_mac_->size();
	size_t need = 0, size = 0;

	if (decrypted_incoming_.size() < min_size)
		encrypted_need_size_ = min_size;

	if (!decrypt_needed())
		return packettype::MORE;

	const uint8_t *pkt = reinterpret_cast<const uint8_t *>(&decrypted_incoming_[0]);

	if (pkt[0] == packettype::DATA) {
		if (decrypted_incoming_.size() >= 5)
			size = (pkt[1] << 24) | (pkt[2] << 16) | (pkt[3] << 8) | pkt[4];
		need = 5 + size + peer_mac_->size();
	} else if (pkt[0] == packettype::REKEY) {
		if (decrypted_incoming_.size() >= 2)
			size = pkt[1];
		need = 2 + size + peer_mac_->size();
	} else if (pkt[0] == packettype::CLOSE) {
		need = min_size;
	} else {
		return packettype::ERROR;
	}

	encrypted_need_size_ = need - decrypted_incoming_.size();
	if (!decrypt_needed())
		return packettype::MORE;

	if (decrypted_incoming_.size() >= need) {
		std::string peer_hash = decrypted_incoming_.substr(need - peer_mac_->size(), peer_mac_->size());
		std::string hash = peer_mac_->compute(pkt, need - peer_mac_->size());

		if (peer_hash != hash)
			return packettype::ERROR;

		if (pkt[0] == packettype::DATA)
			data = decrypted_incoming_.substr(5, size);
		else if (pkt[0] == packettype::REKEY)
			data = decrypted_incoming_.substr(2, size);

		decrypted_incoming_.erase(0, encrypted_need_size_);
		return static_cast<packettype>(pkt[0]);
	} else {
		encrypted_need_size_ = need - decrypted_incoming_.size();
		return packettype::MORE;
	}
}

void session::handle_incoming_data()
{
	while (encrypted_incoming_.size()) {
		std::string data;

		switch (decrypt_next_packet(data)) {
		case packettype::CLOSE:
			std::cout << "\tClose\n";
			break;
		case packettype::DATA:
			std::cout << "\tData " << data << "\n";
			incoming_ += data;
			break;
		case packettype::REKEY:
			std::cout << "\tRekey\n";
			break;
		case packettype::MORE:
			return;
		default:
			std::cout << "\tERROR msg type\n";
			return set_error(error::OTHER);
		}
	}
}

void session::write_packet(const char *buf, size_t size)
{
	const size_t start = outgoing_.size();
	const size_t hdr_size = 5;

	uint8_t hdr[hdr_size] = {
		0x01, 0x00, 0x00,
		(uint8_t) ((size >> 8) & 0xff),
		(uint8_t) (size & 0xff)
	};

	outgoing_.append(reinterpret_cast<char *>(hdr), 5);
	outgoing_.append(buf, size);
	outgoing_.append(mac_->compute(&outgoing_[start], hdr_size + size));

	cipher_->encrypt(
		reinterpret_cast<void *>(&outgoing_[start]),
		hdr_size + size + mac_->size()
	);
}

void session::write(const char *buf, size_t size)
{
	if (state_ != state::NORMAL || !size)
		return;

	const size_t hdr_size = 5;
	const size_t wrp_size = hdr_size + mac_->size();
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
	if (state_ != state::NORMAL && state_ != state::CLOSING)
		return -1;

	if (!incoming_.size())
		return -2;

	if (incoming_.size() < size)
		size = incoming_.size();

	::memcpy(buf, &incoming_[0], size);
	incoming_.erase(0, size);
	return size;
}

ssize_t session::read_outgoing(char *buf, size_t size)
{
	ssize_t res = std::min(size, outgoing_.size());
	memcpy(buf, outgoing_.c_str(), res);
	outgoing_.erase(0, res);

	if (state_ == state::HANDSHAKING_TILL_SENT && outgoing_.size() == 0)
		state_ = state::NORMAL;

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
	mode_ = mode::SERVER;
}

void session::start_client(const char *server_name)
{
	mode_ = mode::CLIENT;
	kex_ = kex::create(use_kex_, kex::mode::CLIENT);

	server_name_ = server_name;
	send_handshake_init(kex_->init());
}

void session::close()
{
}

}