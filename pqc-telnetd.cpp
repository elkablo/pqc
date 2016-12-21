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
#include <sys/wait.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <pqc_socket_session.hpp>

using namespace std;
using namespace pqc;

static void set_fd_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL);

	if (flags < 0) {
		cerr << "cannot get file status flags: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	if (flags & O_NONBLOCK)
		return;

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		cerr << "cannot set file status flags: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}

static void set_fd_cloexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);

	if (flags < 0) {
		cerr << "cannot get file descriptor flags: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	if (flags & FD_CLOEXEC)
		return;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
		cerr << "cannot set file descriptor flags: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}

static string read_packet(pqc::socket_session& sess, size_t len)
{
	string result;
	result.resize(len);
	ssize_t rd = sess.read(&result[0], len);
	if (rd != len) {
		cerr << "cannot read packet" << endl;
		exit(EXIT_FAILURE);
	}

	return result;
}

static string read_varsized_packet(pqc::socket_session& sess)
{
	uint16_t length;

	if (sess.read(&length, sizeof(length)) != sizeof(length)) {
		cerr << "cannot read length of variable sized packet" << endl;
		exit(EXIT_FAILURE);
	}

	length = ntohs(length);
	return read_packet(sess, length);
}

static struct winsize read_winsize_packet(pqc::socket_session& sess)
{
	string pkt = read_packet(sess, 16);
	const uint32_t *ptr = reinterpret_cast<const uint32_t *>(&pkt[0]);
	struct winsize result;

	result.ws_row = ntohl(ptr[0]);
	result.ws_col = ntohl(ptr[1]);
	result.ws_xpixel = ntohl(ptr[2]);
	result.ws_ypixel = ntohl(ptr[3]);

	return result;
}

static uint8_t read_byte(pqc::socket_session& sess)
{
	uint8_t res;
	sess.read(&res, 1);
	return res;
}

static void handle_session_input(pqc::socket_session& sess, int pty)
{
	sess.receive(false);
	if (sess.bytes_available() == 0)
		return;

	do {
		uint8_t byte = read_byte(sess);

		if (byte == 1) {
			string pkt = read_varsized_packet(sess);

			if (::write(pty, pkt.c_str(), pkt.size()) != pkt.size()) {
				cerr << "cannot write to pty" << endl;
				exit(EXIT_FAILURE);
			}
		} else if (byte == 2) {
			struct winsize ws = read_winsize_packet(sess);
			::ioctl(pty, TIOCSWINSZ, &ws);
		} else {
			cerr << "wrong packet type from client" << endl;
			exit(EXIT_FAILURE);
		}
	} while (sess.bytes_available());

	if (sess.is_peer_closed())
		sess.close();
}

static bool handle_pty_input(pqc::socket_session& sess, int pty, bool all = false)
{
	char buffer[4096];
	ssize_t rd;

	do {
		rd = ::read(pty, buffer, 4096);

		if (rd < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
			return false;

		if (rd > 0)
			sess.write(buffer, rd);
	} while (all && rd == 4096);

	return true;
}

static pid_t forkpty(int *amaster, const struct winsize *winp)
{
	int master = ::open("/dev/ptmx", O_RDWR|O_NOCTTY);
	if (master < 0) {
		cerr << "cannot open /dev/ptmx: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}

	*amaster = master;

	pid_t pid = fork();
	if (pid < 0) {
		cerr << "cannot fork: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	} else if (pid == 0) {
		char name[256];

		if (::grantpt(master) < 0 || ::unlockpt(master) < 0 || ::ptsname_r(master, name, 256) < 0) {
			cerr << "grantpt/unlockpt/ptsname_r error" << endl;
			exit(EXIT_FAILURE);
		}

		int slave = ::open(name, O_RDWR);
		if (slave < 0) {
			cerr << "cannot open " << name << ": " << strerror(errno) << endl;
			exit(EXIT_FAILURE);
		}

		close(master);
		setsid();
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		if (slave != 0 && slave != 1 && slave != 2)
			close(slave);

		const char * const argv[3] = {"/bin/bash", "-l", NULL};
		const char * envp[2];

		string termenv("TERM=");
		termenv.append(::getenv("TERM"));

		envp[0] = termenv.c_str();
		envp[1] = NULL;

		::execve("/bin/bash", const_cast<char **>(argv), const_cast<char **>(envp));
		exit(EXIT_FAILURE);
	} else {
		if (winp)
			::ioctl(master, TIOCSWINSZ, winp);
	}

	return pid;
}

static void handle_client(int sock)
{
	pqc::socket_session sess(sock);
	sess.start_server();
	sess.handshake();

	if (sess.is_error()) {
		cerr << "handshake with client failed" << endl;
		exit(EXIT_FAILURE);
	}

	sess.write("pqct", 4);

	char magic[4];
	if (sess.read(magic, 4) != 4 || memcmp(magic, "pqct", 4)) {
		cerr << "wrong magic header from client" << endl;
		exit(EXIT_FAILURE);
	}

	::setenv("TERM", read_varsized_packet(sess).c_str(), 1);

	if (read_byte(sess) != 2) {
		cerr << "expected winsize packet" << endl;
		exit(EXIT_FAILURE);
	}

	struct winsize ws = read_winsize_packet(sess);
	int master;
	pid_t pid;

	pid = forkpty(&master, &ws);
	set_fd_nonblocking(master);

	struct pollfd pfds[2];

	pfds[0].fd = sock;
	pfds[0].events = POLLIN;
	pfds[1].fd = master;
	pfds[1].events = POLLIN;

	if (sess.bytes_available())
		handle_session_input(sess, master);

	while (!sess.is_closed() && !sess.is_error()) {
		int res = poll(pfds, 2, -1);

		if (res < 0) {
			if (errno == EINTR)
				continue;
			cerr << "cannot poll: " << strerror(errno) << endl;
			exit(EXIT_FAILURE);
		}

		if (pfds[0].revents)
			handle_session_input(sess, master);

		if (pfds[1].revents) {
			if (!handle_pty_input(sess, master)) {
				close(master);
				master = -1;
				break;
			}
		}
	}

	if (sess.is_error()) {
		cerr << "pqc error" << endl;
		exit(EXIT_FAILURE);
	}

	if (!sess.is_closed()) {
		if (master > -1)
			handle_pty_input(sess, master, true);
		sess.close();
	}

	close(master);

	exit(EXIT_SUCCESS);
}

static void set_reuse_addr(int sock)
{
	int val = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val))) {
		cerr << "cannot reuse address: " << strerror(errno) << endl;
		exit(EXIT_FAILURE);
	}
}

static void signal_handler(int signum)
{
	if (signum == SIGCHLD) {
		int status;
		while (waitpid(-1, &status, WNOHANG) > 0)
			;
	}
}

int main (int argc, char **argv) {
	if (argc != 2) {
		cerr << "usage: pqc-telnetd TCP_PORT" << endl << endl;
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, signal_handler);

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
	set_fd_cloexec(sock);

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
			if (errno == EINTR)
				continue;
			cerr << "cannot accept client: " << strerror(errno) << "\n";
			continue;
		}

		set_fd_cloexec(sock);

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
