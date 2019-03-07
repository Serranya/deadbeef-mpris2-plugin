pkglib_LTLIBRARIES = mpris.la

ACLOCAL_AMFLAGS= -I m4

mpris_la_SOURCES=src/mpris.c src/mprisServer.c src/mprisServer.h src/logging.c src/logging.h src/artwork.h
mpris_la_CFLAGS=${GIO_DEPS_CFLAGS} ${GIOUNIX_DEPS_CFLAGS} ${GTHREAD_DEPS_CFLAGS} ${GLIB_DEPS_CFLAGS}
mpris_la_LDFLAGS=-module -avoid-version -shared
mpris_la_LIBADD=${GIO_DEPS_LIBS} ${GIOUNIX_DEPS_LIBS} ${GTHREAD_DEPS_LIBS} ${GLIB_DEPS_LIBS}

EXTRA_DIST=LICENSE
