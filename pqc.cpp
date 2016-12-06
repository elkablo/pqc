#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <poll.h>
#include <pqc_session.hpp>

void waitfd(int fd, int ms)
{
	struct pollfd pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	poll(&pfd, 1, ms);
}

void sess_write(pqc::session& sess, int fd, const std::string& str = "")
{
	if (str.size()) {
		sess.write(str.c_str(), str.size());
		if (sess.is_error()) {
			std::cout << "sess_write is error\n";
			return;
		}
	}

	char buf[4096];
	size_t sz;
	while ((sz = sess.bytes_outgoing_available()) > 0) {
		if (sz > 4096)
			sz = 4096;
		sess.read_outgoing(buf, sz);
		if (write(fd, buf, sz) < (ssize_t) sz) {
			perror("write");
			exit(1);
		} else {
			printf("--Written %zi bytes to socket %i.\n", sz, fd);
		}
	}
}

std::string sess_read(pqc::session& sess, int fd, int ms = 1000, bool dontread = false)
{
	char buf[4096];

	waitfd(fd, ms);
	std::string result;

	while (1) {
		int value;
		if (ioctl(fd, FIONREAD, &value) < 0) {
			perror("ioctl");
			exit(1);
		}

		if (value == 0)
			break;

		if (value > 4096)
			value = 4096;

		if (read(fd, buf, value) < value) {
			perror("read");
			exit(1);
		}

		printf("--Read %i bytes from socket %i.\n", value, fd);
		sess.write_incoming(buf, value);
		if (sess.is_error())
			return "";
	}

	if (dontread)
		return "";

	result.resize(sess.bytes_available());
	sess.read(&result[0], sess.bytes_available());
	return result;
}

void move_between(pqc::session& sess, int fd)
{
	sess_write(sess, fd);
	sess_read(sess, fd, 1000, true);
}

int do_server(int fd)
{
//	char buf[4096];
//	size_t sz;
	pqc::session sess;

	sess.start_server();
	while (!sess.is_handshaken()) {
		move_between(sess, fd);
		if (sess.is_error()) {
			std::cerr << "server: error\n";
			return 1;
		}
	}

	std::cout << sess_read(sess, fd, 2000) << "\n";

	std::cout << "server done\n";

	return 0;
}

int do_client(int fd)
{
//	char buf[4096];
//	size_t sz;
	pqc::session sess;

	sess.start_client("google");
	while (!sess.is_handshaken()) {
		move_between(sess, fd);
		if (sess.is_error()) {
			std::cerr << "client: error\n";
			return 1;
		}
	}

	sess_write(sess, fd, "test1");

	std::cout << "client done\n";

	return 0;
}

int main () {
	int sv[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) {
		perror("socketpair");
		return 1;
	}

	pid_t pid = fork();
	if (pid == 0) {
		close(sv[1]);
		return do_client(sv[0]);
	} else if (pid > 0) {
		close(sv[0]);
		int res = do_server(sv[1]);
		wait(NULL);
		return res;
	} else {
		perror("fork");
		return 1;
	}
}
