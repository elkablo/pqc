#include <sstream>
#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pqc_session.hpp>

namespace pqc {

class socket_session : public session
{
public:
	socket_session() = delete;
	socket_session(int sock) : sock_(sock), errno_(0), closed_(false) {}
	~socket_session()
	{
		if (sock_ >= 0)
			::close(sock_);
	}

	void blocking_handshake()
	{
		struct pollfd pfd;
		pfd.fd = sock_;
		pfd.events = POLLIN;

		while (!is_handshaken() && !is_error()) {
			if (poll(&pfd, 1, -1) < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
				set_errno();
				return;
			}

			receive();
			transmit();
		}
	}

	void start_client(const char *servername)
	{
		session::start_client(servername);
		transmit();
	}

	void write(const char *buf, size_t size)
	{
		session::write(buf, size);
		transmit();
	}

	ssize_t read(char *buf, size_t size)
	{
		receive();
		transmit();
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

private:
	size_t socket_bytes_available()
	{
		int result = 0;
		if (::ioctl(sock_, FIONREAD, &result) < 0)
			set_errno();
		return result;
	}

	void receive()
	{
		size_t size = socket_bytes_available();

		if (!size)
			return;

		char *buffer = new char[size];
		ssize_t rd = ::read(sock_, buffer, size);

		if (rd > 0)
			write_incoming(buffer, rd);

		delete[] buffer;
		
		if (rd < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			set_errno();
	}

	void set_errno()
	{
		errno_ = errno;
		set_error(error::OTHER);
	}

	ssize_t transmit()
	{
		if (closed_ || !outgoing_.size())
			return 0;

		ssize_t written = ::write(sock_, outgoing_.c_str(), outgoing_.size());

		if (written < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return 0;
			else
				set_errno();
		} else if (written < outgoing_.size()) {
			outgoing_.erase(0, written);
		} else {
			outgoing_.erase();
		}

		return written;
	}

	int errno_;
	int sock_;
	bool closed_;
};

}

using namespace std;
using namespace pqc;

void set_fd_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags < 0) {
		cerr << "cannot get file descriptor flags: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	if (flags & O_NONBLOCK)
		return;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		cerr << "cannot set file descriptor flags: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}

void handle_client(int sock)
{
	struct pollfd pfds[2];

	set_fd_nonblocking(sock);

	pqc::socket_session sess(sock);
	sess.start_server();
	sess.blocking_handshake();

	if (sess.is_error()) {
		cerr << "handshake with client failed" << endl;
		exit(EXIT_FAILURE);
	}

	
}

void set_reuse_addr(int sock)
{
	int val = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		cerr << "cannot reuse address: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}

int main (int argc, char **argv) {
	if (argc != 2) {
		cerr << "usage: pqc-telnetd TCP_PORT" << endl << endl;
		exit(EXIT_FAILURE);
	}

	int sock;
	struct sockaddr_in addr;
	char *end;
	unsigned long int port = strtoul(argv[1], &end, 10);

	if (*end != '\0' || port < 1 || port > 65535) {
		cerr << "wrong port number " << argv[1] << endl;
		exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((uint16_t) port);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	set_reuse_addr(sock);

	if (bind(sock, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
		cerr << "cannot bind to port " << port << ": " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	if (listen(sock, 4) < 0) {
		cerr << "cannot listen: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	while (true) {
		struct sockaddr_in addr;
		socklen_t addrlen = sizeof(addr);

		int client = accept(sock, (struct sockaddr *) &addr, &addrlen);

		if (client < 0) {
			cerr << "cannot accept client: " << strerror(errno) << "\n";
			continue;
		}

		char client_ip[32];
		inet_ntop(AF_INET, &addr.sin_addr, client_ip, 32);

		pid_t pid = fork();
		if (pid == 0) {
			handle_client(client);
			exit(EXIT_SUCCESS);
		} else if (pid > 0) {
			cout << "accepted client " << client_ip << ":" << ntohs(addr.sin_port) << endl;
		} else {
			cerr << "cannot fork for client " << client_ip << ":" << ntohs(addr.sin_port) << ": " << strerror(errno) << endl;
		}

		close(client);
	}

	return 0;
}
