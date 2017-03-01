#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([yinka-utils], [1.0], [https://github.com/geekerlw/yinka-utils])
AM_INIT_AUTOMAKE([1.0])
AC_CONFIG_SRCDIR([autogen.sh])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Checks for header files.
AC_CHECK_HEADERS([pthread.h sys/socket.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for tools.
AC_CHECK_TOOL([SHC], [shc])
AC_CHECK_TOOL([PKG_CONFIG], [pkg-config])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup strerror])

# Flags and lib configure
AC_SUBST(PTHREAD_LIBS, -lpthread)

PKG_CHECK_MODULES(XML, libxml-2.0 >= 2.9.3)
AC_SUBST(XML_CFLAGS)
AC_SUBST(XML_LIBS)

#PKG_CHECK_MODULES(PTHREAD, libpthread)
#AC_SUBST(PTHREAD_CFLAGS)
#AC_SUBST(PTHREAD_LIBS)

PKG_CHECK_MODULES(CURL, libcurl >= 7.47.0)
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

PKG_CHECK_MODULES(JSON_C, json-c >= 0.11.99)
AC_SUBST(JSON_C_CFLAGS)
AC_SUBST(JSON_C_LIBS)

# Out file configure
AC_CONFIG_FILES([
Makefile
src/Makefile
src/utils/Makefile
src/yinkad/Makefile
src/updater/Makefile
scripts/Makefile
data/Makefile
])

AC_OUTPUT
