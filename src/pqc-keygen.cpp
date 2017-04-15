#include <iostream>
#include <fstream>
#include <pqc_base64.hpp>
#include <pqc_auth.hpp>
#include <pqc_sha.hpp>

using namespace std;
using namespace pqc;

int main (int argc, char **argv) {
	if (argc != 2 && argc != 4) {
		cerr << "usage: pqc-keygen TYPE private-file public-file" << endl
			<< "   or: pqc-keygen TYPE" << endl << endl
			<< "   private key will be stored in private-file" << endl
			<< "   public key will be stored in public-file" << endl
			<< "   if private-file and public-file not present, key will be printed to standard output" << endl
			<< "   TYPE may currently be only SIDHex-sha512" << endl << endl;
		exit(EXIT_FAILURE);
	}

	ofstream priv_file, pub_file;
	if (argc == 4) {
		priv_file.open(argv[2], ios_base::out | ios_base::binary);
		if (!priv_file) {
			cerr << "cannot open " << argv[2] << endl << endl;
			exit(EXIT_FAILURE);
		}

		pub_file.open(argv[3], ios_base::out | ios_base::binary);
		if (!pub_file) {
			cerr << "cannot open " << argv[3] << endl << endl;
			exit(EXIT_FAILURE);
		}
	}

	shared_ptr<auth> auth_ = auth::create(auth::from_string(argv[1]));
	if (!auth_) {
		cerr << "wrong auth type: " << argv[1] << endl << endl;
		exit(EXIT_FAILURE);
	}

	shared_ptr<asymmetric_key> key = auth_->generate_key();
	if (!key) {
		cerr << "key generation failed" << endl << endl;
		exit(EXIT_FAILURE);
	}

	string priv = key->export_private();
	string pub = key->export_public();

	if (argc == 2) {
		cout << "Key ID: " << sha256(pub) << endl
			<< "Private part: " << base64_encode(priv) << endl
			<< "Public part: " << base64_encode(pub) << endl << endl;
	} else {
		string id = sha256(pub, false);
		priv_file << id << priv;
		priv_file.flush();
		priv_file.close();
		pub_file << id << pub;
		pub_file.flush();
		pub_file.close();
	}

	return 0;
}
