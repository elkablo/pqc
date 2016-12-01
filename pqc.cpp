#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>

#include "pqc.hpp"

namespace pqc
{

session::session() :
	error_(error::NONE),
	state_(state::INIT),
	mode_(mode::NONE),
	rekey_after_(1024*1024*1024),
	since_last_rekey_(0),
	kex_(nullptr),
	cipher_(nullptr),
	peer_cipher_(nullptr),
	mac_(nullptr),
	peer_mac_(nullptr),
	enabled_ciphers_(cipher::enabled_default()),
	enabled_auths_(0),
	enabled_macs_(mac::enabled_default()),
	enabled_kexes_(kex::enabled_default()),
	use_kex_(kex::get_default())
{}

session::~session()
{
	if (kex_)
		delete kex_;
	if (cipher_)
		delete cipher_;
	if (peer_cipher_)
		delete peer_cipher_;
	if (mac_)
		delete mac_;
	if (peer_mac_)
		delete peer_mac_;
}

#define ENABLER(name,type,var,fst,last)			\
void session::name##_enable (type value, bool enable)	\
{							\
	if (value < fst || value > last)		\
		return;					\
	if (enable)					\
		var |= 1 << value;			\
	else						\
		var &= ~(uint32_t) (1 << value);	\
}							\
bool session::is_##name##_enabled (type value) const	\
{							\
	if (value < fst || value > last)		\
		return false;				\
	return var & (1 << value);			\
}
ENABLER(cipher, enum pqc_cipher, enabled_ciphers_, PQC_CIPHER_FIRST, PQC_CIPHER_LAST)
ENABLER(kex, enum pqc_kex, enabled_kexes_, PQC_KEX_FIRST, PQC_KEX_LAST)
ENABLER(auth, enum pqc_auth, enabled_auths_, PQC_AUTH_FIRST, PQC_AUTH_LAST)
ENABLER(mac, enum pqc_mac, enabled_macs_, PQC_MAC_FIRST, PQC_MAC_LAST)
#undef ENABLER

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

void session::write_incoming(const char *buf, size_t size)
{
	encrypted_incoming_.append(buf, size);

	if (state_ == state::INIT) {
		bool has_nn = encrypted_incoming_.find("\n\n") != std::string::npos;
		if (!has_nn && encrypted_incoming_.size() > 4096) {
			error_ = error::HANDSHAKE;
			return;
		}

		if (!has_nn)
			return;

		const char *ptr;
		handshake handshake;

		ptr = handshake.parse_init(encrypted_incoming_.c_str());
		if (!ptr) {
			error_ = error::HANDSHAKE;
			return;
		}

		encrypted_incoming_.erase(0, ptr - encrypted_incoming_.c_str());

		ciphers_bitset available_ciphers = handshake.supported_ciphers & enabled_ciphers_;
		macs_bitset available_macs = handshake.supported_macs & enabled_macs_;

		if (handshake.version != 1
			|| handshake.kex == PQC_KEX_UNKNOWN
			|| !is_kex_enabled(handshake.kex)
			|| !available_ciphers
			|| !available_macs
			|| !handshake.encrypted_secret
		) {
			error_ = error::HANDSHAKE;
			return;
		}

		use_kex_ = handshake.kex;

		// kex_ = ...

		for_each_pqc_cipher(c) {
			if ((1 << c) & available_ciphers) {
				cipher_ = cipher::create(c);
				break;
			}
		}

		for_each_pqc_mac(m) {
			if ((1 << m) & available_macs) {
				mac_ = mac::create(m);
				break;
			}
		}

		if (mode_ == mode::SERVER) {
			send_handshake_init();
		} else {
			send_handshake_fini();
		}

		state_ = state::HANDSHAKING;
	} else if (state_ == state::HANDSHAKING) {
		bool has_nn = encrypted_incoming_.find("\n\n") != std::string::npos;
		if (!has_nn && encrypted_incoming_.size() > 4096) {
			error_ = error::HANDSHAKE;
			return;
		}

		if (!has_nn)
			return;

		const char *ptr;
		handshake handshake;

		ptr = handshake.parse_fini(encrypted_incoming_.c_str());
		if (!ptr) {
			error_ = error::HANDSHAKE;
			return;
		}

		encrypted_incoming_.erase(0, ptr - encrypted_incoming_.c_str());

		if (handshake.cipher == PQC_CIPHER_UNKNOWN
			|| !is_cipher_enabled(handshake.cipher)
			|| handshake.mac == PQC_MAC_UNKNOWN
			|| !is_mac_enabled(handshake.mac)
			|| !handshake.nonce
		) {
			error_ = error::HANDSHAKE;
			return;
		}

		peer_cipher_ = cipher::create(handshake.cipher);
		peer_mac_ = mac::create(handshake.mac);
		peer_nonce_ = handshake.nonce;

		if (mode_ == mode::SERVER) {
			send_handshake_fini();
			state_ = state::HANDSHAKING_TILL_SENT;
		} else {
			state_ = state::NORMAL;
		}
	} else if (state_ == state::NORMAL) {
		
	}
}

void session::write(const char *buf, size_t size)
{
	if (state_ != state::NORMAL)
		return;


}

ssize_t session::read(char *buf, size_t size)
{
	if (state_ < state::NORMAL)
		return -1;


	return 0;
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

void session::start_server()
{
	mode_ = mode::SERVER;

	std::stringstream stream;
	stream << "Post-quantum hello v1.\n";
}

void session::send_handshake_init(const char *server_name)
{
	std::stringstream stream;

	if (mode_ == mode::SERVER)
		stream	<< "Post-quantum hello v1.\n";
	else
		stream	<< "Post-quantum hello v1, " << server_name << ".\n";

	stream	<< "Key-exchange: " << kex::to_string(use_kex_) << '\n'
		<< "Supported-ciphers:";

	for_each_pqc_cipher(c)
		if (is_cipher_enabled(c))
			stream << " " << cipher::to_string(c);

	stream << "\nSupported-MACs:";

	for_each_pqc_mac(m)
		if (is_mac_enabled(m))
			stream << " " << mac::to_string(m);

	stream << '\n';

	if (server_auth_.size())
		stream << "Server-auth: " << server_auth_ << '\n';

	/* Client-auth !!! */

	stream << "Encrypted-secret: brekeke\n";

	stream << "\n";

	outgoing_ = stream.str();
}

void session::send_handshake_fini()
{
	std::string nonce(random_string(cipher_->nonce_size()));

/*	cipher_->key
	cipher_->nonce(nonce.c_str());
	mac_->key(*/

	std::stringstream stream;
	stream	<< "KEX: OK\n"
		<< "Cipher: " << cipher::to_string(*cipher_) << '\n'
		<< "MAC: " << mac::to_string(*mac_) << '\n'
		<< "Nonce: " << base64_encode(nonce) << "\n\n";

	outgoing_.append(stream.str());
}

void session::start_client(const char *server_name)
{
	mode_ = mode::CLIENT;

	send_handshake_init(server_name);
}

void session::close()
{
}

}

#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <poll.h>

void waitfd(int fd)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	poll(&pfd, 1, 1000);
}

