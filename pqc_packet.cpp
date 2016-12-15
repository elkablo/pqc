#include <cstring>
#include <arpa/inet.h>
#include <pqc_packet.hpp>

namespace pqc {

packet::packet(std::string& buffer, size_t position, std::shared_ptr<mac>& mac) :
	buffer_(buffer), position_(position), mac_(mac), encrypted_(false)
{}

packet::packet(std::string& buffer, std::shared_ptr<mac>& mac) :
	buffer_(buffer), position_(buffer.size()), mac_(mac), encrypted_(false)
{}

packet::~packet()
{}

void packet::sign()
{
	mac_->compute(macptr(), ptr(), size_for_mac());
}

bool packet::verify() const
{
	uint8_t mac[mac_->size()];
	mac_->compute(mac, ptr(), size_for_mac());
	return !memcmp(mac, macptr(), mac_->size());
}

void packet::encrypt(const std::shared_ptr<cipher>& cipher)
{
	if (!encrypted_) {
		cipher->encrypt(reinterpret_cast<void *>(ptr()), get_size());
		encrypted_ = true;
	}
}

bool packet::is_encrypted() const
{
	return encrypted_;
}

size_t packet::get_size() const
{
	return size_for_mac() + mac_->size();
}

uint8_t *packet::ptr()
{
	return reinterpret_cast<uint8_t *>(&buffer_[position_]);
}

uint8_t *packet::macptr()
{
	return ptr() + size_for_mac();
}

const uint8_t *packet::ptr() const
{
	return reinterpret_cast<uint8_t *>(&buffer_[position_]);
}

const uint8_t *packet::macptr() const
{
	return ptr() + size_for_mac();
}

close_packet::close_packet(std::string& buffer, size_t position, std::shared_ptr<mac>& mac) :
	packet(buffer, position, mac)
{}

close_packet::close_packet(std::string& buffer, std::shared_ptr<mac>& mac) :
	packet(buffer, mac)
{}

void close_packet::set_data()
{
	// resize
	buffer_.resize(position_ + header_size + mac_->size());

	// set type
	ptr()[0] = type::CLOSE;
}

packet::type close_packet::get_type() const
{
	return type::CLOSE;
}

size_t close_packet::get_data_size() const
{
	return 0;
}

const char *close_packet::get_data() const
{
	return nullptr;
}

size_t close_packet::size_for_mac() const
{
	return header_size;
}

data_packet::data_packet(std::string& buffer, size_t position, std::shared_ptr<mac>& mac) :
	packet(buffer, position, mac)
{}

data_packet::data_packet(std::string& buffer, std::shared_ptr<mac>& mac) :
	packet(buffer, mac)
{}

void data_packet::set_data(const char *data, uint32_t size)
{
	// resize
	buffer_.resize(position_ + header_size + size + mac_->size());

	// set type
	ptr()[0] = type::DATA;

	// set size
	*reinterpret_cast<uint32_t *>(ptr() + 1) = ::htonl(size);

	// set data
	memcpy(ptr() + header_size, data, size);
}

packet::type data_packet::get_type() const
{
	return type::DATA;
}

size_t data_packet::get_data_size() const
{
	return ::ntohl(*reinterpret_cast<const uint32_t *>(ptr() + 1));
}

const char *data_packet::get_data() const
{
	return reinterpret_cast<const char *>(ptr() + header_size);
}

size_t data_packet::size_for_mac() const
{
	return header_size + get_data_size();
}

rekey_packet::rekey_packet(std::string& buffer, size_t position, std::shared_ptr<mac>& mac) :
	packet(buffer, position, mac)
{}

rekey_packet::rekey_packet(std::string& buffer, std::shared_ptr<mac>& mac) :
	packet(buffer, mac)
{}

void rekey_packet::set_data(const char *data, uint8_t size)
{
	// resize
	buffer_.resize(position_ + header_size + size + mac_->size());

	// set type
	ptr()[0] = type::REKEY;

	// set size
	*reinterpret_cast<uint8_t *>(ptr() + 1) = size;

	// set data
	memcpy(ptr() + header_size, data, size);
}

packet::type rekey_packet::get_type() const
{
	return type::REKEY;
}

size_t rekey_packet::get_data_size() const
{
	return *reinterpret_cast<const uint8_t *>(ptr() + 1);
}

const char *rekey_packet::get_data() const
{
	return reinterpret_cast<const char *>(ptr() + header_size);
}

size_t rekey_packet::size_for_mac() const
{
	return header_size + get_data_size();
}

}
