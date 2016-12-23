override CPPFLAGS += -I.
override CXXFLAGS += -fPIC -fno-stack-protector -std=c++14
override LDFLAGS += -lgmpxx -lgmp -lnettle

CXX ?= g++

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

BINARIES =			\
	pqc-telnet		\
	pqc-telnetd		\
	pqc-keygen		\
	test_weierstrass

all: optimize libpqc.so $(BINARIES)
debug: debugize libpqc.so $(BINARIES)

optimize:
	$(eval CXXFLAGS += -O2)

debugize:
	$(eval CXXFLAGS += -ggdb)

libpqc.so: $(LIBPQC_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -o libpqc.so $(LIBPQC_OBJS)

$(BINARIES): %:%.o libpqc.so
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -L. -lpqc -Wl,-rpath,.

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(LIBPQC_OBJS) libpqc.so $(BINARIES) $(addsuffix .o,$(BINARIES))
