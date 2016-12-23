CXXFLAGS = -fPIC -fno-exceptions -fno-stack-protector -march=native -std=c++14 -I.
LDFLAGS = -lgmpxx -lgmp -lnettle

LIBPQC_OBJS = 			\
	pqc_sha.o		\
	pqc_auth.o		\
	pqc_auth_sidhex.o	\
	pqc_asymmetric_key.o	\
	pqc_sidh_params.o	\
	pqc_sidh_key_basic.o	\
	pqc_sidh_key.o		\
	pqc_packet.o		\
	pqc_packet_reader.o	\
	pqc_session.o		\
	pqc_handshake.o		\
	pqc_kex.o		\
	pqc_kex_sidhex.o	\
	pqc_mac.o		\
	pqc_mac_hmac_sha.o	\
	pqc_cipher.o		\
	pqc_cipher_chacha20.o	\
	pqc_random.o		\
	pqc_base64.o		\
	pqc_weierstrass.o	\
	pqc_chacha.o		\
	gf.o

all: optimize libpqc.so pqc-telnet pqc-telnetd pqc-keygen weier
debug: debugize libpqc.so pqc-telnet pqc-telnetd pqc-keygen weier

optimize:
	$(eval CXXFLAGS += -O2)

debugize:
	$(eval CXXFLAGS += -ggdb)

libpqc.so: $(LIBPQC_OBJS)
	g++ $(CXXFLAGS) $(LDFLAGS) -shared -o libpqc.so $(LIBPQC_OBJS)

pqc-telnet: pqc-telnet.o libpqc.so
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ pqc-telnet.o -L. -lpqc -Wl,-rpath,.

pqc-telnetd: pqc-telnetd.o libpqc.so
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ pqc-telnetd.o -L. -lpqc -Wl,-rpath,.

pqc-keygen: pqc-keygen.o libpqc.so
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ pqc-keygen.o -L. -lpqc -Wl,-rpath,.

weier: pqc_weierstrass_main.o libpqc.so
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ pqc_weierstrass_main.o -L. -lpqc -Wl,-rpath,.

pqc_weierstrass_main.o: pqc_weierstrass.cpp
	g++ -DWEIERSTRASS_MAIN $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(LIBPQC_OBJS) libpqc.so pqc-telnet pqc-telnet.o pqc-telnetd \
		pqc-telnetd.o pqc-keygen pqc-keygen.o pqc_weierstrass_main.o weier