void move_between(pqc::session& sess, int fd)
{
	char buf[4096];
	size_t sz;
	while ((sz = sess.bytes_outgoing_available()) > 0) {
		if (sz > 4096)
			sz = 4096;
		sess.read_outgoing(buf, sz);
		if (write(fd, buf, sz) < (ssize_t) sz) {
			perror("write");
			exit(1);
		} else {
			printf("--Written %zi bytes to socket %i.\n", sz, fd);
		}
	}

	waitfd(fd);

	while (1) {
		int value;
		if (ioctl(fd, FIONREAD, &value) < 0) {
			perror("ioctl");
			exit(1);
		}

		if (value == 0)
			break;

		if (value > 4096)
			value = 4096;

		if (read(fd, buf, value) < value) {
			perror("read");
			exit(1);
		}

		printf("--Read %i bytes from socket %i.\n", value, fd);
		sess.write_incoming(buf, value);
	}
}

int do_server(int fd)
{
//	char buf[4096];
//	size_t sz;
	pqc::session sess;

	sess.start_server();
	while (!sess.is_handshaken()) {
		move_between(sess, fd);
		if (sess.is_error()) {
			std::cerr << "server: error\n";
			return 1;
		}
	}

	std::cout << "server done\n";

	return 0;
}

int do_client(int fd)
{
//	char buf[4096];
//	size_t sz;
	pqc::session sess;

	sess.start_client("google");
	while (!sess.is_handshaken()) {
		move_between(sess, fd);
		if (sess.is_error()) {
			std::cerr << "client: error\n";
			return 1;
		}
	}

	std::cout << "client done\n";

	return 0;
}

int main () {
	int sv[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
		perror("socketpair");
		return 1;
	}

	pid_t pid = fork();
	if (pid == 0) {
		close(sv[1]);
		return do_client(sv[0]);
	} else if (pid > 0) {
		close(sv[0]);
		int res = do_server(sv[1]);
		wait(NULL);
		return res;
	} else {
		perror("fork");
		return 1;
	}
}
