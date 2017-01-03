override CPPFLAGS += -I.
override CXXFLAGS += -fPIC -fno-stack-protector -std=c++14
override LDFLAGS += -lgmpxx -lgmp -lnettle

CXX ?= g++
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
INSTALL ?= install

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
	pqc_gf.o

HEADERS =			\
	pqc_asymmetric_key.hpp	\
	pqc_auth.hpp		\
	pqc_auth_sidhex.hpp	\
	pqc_base64.hpp		\
	pqc_chacha.hpp		\
	pqc_cipher_chacha20.hpp	\
	pqc_cipher.hpp		\
	pqc_enumset.hpp		\
	pqc_enums.hpp		\
	pqc_gf.hpp		\
	pqc_handshake.hpp	\
	pqc_kex.hpp		\
	pqc_kex_sidhex.hpp	\
	pqc_mac_hmac_sha.hpp	\
	pqc_mac.hpp		\
	pqc_packet.hpp		\
	pqc_packet_reader.hpp	\
	pqc_random.hpp		\
	pqc_session.hpp		\
	pqc_sha.hpp		\
	pqc_sidh_key_basic.hpp	\
	pqc_sidh_key.hpp	\
	pqc_sidh_params.hpp	\
	pqc_socket_session.hpp	\
	pqc_weierstrass.hpp

BINARIES =			\
	pqc-telnet		\
	pqc-telnetd		\
	pqc-keygen		\
	test_weierstrass

all: optimize libpqc.so $(BINARIES)
debug: debugize libpqc.so $(BINARIES)

install: all
	$(INSTALL) -d $(BINDIR) $(LIBDIR) $(INCLUDEDIR)
	$(INSTALL) $(BINARIES) $(BINDIR)/
	$(INSTALL) libpqc.so $(LIBDIR)/
	$(INSTALL) --mode=0644 $(HEADERS) $(INCLUDEDIR)/

optimize:
	$(eval override CXXFLAGS += -O2)

debugize:
	$(eval override CXXFLAGS += -ggdb)

libpqc.so: $(LIBPQC_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared -o libpqc.so $(LIBPQC_OBJS)

$(BINARIES): %:%.o libpqc.so
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $< -L. -lpqc -Wl,-rpath,.

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(LIBPQC_OBJS) libpqc.so $(BINARIES) $(addsuffix .o,$(BINARIES))
