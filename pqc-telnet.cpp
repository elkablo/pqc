#include <iostream>
#include <fstream>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pqc_auth.hpp>
#include <pqc_sha.hpp>
#include <pqc-telnet-common.hpp>

using namespace std;
using namespace pqc;

static void set_fd_nonblocking(int fd)
{
	int flags = ::fcntl(fd, F_GETFL);

	if (flags < 0) {
		cerr << "cannot get file descriptor flags: " << ::strerror(errno) << endl;
		std::exit(EXIT_FAILURE);
	}

	if (flags & O_NONBLOCK)
		return;

	if (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		cerr << "cannot set file descriptor flags: " << ::strerror(errno) << endl;
		std::exit(EXIT_FAILURE);
	}
}

static void handle_session_input(socket_session& sess)
{
	sess.receive(false);
	if (sess.bytes_available() > 0) {
		do {
			uint8_t buffer[1024];
			ssize_t rd = sess.read(buffer, 1024);

			if (rd < 0) {
				cerr << "cannot read from session" << endl;
				std::exit(EXIT_FAILURE);
			}

			if (rd > 0 && ::write(STDOUT_FILENO, buffer, rd) != rd) {
				cerr << "cannot write to standard output" << endl;
				std::exit(EXIT_FAILURE);
			}
		} while (sess.bytes_available() > 0);
	}

	if (sess.is_peer_closed())
		sess.close();
}

static struct termios oldattr;

static void reset_terminal()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
}

static void set_terminal()
{
	struct termios attr;

	tcgetattr(STDIN_FILENO, &attr);
	oldattr = attr;

	std::atexit(reset_terminal);

	attr.c_iflag = 0;
	attr.c_lflag &= ~(ICANON|ECHO|ISIG);
	tcsetattr(STDIN_FILENO, TCSANOW, &attr);
}

static void handle_stdin_input(socket_session& sess)
{
	uint8_t buffer[1027];
	ssize_t rd;

	buffer[0] = 1;

	do {
		rd = ::read(STDIN_FILENO, buffer + 3, 1024);
		if (rd < 0) {
			cerr << "cannot read from standard input" << endl;
			std::exit(EXIT_FAILURE);
		} else if (rd > 0) {
			*reinterpret_cast<uint16_t *>(&buffer[1]) = htons(rd);
			sess.write(buffer, 3 + rd);
		}
	} while (rd == 1024 && !sess.is_error());
}

static void send_sigwinch_packet(socket_session& sess)
{
	struct winsize ws;

	if (::ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0) {
		cerr << "cannot get window size" << endl;
		std::exit(EXIT_FAILURE);
	}

	uint8_t buffer[17];
	buffer[0] = 2;

	uint32_t *ptr = reinterpret_cast<uint32_t *>(&buffer[1]);
	ptr[0] = htonl((uint32_t) ws.ws_row);
	ptr[1] = htonl((uint32_t) ws.ws_col);
	ptr[2] = htonl((uint32_t) ws.ws_xpixel);
	ptr[3] = htonl((uint32_t) ws.ws_ypixel);

	sess.write(buffer, 17);
}

static void handle_sigwinch(socket_session& sess)
{
	send_sigwinch_packet(sess);
}

static void handle_term(socket_session& sess)
{
	sess.close();
}

static int signal_pipe[2];

static void signal_handler(int signum)
{
	int saved_errno = errno;
	if (::write(signal_pipe[1], &signum, sizeof(signum)) != sizeof(signum))
		std::exit(EXIT_FAILURE);
	errno = saved_errno;
}

static void handle_signal_input(socket_session& sess)
{
	int signum;

	if (::read(signal_pipe[0], &signum, sizeof(signum)) != sizeof(signum))
		return;

	if (signum == SIGINT || signum == SIGTERM)
		handle_term(sess);
	else if (signum == SIGWINCH)
		handle_sigwinch(sess);
}

string server_pub_key_id, server_pub_key;

