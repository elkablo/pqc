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

```sh
$ gmake
$ gmake install
```