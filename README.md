# libpqc - the post-quantum cryptography library

**pqc** is a C++ library that can be used to secure socket-like connections with
cryptographic mechanisms able to resist attacks by adversaries in possession of
a large quantum computer.

# Requirements

A C++14 capable compiler is required to compile the library. The library has been
successfuly compiled with GCC version 5.4 and Clang version 3.8.

This software is dependent on the GNU Nettle library.

GNU Make is also required for compilation.

# Installation

## Linux

This command sequence compiles the library and installs it into /usr/local

```sh
$ make
$ make install
```

Alternatively, it can be compiled with additional compiler flags and installed 
into another directory:

```sh
$ make CXXFLAGS="-march=native"
$ make install PREFIX=/usr
```

## FreeBSD

It is required to use the Clang compiler on FreeBSD. Also, if the Nettle library
is installed in /usr/local, the compiler needs to be told.

```sh
$ gmake CXX=clang++ CPPFLAGS=-I/usr/local/include LDFLAGS=/usr/local/lib
$ gmake install
```

# Example Programs

The library comes with several example programs.

## pqc-keygen

The pqc-kegyen utility is a utility that can be used to generate authentication keys
for the other two programs. To create a keypair and store the public part into the file
server.pub and the private part into the file server.priv, run:

```sh
$ pqc-keygen SIDHex-sha512 server.priv server.pub
```

## pqc-telnetd

The pqc-telnetd is a server that can expose the shell of the machine it is running on
to remote users who connect with the pqc-telnet program. To start the server on TCP port
8822 with the private key from the server.priv file, run the following command:

```sh
$ pqc-telnetd server.priv 8822
```

## pqc-telnet

The pqc-telnet is the corresponding client program for pqc-telnetd. To connect to
a machine running the pqc-telnetd on IP address 10.20.30.40 and TCP port 8822, and
verify the server with the public key stored in the server.pub file, run:

```sh
$ pqc-telnet server.pub 10.20.30.40 8822
```

If the connection is successful, a command prompt should be shown.
