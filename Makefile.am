pkglib_LTLIBRARIES = mpris.la

ACLOCAL_AMFLAGS= -I m4

mpris_la_SOURCES=src/mpris.c src/mprisServer.c src/mprisServer.h src/logging.c src/logging.h
mpris_la_CFLAGS=${GIO_DEPS_CFLAGS} ${GTHREAD_DEPS_CFLAGS} ${GLIB_DEPS_CFLAGS} ${GTK_DEPS_CFLAGS}
mpris_la_LDFLAGS=${GIO_DEPS_LIBS} ${GTK_DEPS_LIBS} ${GTHREAD_DEPS_LIBS} ${GLIB_DEPS_LIBS} -module -avoid-version -shared
