#ifndef PQC_SESSION_HPP
#define PQC_SESSION_HPP

#include <functional>
#include <string>
#include <memory>
#include <pqc_enumset.hpp>
#include <pqc_packet_reader.hpp>

namespace pqc
{

class kex;
class cipher;
class mac;
class auth;

class session
{
	static const size_t min_nonce_size = 32;

	enum class state {
		ERROR,
		INIT,
		HANDSHAKING,
		NORMAL,
		CLOSED
	};
	enum class mode {
		NONE,
		SERVER,
		CLIENT
	};
public:
	enum class error {
		NONE = 0,
		WRONG_AUTH,
		BAD_HANDSHAKE,
		BAD_PACKET,
		BAD_MAC,
		BAD_REKEY,
		ALREADY_CLOSED,
		OTHER
	};

	typedef std::function<std::string(const std::string&)> auth_callback_t;

	session();
	virtual ~session();

	void cipher_enable(enum pqc_cipher, bool = true);
	bool is_cipher_enabled(enum pqc_cipher) const;
	void kex_enable(enum pqc_kex, bool = true);
	bool is_kex_enabled(enum pqc_kex) const;
	void auth_enable(enum pqc_auth, bool = true);
	bool is_auth_enabled(enum pqc_auth) const;
	void mac_enable(enum pqc_mac, bool = true);
	bool is_mac_enabled(enum pqc_mac) const;

	void set_kex(enum pqc_kex);
	enum pqc_kex get_kex() const;

	const std::string& get_server_name() const;
	void set_server_auth(const std::string&, const std::string&);
	void set_auth(const std::string&);
	void set_auth_callback(const auth_callback_t&);

	void set_rekey_after(size_t);
	size_t get_rekey_after() const;

	size_t since_last_rekey() const;
	size_t since_last_peer_rekey() const;

	bool is_error() const;
	error error_code() const;
	size_t bytes_available() const;
	size_t bytes_outgoing_available() const;
	bool is_handshaken() const;
	bool is_closed() const;
	bool is_peer_closed() const;
	void write_incoming(const void *, size_t);
	void write(const void *, size_t);
	ssize_t read(void *, size_t);
	ssize_t read_outgoing(void *, size_t);

	void start_server();
	void start_client(const char *);

	void close();

protected:
	void set_error(error);

private:
	void do_rekey();
	void write_packet(const char *, size_t);
	void send_handshake_init(const std::string&);
	void send_handshake_fini(const std::string&, const std::string& = "");
	void handle_handshake(const char *, size_t);
	void handle_incoming_close();
	void handle_incoming_data(const char *, size_t);
	void handle_incoming_rekey(const char *, size_t);
	void handle_incoming(const char *, size_t);

	error error_;
	state state_;
	mode mode_;
	bool peer_closed_;
	size_t rekey_after_, since_last_rekey_, since_last_peer_rekey_;

protected:
	std::string incoming_, outgoing_;

private:
	std::string incoming_handshake_;

	packet_reader packet_reader_;

	std::string session_key_, ephemeral_key_, peer_ephemeral_key_;

	std::string server_name_;
	std::string server_auth_id_, server_auth_;
	std::shared_ptr<kex> kex_;
	std::shared_ptr<auth> auth_;
	std::shared_ptr<cipher> cipher_, peer_cipher_;
	std::shared_ptr<mac> mac_, peer_mac_;
	auth_callback_t auth_callback_;
	cipherset enabled_ciphers_;
	authset enabled_auths_;
	macset enabled_macs_;
	kexset enabled_kexes_;
	enum pqc_kex use_kex_;
	enum pqc_auth use_auth_;
};

}

#endif /* PQC_SESSION_HPP */
