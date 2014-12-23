dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([deadbeef-mpris2-plugin], [1.3], [peterlamby@web.de])
AC_CONFIG_SRCDIR([src/mpris.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])

dnl Override $PACKAGE to make $pkglibdir use deadbeef search path
PACKAGE=deadbeef

AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

LT_INIT

PKG_CHECK_MODULES([GTK_DEPS], [gtk+-2.0], , AC_MSG_ERROR([GTK-2 is required for this package]))
PKG_CHECK_MODULES([GLIB_DEPS], [glib-2.0], , AC_MSG_ERROR([glibc-2 is required for this package]))
PKG_CHECK_MODULES([GIO_DEPS], [gio-2.0], , AC_MSG_ERROR([gio-2 is required for this package]))
PKG_CHECK_MODULES([GTHREAD_DEPS], [gthread-2.0], , AC_MSG_ERROR([gthread-2 is required for this package]))
PKG_CHECK_MODULES([CURL_DEPS], [libcurl], , AC_MSG_ERROR([libcurl is required for this package]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
