#ifndef PQC_HPP
#define PQC_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <memory>

#include "gf.hpp"

enum pqc_cipher {
	PQC_CIPHER_UNKNOWN = 0,
	PQC_CIPHER_CHACHA20,
	PQC_CIPHER_PLAIN,

	PQC_CIPHER_FIRST = PQC_CIPHER_CHACHA20,
	PQC_CIPHER_LAST = PQC_CIPHER_PLAIN
};

#define for_each_pqc_cipher(var) \
	for (enum pqc_cipher var = PQC_CIPHER_FIRST; var <= PQC_CIPHER_LAST; ++*reinterpret_cast<int*>(&var))

enum pqc_auth {
	PQC_AUTH_UNKNOWN = 0,
	PQC_AUTH_SIDHex,

	PQC_AUTH_FIRST = PQC_AUTH_SIDHex,
	PQC_AUTH_LAST = PQC_AUTH_SIDHex
};

#define for_each_pqc_auth(var) \
	for (enum pqc_auth var = PQC_AUTH_FIRST; var <= PQC_AUTH_LAST; ++*reinterpret_cast<int*>(&var))

enum pqc_kex {
	PQC_KEX_UNKNOWN = 0,
	PQC_KEX_SIDHex,

	PQC_KEX_FIRST = PQC_KEX_SIDHex,
	PQC_KEX_LAST = PQC_KEX_SIDHex
};

#define for_each_pqc_kex(var) \
	for (enum pqc_kex var = PQC_KEX_FIRST; var <= PQC_KEX_LAST; ++*reinterpret_cast<int*>(&var))

enum pqc_mac {
	PQC_MAC_UNKNOWN = 0,
	PQC_MAC_HMAC_SHA256,
	PQC_MAC_HMAC_SHA512,

	PQC_MAC_FIRST = PQC_MAC_HMAC_SHA256,
	PQC_MAC_LAST = PQC_MAC_HMAC_SHA512
};

#define for_each_pqc_mac(var) \
	for (enum pqc_mac var = PQC_MAC_FIRST; var <= PQC_MAC_LAST; ++*reinterpret_cast<int*>(&var))

namespace pqc
{

typedef uint32_t ciphers_bitset;
typedef uint32_t macs_bitset;
typedef uint32_t kexes_bitset;
typedef uint32_t auths_bitset;

class cipher;
class kex;
class mac;

class session
{
	enum class state {
		ERROR,
		INIT,
		HANDSHAKING,
		HANDSHAKING_TILL_SENT,
		NORMAL,
		CLOSING,
		CLOSED
	};
	enum class mode {
		NONE,
		SERVER,
		CLIENT
	};
	enum packettype {
		CLOSE = 0,
		DATA = 1,
		REKEY = 2,
		MORE = 126,
		ERROR = 127
	};
public:
	enum class error {
		NONE = 0,
		INIT,
		HANDSHAKE,
		OTHER
	};

	typedef std::function<const char *(const char *)> auth_callback_t;

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
	void set_server_auth(const char *);
	void set_auth(const char *);
	void set_auth_callback(const auth_callback_t&);

	void set_rekey_after(size_t);
	size_t get_rekey_after() const;

	bool is_error() const;
	error error_code() const;
	size_t bytes_available() const;
	size_t bytes_outgoing_available() const;
	bool is_handshaken() const;
	bool is_closed() const;
	bool is_remote_closed() const;
	void write_incoming(const char *, size_t);
	void write(const char *, size_t);
	ssize_t read(char *, size_t);
	ssize_t read_outgoing(char *, size_t);

	void start_server();
	void start_client(const char *);

	void close();

private:
	void send_handshake_init(const std::string&);
	void send_handshake_fini(const std::string&);
	void set_error(error);
	void decrypt_raw(size_t size = 0);
	bool decrypt_needed();
	packettype decrypt_next_packet(std::string&);
	void handle_incoming_data();

	error error_;
	state state_;
	mode mode_;
	bool remote_closed_;
	size_t rekey_after_, since_last_rekey_;
	std::string incoming_, outgoing_, encrypted_incoming_;

	std::string decrypted_incoming_;
	size_t encrypted_need_size_;

	std::string session_key_, ephemeral_key_, peer_ephemeral_key_;
	//std::string nonce_, peer_nonce_;

	std::string server_name_;
	std::string server_auth_, auth_;
	std::shared_ptr<kex> kex_;
	//auth *auth_;
	std::shared_ptr<cipher> cipher_, peer_cipher_;
	std::shared_ptr<mac> mac_, peer_mac_;
	auth_callback_t auth_callback_;
	ciphers_bitset enabled_ciphers_;
	auths_bitset enabled_auths_;
	macs_bitset enabled_macs_;
	kexes_bitset enabled_kexes_;
	enum pqc_kex use_kex_;
};

}

#endif /* PQC_HPP */
