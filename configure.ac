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
AC_CHECK_TOOL([HAVE_SHELL_TOOL], [shc], 0)

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup strerror])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/utils/Makefile
src/yinkad/Makefile
scripts/Makefile
])

AC_OUTPUT
