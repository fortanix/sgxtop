#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([sgxtop], [0.1], [support@fortanix.com])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([sgxtop.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
: ${CFLAGS="-g -Wall"}
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([curses], [initscr], , echo -e "=====> Error: libncurses5-dev missing." && exit)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([clock_gettime memset strdup])

AC_CONFIG_FILES([Makefile])
#AC_CONFIG_SUBDIRS([sgxtop-0.1])
AC_OUTPUT
