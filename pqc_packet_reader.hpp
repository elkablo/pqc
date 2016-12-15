#ifndef PQC_PACKET_READER_HPP
#define PQC_PACKET_READER_HPP

#include <cstddef>
#include <string>
#include <memory>
#include <pqc_packet.hpp>
#include <pqc_cipher.hpp>
#include <pqc_mac.hpp>

namespace pqc {

class packet_reader
{
public:
	packet_reader();
	packet_reader(const packet_reader&) = delete;

	void set_cipher(const std::shared_ptr<cipher>&);
	void set_mac(const std::shared_ptr<mac>&);

	void write_incoming(const char *, size_t);
	bool is_error() const;
	const packet *get_packet();
	void pop_packet();

private:
	void decrypt_raw(size_t);
	bool decrypt_needed();
	bool decrypt_packet();

	std::string incoming_;
	size_t decrypted_, need_;
	std::shared_ptr<cipher> cipher_;
	std::shared_ptr<mac> mac_;
	std::shared_ptr<packet> packet_;
	bool packet_complete_;
	bool error_;
};

}

#endif /* PQC_PACKET_READER_HPP */
