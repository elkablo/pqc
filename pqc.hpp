#include <cstdint>
#include <functional>
#include <string>

enum pqc_cipher {
	PQC_CIPHER_UNKNOWN = 0,
	PQC_CIPHER_CHACHA20,
	PQC_CIPHER_PLAIN,
	PQC_CIPHER_LAST
};

enum pqc_auth {
	PQC_AUTH_UNKNOWN = 0,
	PQC_AUTH_SIDHex,
	PQC_AUTH_LAST
};

enum pqc_keyx {
	PQC_KEYX_UNKNOWN = 0,
	PQC_KEYX_SIDHex,
	PQC_KEYX_LAST
};

enum pqc_mac {
	PQC_MAC_UNKNOWN = 0,
	PQC_MAC_HMAC_SHA256,
	PQC_MAC_HMAC_SHA512,
	PQC_MAC_LAST
};

namespace pqc
{

class cipher
{
public:
	virtual ~cipher() {}

	virtual std::size_t key_size() const = 0;
	virtual std::size_t nonce_size() const = 0;

	virtual void key(const void *) = 0;
	virtual void nonce(const void *) = 0;

	virtual std::size_t encrypt(void *, std::size_t, const void *, std::size_t) = 0;
	virtual std::size_t decrypt(void *, std::size_t, const void *, std::size_t) = 0;

	virtual std::string encrypt(const std::string&);
	virtual std::string decrypt(const std::string&);

	static cipher * create_cipher(enum pqc_cipher);
};

class keyx
{
	
};

class mac
{
public:
	virtual ~mac() {}
	virtual std::size_t size() const = 0;
	virtual void compute(void *, const void *, std::size_t) = 0;
	virtual void compute(void *, const std::string&);

	virtual void key(const void *, std::size_t len) = 0;

	static mac * create_mac(enum pqc_mac);
};

class session
{
public:
	typedef std::function<std::string(const std::string&)> auth_callback_t;

	session();
	virtual ~session();

	void cipher_enable(enum pqc_cipher, bool);
	bool cipher_is_enabled(enum pqc_cipher) const;
	void keyx_enable(enum pqc_keyx, bool);
	bool keyx_is_enabled(enum pqc_cipher) const;
	void auth_enable(enum pqc_auth, bool);
	bool auth_is_enabled(enum pqc_cipher) const;
	void mac_enable(enum pqc_mac, bool);
	bool mac_is_enabled(enum pqc_cipher) const;

	void set_auth(const std::string&);
	void set_auth_callback(const auth_callback_t&);

	void set_rekey_after(std::size_t);
	std::size_t get_rekey_after() const;

	int received(void *, std::size_t, const void *, std::size_t);
	int tosend(void *, std::size_t, const void *, std::size_t);
	int shutdown(void *, std::size_t);

protected:
	int errno_;
	std::size_t rekey_after_;
	keyx *keyx_;
	auth *auth_;
	cipher *cipher_, *peer_cipher_;
	mac *mac_, *peer_mac_;
	auth_callback_t auth_callback_;
	std::bitset<PQC_CIPHER_LAST> enabled_ciphers_;
	std::bitset<PQC_AUTH_LAST> enabled_auths_;
	std::bitset<PQC_MAC_LAST> enabled_macs_;
	std::bitset<PQC_KEYX_LAST> enabled_keyxs_;
};

class server : public session
{
public:
	const std::string& get_server_name() const;

	int handshake_init(void *, std::size_t, const void *, std::size_t);
	int handshake_fini(const void *, std::size_t);

private:
	std::string server_name_;
};

class client : public session
{
public:
	void set_server_name(const std::string&);
	void set_server_auth(const std::string&);

	int handshake_init(void *, std::size_t);
	int handshake_fini(void *, std::size_t, const void *, std::size_t);

private:
	std::string server_name_;
	std::string server_auth_;
};

}
