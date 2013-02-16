#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gcml2], [0.7.1], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([check/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AM_SANITY_CHECK
AC_ISC_POSIX

# Checks for programs.
AC_PROG_AWK
AC_PROG_YACC
AC_PROG_CC
AC_PROG_CPP
AC_PROG_GCC_TRADITIONAL
AC_PROG_LEX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PATH_PROG([LIBGLADE_CONFIG],[libglade-config])
AC_PATH_PROG([SED],[sed])
AC_PATH_PROG([GSED],[gsed])

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lcml':
AC_CHECK_LIB([cml], [main])
# FIXME: Replace `main' with a function in `-lncurses':
AC_CHECK_LIB([ncurses], [main])
# FIXME: Replace `main' with a function in `-lglib':
AC_CHECK_LIB([glib], [main])
# FIXME: Replace `main' with a function in `-lImlib':
AC_CHECK_LIB([Imlib], [main])
# FIXME: Replace `main' with a function in `-lgdk_imlib':
AC_CHECK_LIB([gdk_mlib], [main])
# FIXME: Replace `main' with a function in `-lgtk':
AC_CHECK_LIB([gtk], [main])
AM_PATH_IMLIB
AM_PATH_GTK
AM_PATH_GLIB
AM_PATH_GDK_PIXBUF
AX_WITH_CURSES_PANEL
AX_WITH_CURSES_MENU
AX_WITH_CURSES_FORM
gt_INTL_MACOSX

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h gdk_imlib.h glade/glade.h glib.h gtk/gtk.h libintl.h malloc.h malloc/malloc.h memory.h ncurses.h stddef.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset mkdir strcasecmp strchr strerror strpbrk strrchr strspn strstr strtol strtoul])

AC_CONFIG_FILES([gcml2.spec])
AC_OUTPUT
