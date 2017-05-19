
AC_PREREQ([2.69])
AC_INIT([pqc], [0.1])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE()
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES])

CFLAGS="$CFLAGS"
CXXFLAGS="$CXXFLAGS"
LDFLAGS="$LDFLAGS"

LT_INIT

AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

#AC_CHECK_HEADERS([gmp.h], , AC_MSG_ERROR([Codecrypt requires gmp.h]))
#AC_CHECK_LIB(gmp, __gmpz_init, , AC_MSG_ERROR([Codecrypt requires libgmp]))
#TODO gmpxx
#TODO nettle

AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h sys/ioctl.h sys/socket.h termios.h unistd.h])
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit dup2 memchr memmove memset pow setenv socket sqrt strcasecmp strchr strerror strncasecmp strndup strtoul])

AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
