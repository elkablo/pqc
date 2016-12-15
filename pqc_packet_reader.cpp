#include <pqc_packet_reader.hpp>

namespace pqc {

packet_reader::packet_reader() :
	decrypted_(0), packet_complete_(false), error_(false)
{}

void packet_reader::set_cipher(const std::shared_ptr<cipher>& cipher)
{
	cipher_ = cipher;
}

void packet_reader::set_mac(const std::shared_ptr<mac>& mac)
{
	mac_ = mac;
}

void packet_reader::write_incoming(const char *data, size_t size)
{
	incoming_.append(data, size);
}

void packet_reader::decrypt_raw(size_t size)
{
	// assert(decrypted_ + size <= incoming_.size());

	if (size > 0) {
		cipher_->decrypt(reinterpret_cast<void *>(&incoming_[decrypted_]), size);
		decrypted_ += size;
	}
}

bool packet_reader::decrypt_needed()
{
	if (incoming_.size() >= need_) {
		decrypt_raw(need_ - decrypted_);
		need_ = 0;
		return true;
	} else {
		return false;
	}
}

bool packet_reader::decrypt_packet()
{
	if (error_ || (packet_ && packet_complete_))
		return false;

	if (decrypted_ < 1 + mac_->size())
		need_ = 1 + mac_->size();
	else if (packet_)
		need_ = packet_->get_size();
	else
		/* need_ set by previous call to this function */
		;

	while (true) {
		if (!decrypt_needed())
			return false;

		if (packet_) {
			packet_complete_ = true;
			return true;
		}

		need_ = mac_->size();

		switch (incoming_[0]) {
		case packet::type::CLOSE:
			need_ += close_packet::header_size;
			if (decrypted_ >= close_packet::header_size)
				packet_ = std::make_shared<close_packet>(incoming_, 0, mac_);
			break;
		case packet::type::DATA:
			need_ += data_packet::header_size;
			if (decrypted_ >= data_packet::header_size)
				packet_ = std::make_shared<data_packet>(incoming_, 0, mac_);
			break;
		case packet::type::REKEY:
			need_ += rekey_packet::header_size;
			if (decrypted_ >= rekey_packet::header_size)
				packet_ = std::make_shared<rekey_packet>(incoming_, 0, mac_);
			break;
		default:
			error_ = true;
			return false;
		}

		if (packet_)
			need_ = packet_->get_size();
	}
}

bool packet_reader::is_error() const
{
	return error_;
}

const packet *packet_reader::get_packet()
{
	decrypt_packet();
	return packet_complete_ ? packet_.get() : nullptr;
}

void packet_reader::pop_packet()
{
	if (packet_complete_) {
		size_t pop_size = packet_->get_size();

		packet_.reset();
		packet_complete_ = false;

		incoming_.erase(0, pop_size);
		decrypted_ = 0;
	}
}

}
