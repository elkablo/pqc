#include <unistd.h>

#include "pqc.hpp"

namespace pqc
{



}

std::string server_auth_callback(const std::string key_id)
{
	std::string private_key_data;
	// find private key data for server key key_id (from filesystem or cache)
	return private_key_data;
}

std::string client_auth_callback(const std::string key_id)
{
	std::string private_key_data;
	// find private key data for client key key_id (from filesystem or cache)
	return private_key_data;
}

void server (int sock)
{
	char *from_client = new char[65*1024];
	char *to_client = new char[65*1024];
	ssize_t rd, wr;
	int res;

	/* example server */
	pqc::server server();

	/* disable plain cipher */
	server.enable_cipher(PQC_CIPHER_PLAIN, false);

	/* set callback for private key data */
	server.set_auth_callback(server_auth_callback);
	/* or just one auth ? */
	// server.set_auth(server_private_key);

	/* rekey after 1GB */
	server.set_rekey_after(1024*1024*1024);

	rd = ::read(sock, from_client, 65*1024);

	wr = 65*1024;
	res = server.handshake_init(to_client, wr, from_client, rd);
	if (res > wr) {
		/* need more space in to_client buffer? */
		return;
	} else if (res < 0) {
		std::cout << server.get_error() << "\n";
		return;
	}

	wr = ::write(sock, to_client, res);
	rd = ::read(sock, from_client, 65*1024);

	res = server.handshake_fini(from_client, rd);
	if (res < 0) {
	
	}


}

void client (int sock)
{
	char *from_server = new char[65*1024];
	char *to_server = new char[65*1024];
	ssize_t rd, wr;

	int res;
	pqc::client client();

	client.set_server_name("server");
	client.set_server_auth(" bla bla bla");

	wr = 65*1024;
	res = client.handshake_init(to_server, wr);
	if (res > wr) {
		/* !! */
		return;
	} else if (res < 0) {
		/* error? */
		return;
	}

	wr = ::write(sock, to_server, res);
	rd = ::read(sock, from_server, 65*1024);

	wr = 65*1024;
	res = client.handshake_fini(to_server, wr, from_server, rd);
	if (res > wr) {
	
	} else if (res < 0) {
	
	}

	wr = ::write(sock, to_server, res);
}

