dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([deadbeef], [1.0], [peterlamby@web.de])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AM_PROG_AR

LT_INIT

AC_CHECK_HEADER([stdint.h])

PKG_CHECK_MODULES([GLIB_DEPS], [glib-2.0])
PKG_CHECK_MODULES([GIO_DEPS], [gio-2.0])
PKG_CHECK_MODULES([GTHREAD_DEPS], [gthread-2.0])

dnl Search for deadbeef installation directory
AC_PREFIX_PROGRAM([deadbeef])
if test "x$prefix" != xNone; then
	PLUGIN_INST="$prefix/lib/deadbeef"
else
	dnl Use default if nothing is found:
	PLUGIN_INST="$ac_default_prefix/lib/deadbeef"
fi
AC_SUBST([PLUGIN_INST])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
