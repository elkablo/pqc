CXXFLAGS = -fPIC -ggdb -fno-exceptions -fno-stack-protector -march=native -std=c++14
LDFLAGS = -lgmpxx -lgmp

OBJS = weierstrass.o gf.o

all: weier

gf.o: gf.hpp
weierstrass.o: gf.hpp

weier: $(OBJS)
	g++ $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJS) weier
