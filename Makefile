CXXFLAGS = -fPIC -ggdb -fno-exceptions -fno-stack-protector -march=native -std=c++14
LDFLAGS = -lgmpxx -lgmp

OBJS = weierstrass.o gf.o

all: weier pqc

gf.o: gf.hpp
weierstrass.o: gf.hpp

weier: $(OBJS)
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJS) weier

PQC_OBJS = pqc.cpp pqc_parser.cpp pqc_kex.cpp pqc_mac.cpp pqc_cipher.cpp pqc_random.cpp pqc_base64.cpp

pqc: $(PQC_OBJS)
	g++ -Wall -ggdb -std=c++11 -o pqc $(PQC_OBJS) -lcrypto -lnettle
