#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <pqc_session.hpp>

namespace pqc {

class socket_session : public session
{
public:
	socket_session() = delete;
	socket_session(int sock) : sock_(sock), errno_(0), closed_(false), peer_closed_(false) {}
	~socket_session()
	{
		if (sock_ >= 0)
			::close(sock_);
	}

	void handshake()
	{
		while (!is_handshaken() && !is_error()) {
			transmit();
			block_for_input();
			receive();
		}
	}

	void start_client(const char *servername)
	{
		session::start_client(servername);
		transmit();
	}

	void write(const void *buf, size_t size)
	{
		session::write(buf, size);
		transmit();
	}

	ssize_t read(void *buf, size_t size)
	{
		if (bytes_available() >= size)
			return session::read(buf, size);

		if (bytes_available() == 0 || socket_bytes_available() > 0)
			receive();

		return session::read(buf, size);
	}

	void close()
	{
		session::close();
		transmit();
		::shutdown(sock_, SHUT_WR);
		closed_ = true;
	}

	int get_errno() const
	{
		return errno_;
	}

	void receive(bool block = true)
	{
		if (peer_closed_)
			return;

		bool is_input = block ? block_for_input() : check_for_input();

		if (is_error())
			return;

		size_t size = socket_bytes_available();

		if (!size) {
			if (is_input) {
				if (!is_peer_closed())
					set_error(error::OTHER);
				peer_closed_ = true;
			}
			return;
		}

		char buffer[1024];
		ssize_t rd;

		do {
			rd = ::read(sock_, buffer, 1024);
			if (rd > 0) {
				write_incoming(buffer, rd);
				size -= rd;
			} else if (rd == 0) {
				if (!is_peer_closed())
					set_error(error::OTHER);
				peer_closed_ = true;
				break;
			} else if (errno != EAGAIN && errno != EWOULDBLOCK) {
				set_errno();
				break;
			}
		} while (size > 0);
	}

	void transmit()
	{
		if (closed_ || !outgoing_.size() || peer_closed_)
			return;

		ssize_t written = ::write(sock_, outgoing_.c_str(), outgoing_.size());

		if (written < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return;
			else
				set_errno();
		} else if (written < outgoing_.size()) {
			outgoing_.erase(0, written);
		} else {
			outgoing_.erase();
		}
	}

private:
	bool check_for_input()
	{
		struct pollfd pfd;
		pfd.fd = sock_;
		pfd.events = POLLIN;
		pfd.revents = 0;

		int res = poll(&pfd, 1, 0);
		if (res < 0)
			set_errno();

		return pfd.revents & POLLIN;
	}

	bool block_for_input()
	{
		int res;
		struct pollfd pfd;
		pfd.fd = sock_;
		pfd.events = POLLIN;
		pfd.revents = 0;

		do {
			res = poll(&pfd, 1, -1);
		} while (res < 0 && errno == EINTR);

		if (res < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			set_errno();

		return pfd.revents & POLLIN;
	}

	size_t socket_bytes_available()
	{
		int result;

		if (::ioctl(sock_, FIONREAD, &result) < 0)
			return 0;

		return result;
	}

	void set_errno()
	{
		errno_ = errno;
		set_error(error::OTHER);
	}

	int errno_;
	int sock_;
	bool closed_, peer_closed_;
};

}
