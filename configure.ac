#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([mercury], [1.00], [gnosticist@protonmail.com])
AX_CXX_COMPILE_STDCXX_17([noext], [mandatory])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile coinbase/Makefile])
AM_INIT_AUTOMAKE

# GNU recommended way to determine host variables (OS, etc)
AC_CANONICAL_HOST

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

# AM_PROG_CC_C_O seems to be the better choice when working with automake
AM_PROG_CC_C_O

# Checks for libraries.
AX_CHECK_OPENSSL([], [AC_MSG_ERROR([OpenSSL not found.])])
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([boost/multiprecision/cpp_int.hpp], [], [AC_MSG_ERROR(Boost multiprecision library not found.)])
AC_LANG_POP([C++])
AC_MSG_CHECKING([for cpprest])
AC_LANG_PUSH(C++)
SAVE_LIBS="$LIBS"
LIBS="$LIBS -lcpprest"
AC_TRY_LINK([#include <some_cpp_lib.hpp>], 
        [SomeCppLib object;],
        has_some_cpp_lib=1,
        has_some_cpp_lib=0)
AC_LANG_POP(C++)
if test $has_some_cpp_lib = 0; then
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([yes])
fi

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror])

AC_OUTPUT

