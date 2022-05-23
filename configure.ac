dnl Process this file with autoconf to produce a configure script.

m4_define([plugin_version_major], [1])
m4_define([plugin_version_minor], [15])
m4_define([plugin_version], [plugin_version_major.plugin_version_minor])

AC_PREREQ([2.69])
AC_INIT([deadbeef-mpris2-plugin], [plugin_version], [peterlamby@web.de], [DeaDBeeF MPRIS2 Plugin], [https://github.com/Serranya/deadbeef-mpris2-plugin])
AC_CONFIG_SRCDIR([src/mpris.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])

dnl Override $PACKAGE to make $pkglibdir use deadbeef search path
PACKAGE=deadbeef

AC_PROG_CC_C99
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

AC_DEFINE(PLUGIN_VERSION_MAJOR, [plugin_version_major],
          [Define to the plug-in major version])
AC_DEFINE(PLUGIN_VERSION_MINOR, [plugin_version_minor],
          [Define to the plug-in minor version])

LT_INIT

PKG_CHECK_MODULES([GLIB_DEPS], [glib-2.0], , AC_MSG_ERROR([glibc-2 is required for this package]))
PKG_CHECK_MODULES([GIO_DEPS], [gio-2.0], , AC_MSG_ERROR([gio-2 is required for this package]))
PKG_CHECK_MODULES([GIOUNIX_DEPS], [gio-unix-2.0], , AC_MSG_ERROR([gio-unix-2 is required for this package]))
PKG_CHECK_MODULES([GTHREAD_DEPS], [gthread-2.0], , AC_MSG_ERROR([gthread-2 is required for this package]))

AS_IF([test "x$ac_cv_prog_cc_c99" = "xno"], AC_MSG_ERROR([C99 Support is required]))

AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug], [The plugin will print debug options to stdout.]),
              AC_DEFINE([MPRIS__DEBUG]))

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
