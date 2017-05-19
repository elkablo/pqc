#!/bin/bash

# simple autogen script that generates basic layout for autotools.

COMMON_CPPFLAGS="-I/usr/local/include"
COMMON_CFLAGS="-Wall"
COMMON_CXXFLAGS="${COMMON_CFLAGS} -std=c++14"
COMMON_LDFLAGS="-L/usr/local/lib"
COMMON_LDADD=""

OUT=Makefile.am
touch NEWS AUTHORS ChangeLog

make_am() {
	cat << EOF
AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4
dist_noinst_SCRIPTS = autogen.sh

pqc_includedir = \$(includedir)/pqc
pqc_include_HEADERS = `echo include/*.hpp`

lib_LTLIBRARIES = libpqc.la
libpqc_ladir  = lib
libpqc_la_SOURCES = `echo lib/*.cpp`
libpqc_la_CPPFLAGS = ${COMMON_CPPFLAGS} -I\$(srcdir)/include/
libpqc_la_CFLAGS = ${COMMON_CFLAGS}
libpqc_la_CXXFLAGS = ${COMMON_CXXFLAGS}
libpqc_la_LDFLAGS = ${COMMON_LDFLAGS}
libpqc_la_LIBADD = ${COMMON_LDADD} -lgmpxx -lgmp -lnettle

bin_PROGRAMS =

EOF

	for prog in `echo src/*.cpp` ; do
		progname=`basename -s.cpp ${prog}`
		name=`echo $progname |tr '-' '_'`
		cat << EOF
bin_PROGRAMS += ${progname}
${name}dir = src
${name}_SOURCES = ${prog}
${name}_CPPFLAGS = ${COMMON_CPPFLAGS} -I\$(srcdir)/include/
${name}_CFLAGS = ${COMMON_CFLAGS}
${name}_CXXFLAGS = ${COMMON_CXXFLAGS}
${name}_LDFLAGS = ${COMMON_LDFLAGS}
${name}_LDADD = ${COMMON_LDADD} libpqc.la -lgmp -lgmpxx
EOF
	done
}

make_am > $OUT

if [[ "$OSTYPE" == "darwin"* ]]; then
  glibtoolize --force && aclocal && autoconf && automake --add-missing
else
  libtoolize --force && aclocal && autoconf && automake --add-missing
fi
