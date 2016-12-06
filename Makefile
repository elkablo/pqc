CXXFLAGS = -fPIC -ggdb -fno-exceptions -fno-stack-protector -march=native -std=c++14 -I.
LDFLAGS = -lgmpxx -lgmp -lcrypto -lnettle

OBJS = 				\
	pqc.o			\
	pqc_handshake.o		\
	pqc_kex.o		\
	pqc_mac.o		\
	pqc_mac_hmac_sha.o	\
	pqc_cipher.o		\
	pqc_cipher_chacha20.o	\
	pqc_random.o		\
	pqc_base64.o		\
	pqc_weierstrass.o	\
	pqc_chacha.o		\
	gf.o

WEIER_OBJS = pqc_weierstrass_main.o gf.o pqc_random.o pqc_chacha.o

all: pqc weier

pqc: $(OBJS)
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

weier: $(WEIER_OBJS)
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ $(WEIER_OBJS)

pqc_weierstrass_main.o: pqc_weierstrass.cpp
	g++ -DWEIERSTRASS_MAIN $(CXXFLAGS) -c -o $@ $<

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJS) $(WEIER_OBJS) weier pqc