static void do_session(int sock)
{
	if (::pipe(signal_pipe) < 0) {
		cerr << "cannot create pipe" << endl;
		std::exit(EXIT_FAILURE);
	}

	::signal(SIGINT, signal_handler);
	::signal(SIGWINCH, signal_handler);

	socket_session sess(sock);
	sess.set_server_auth(server_pub_key_id, server_pub_key);
	sess.start_client("pqctelnet.test");
	sess.handshake();

	if (sess.is_error()) {
		cerr << "handshake with server failed" << endl;
		std::exit(EXIT_FAILURE);
	}

	sess.write("pqct", 4);

	char magic[4];
	if (sess.read(magic, 4) != 4 || memcmp(magic, "pqct", 4)) {
		cerr << "wrong magic header from server" << endl;
		std::exit(EXIT_FAILURE);
	}

	const char *term = ::getenv("TERM");
	uint16_t len = ::strlen(term) < 65535 ? ::strlen(term) : 65535;
	uint16_t nlen = htons(len);
	sess.write(&nlen, sizeof(nlen));
	sess.write(term, len);

	set_terminal();
	send_sigwinch_packet(sess);

	struct pollfd pfds[3];

	pfds[0].fd = sock;
	pfds[0].events = POLLIN;
	pfds[1].fd = STDIN_FILENO;
	pfds[1].events = POLLIN;
	pfds[2].fd = signal_pipe[0];
	pfds[2].events = POLLIN;

	if (sess.bytes_available())
		handle_session_input(sess);

	while (!sess.is_closed() && !sess.is_error()) {
		int res = poll(pfds, 3, -1);

		if (res < 0) {
			if (errno == EINTR)
				continue;
			cerr << "cannot poll: " << ::strerror(errno) << endl;
			std::exit(EXIT_FAILURE);
		}

		if (pfds[0].revents)
			handle_session_input(sess);
		if (pfds[1].revents)
			handle_stdin_input(sess);
		if (pfds[2].revents)
			handle_signal_input(sess);
	}

	if (sess.is_error()) {
		cerr << "pqc error" << endl;
		std::exit(EXIT_FAILURE);
	}

	std::exit(EXIT_SUCCESS);
}

static void read_key(const char *path)
{
	ifstream pub_file(path, ios_base::in | ios_base::binary);

	if (!pub_file) {
		cerr << "cannot open " << path << endl << endl;
		std::exit(EXIT_FAILURE);
	}

	pub_file.seekg(0, pub_file.end);
	size_t size = pub_file.tellg();
	pub_file.seekg(0, pub_file.beg);

	if (size <= 32) {
		cerr << path << " does not contain a key" << endl << endl;
		std::exit(EXIT_FAILURE);
	}

	server_pub_key_id.resize(32);
	pub_file.read(&server_pub_key_id[0], 32);
	server_pub_key_id = bin2hex(server_pub_key_id);

	server_pub_key.resize(size - 32);
	pub_file.read(&server_pub_key[0], size - 32);

	shared_ptr<auth> auth_ = auth::create(PQC_AUTH_SIDHex_SHA512);
	if (!auth_->set_request_key(server_pub_key)) {
		cerr << path << " does not contain a valid SIDHex-SHA512 key" << endl << endl;
		std::exit(EXIT_FAILURE);
	}
}

int main (int argc, char **argv) {
	if (argc != 4) {
		cerr << "usage: pqc-telnet pub-key-file ip-addr tcp-port" << endl << endl;
		std::exit(EXIT_FAILURE);
	}

	read_key(argv[1]);

	int sock;
	struct sockaddr_in addr;

	if (::inet_pton(AF_INET, argv[2], &addr.sin_addr) != 1) {
		cerr << "wrong ip address " << argv[2] << endl;
		std::exit(EXIT_FAILURE);
	}

	char *end;
	unsigned long int port = std::strtoul(argv[3], &end, 10);

	if (*end != '\0' || port < 1 || port > 65535) {
		cerr << "wrong port number " << argv[3] << endl;
		std::exit(EXIT_FAILURE);
	}

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((uint16_t) port);

	sock = ::socket(AF_INET, SOCK_STREAM, 0);

	if (::connect(sock, (const struct sockaddr *) &addr, sizeof(addr)) < 0) {
		cerr << "cannot connect to " << argv[2] << ":" << port << ": " << ::strerror(errno) << endl;
		std::exit(EXIT_FAILURE);
	}

	do_session(sock);
	return 0;
}
