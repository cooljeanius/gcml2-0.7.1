dnl# gcml2-0.7.1/configure.ac                                  -*- Autoconf -*-
dnl# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.70])
dnl# bug report address is the most recent person to have a ChangeLog entry
dnl# in this directory from before I picked it up:
AC_INIT([gcml2],[0.7.1],[gnb@alphalink.com.au])
AC_CONFIG_SRCDIR([check/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE
AM_SANITY_CHECK
AC_SEARCH_LIBS([strerror],[cposix])dnl

# Checks for programs.
if test "x${AWK}" = "x"; then
  test -z "${AWK}"
  AC_PROG_AWK
else
  test -n "${AWK}" && export AWK
fi
AC_PROG_YACC
if test "x${CC}" = "x"; then
  test -z "${CC}"
  AC_PROG_CC
else
  test -n "${CC}" && export CC
fi
AC_PROG_CPP
AC_PROG_GCC_TRADITIONAL
AC_PROG_LEX
AC_PROG_INSTALL
AC_PROG_LN_S
AX_CHECK_GNU_MAKE
AC_PATH_PROG([GLIB_CONFIG],[glib-config])
AC_PATH_PROG([LIBGLADE_CONFIG],[libglade-config])
AC_PATH_PROG([RPM],[rpm],[rpm])
AC_PATH_PROG([SED],[sed],[sed])
AC_PATH_PROG([GSED],[gsed],[gsed])dnl

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcml':
AC_CHECK_LIB([cml],[main])
# FIXME: Replace `main' with a function in `-lncurses':
AC_CHECK_LIB([ncurses],[main])
# FIXME: Replace `main' with a function in `-lglib':
AC_CHECK_LIB([glib],[main])
# FIXME: Replace `main' with a function in `-lImlib':
AC_CHECK_LIB([Imlib],[main])
# FIXME: Replace `main' with a function in `-lgdk_imlib':
AC_CHECK_LIB([gdk_mlib],[main])
# FIXME: Replace `main' with a function in `-lgtk':
AC_CHECK_LIB([gtk],[main])
AM_PATH_IMLIB
AM_PATH_GTK
AM_PATH_GLIB
AM_PATH_GDK_PIXBUF
AX_WITH_CURSES_PANEL
AX_WITH_CURSES_MENU
AX_WITH_CURSES_FORM
gt_INTL_MACOSX

# Checks for header files.
dnl# autoupdate says this is obsolete:
dnl# The preprocessor macro `STDC_HEADERS' is obsolete.
dnl# Except in unusual embedded environments, you can safely include all
dnl# ISO C90 headers unconditionally.
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
if test "x${EGREP}" = "x"; then
  test -z "${EGREP}"
  AC_PROG_EGREP
fi
AC_HEADER_STDBOOL dnl# also "_CHECK"s it
AC_CHECK_HEADERS([fcntl.h gdk_imlib.h glade/glade.h glib.h glib/glib.h \
                  gtk/gtk.h libintl.h malloc.h malloc/malloc.h memory.h \
                  ncurses.h stddef.h sys/time.h])
AC_CHECK_HEADERS_ONCE([inttypes.h stdint.h stdlib.h string.h unistd.h])dnl

if test "x${ac_cv_header_inttypes_h}" = "x"; then
  test -z "${ac_cv_header_inttypes_h}"
  AC_CHECK_HEADERS([inttypes.h])
fi
if test "x${ac_cv_header_stdint_h}" = "x"; then
  test -z "${ac_cv_header_stdint_h}"
  AC_CHECK_HEADERS([stdint.h])
fi
if test "x${ac_cv_header_unistd_h}" = "x"; then
  test -z "${ac_cv_header_unistd_h}"
  AC_CHECK_HEADERS([unistd.h])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset mkdir strcasecmp strchr strerror \
                strpbrk strrchr strspn strstr strtol strtoul])dnl

# Output.
AC_CONFIG_FILES([gcml2.spec])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
