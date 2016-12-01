#include <cstdint>
#include <functional>
#include <string>
#include <vector>

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

class cipher
{
public:
	virtual ~cipher() {}

	virtual size_t key_size() const = 0;
	virtual size_t nonce_size() const = 0;

	virtual void key(const void *) = 0;
	virtual void nonce(const void *) = 0;

	virtual size_t encrypt(void *, size_t, const void *, size_t) = 0;
	virtual size_t decrypt(void *, size_t, const void *, size_t) = 0;

	virtual std::string encrypt(const std::string&);
	virtual std::string decrypt(const std::string&);

	virtual operator pqc_cipher () const = 0;

	static cipher * create(enum pqc_cipher);
	static enum pqc_cipher from_string(const char *str, size_t size);
	static const char *to_string(enum pqc_cipher);

	static constexpr enum pqc_cipher get_default() { return PQC_CIPHER_CHACHA20; }
	static constexpr ciphers_bitset enabled_default()
	{
		return (1 << PQC_CIPHER_CHACHA20);
	}
};

class kex
{
public:
	enum class mode {
		SERVER,
		CLIENT
	};

	kex(mode);
	virtual ~kex() {}

	virtual std::string init(mode) = 0;
	virtual std::string fini(const std::string&) = 0;

	static kex * create(enum pqc_kex);
	static enum pqc_kex from_string (const char *, size_t);
	static const char *to_string(enum pqc_kex);

	static constexpr enum pqc_kex get_default() { return PQC_KEX_SIDHex; }
	static constexpr kexes_bitset enabled_default()
	{
		return (1 << PQC_KEX_SIDHex);
	}
protected:
	mode mode_;
};

class mac
{
public:
	virtual ~mac() {}
	virtual size_t size() const = 0;
	virtual void compute(void *, const void *, size_t) = 0;
	virtual void compute(void *, const std::string&);

	virtual void key(const void *, size_t len) = 0;

	virtual operator enum pqc_mac() const = 0;

	static mac * create(enum pqc_mac);
	static enum pqc_mac from_string (const char *, size_t);
	static const char *to_string(enum pqc_mac);

	static constexpr enum pqc_mac get_default() { return PQC_MAC_HMAC_SHA512; }
	static constexpr macs_bitset enabled_default()
	{
		return (1 << PQC_MAC_HMAC_SHA256) | (1 << PQC_MAC_HMAC_SHA512);
	}
};

struct handshake {
	int version;
	char *server_name;
	enum pqc_kex kex;
	ciphers_bitset supported_ciphers;
	macs_bitset supported_macs;
	char *server_auth;

	char **client_auths;
	size_t client_auths_len;

	char *encrypted_secret;

	enum pqc_cipher cipher;
	enum pqc_mac mac;
	char *nonce;

	handshake();
	~handshake();
	bool supports_cipher(enum pqc_cipher);
	bool supports_mac(enum pqc_mac);
	const char * parse_init(const char *);
	const char * parse_fini(const char *);
};

class session
{
	enum class state {
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
	void write_incoming(const char *, size_t);
	void write(const char *, size_t);
	ssize_t read(char *, size_t);
	ssize_t read_outgoing(char *, size_t);

	void start_server();
	void start_client(const char *);

	void close();

private:
	void send_handshake_init(const char *server_name = nullptr);
	void send_handshake_fini();

	error error_;
	state state_;
	mode mode_;
	size_t rekey_after_, since_last_rekey_;
	std::string incoming_, outgoing_, encrypted_incoming_;

	std::string key_, peer_key_;
	std::string nonce_, peer_nonce_;

	std::string server_auth_, auth_;
	kex *kex_;
	//auth *auth_;
	cipher *cipher_, *peer_cipher_;
	mac *mac_, *peer_mac_;
	auth_callback_t auth_callback_;
	ciphers_bitset enabled_ciphers_;
	auths_bitset enabled_auths_;
	macs_bitset enabled_macs_;
	kexes_bitset enabled_kexes_;
	enum pqc_kex use_kex_;
};

void random_bytes(char *, size_t);
std::string random_string(size_t);
Z random_z(size_t);
Z random_z_below(const Z&);
std::string base64_encode(const std::string&);
std::string base64_decode(const std::string&);

}
