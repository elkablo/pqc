#ifndef PQC_PACKET_HPP
#define PQC_PACKET_HPP

#include <cstddef>
#include <string>
#include <pqc_cipher.hpp>
#include <pqc_mac.hpp>

namespace pqc {

class packet
{
public:
	enum type {
		CLOSE = 0x00,
		DATA = 0x01,
		REKEY = 0x02
	};

	packet(std::string&, size_t, std::shared_ptr<mac>&);
	packet(std::string&, std::shared_ptr<mac>&);
	virtual ~packet();

	void sign();
	bool verify() const;
	void encrypt(const std::shared_ptr<cipher>&);
	bool is_encrypted() const;

	size_t get_size() const;

	virtual type get_type() const = 0;
	virtual size_t get_data_size() const = 0;
	virtual const char *get_data() const = 0;

protected:
	uint8_t *ptr();
	uint8_t *macptr();
	const uint8_t *ptr() const;
	const uint8_t *macptr() const;
	virtual size_t size_for_mac() const = 0;

	std::string &buffer_;
	size_t position_;
	std::shared_ptr<mac> mac_;
	bool encrypted_;
};

class close_packet : public packet
{
public:
	static const size_t header_size = 1;

	close_packet(std::string&, size_t, std::shared_ptr<mac>&);
	close_packet(std::string&, std::shared_ptr<mac>&);

	void set_data();

	type get_type() const;
	size_t get_data_size() const;
	const char *get_data() const;

protected:
	size_t size_for_mac() const;
};

class data_packet : public packet
{
public:
	static const size_t header_size = 5;

	data_packet(std::string&, size_t, std::shared_ptr<mac>&);
	data_packet(std::string&, std::shared_ptr<mac>&);

	void set_data(const char *, uint32_t);

	type get_type() const;
	size_t get_data_size() const;
	const char *get_data() const;

protected:
	size_t size_for_mac() const;
};

class rekey_packet : public packet
{
public:
	static const size_t header_size = 2;

	rekey_packet(std::string&, size_t, std::shared_ptr<mac>&);
	rekey_packet(std::string&, std::shared_ptr<mac>&);

	void set_data(const char *, uint8_t);

	type get_type() const;
	size_t get_data_size() const;
	const char *get_data() const;

protected:
	size_t size_for_mac() const;
};

}

#endif /* PQC_PACKET_HPP */
