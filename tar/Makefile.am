## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

lib_LTLIBRARIES = kio_tar.la

kio_tar_la_SOURCES = tar.cc   kgzipdev.cpp ktar.cpp
kio_tar_la_LIBADD = $(LIB_KIO)
kio_tar_la_LDFLAGS = -module -avoid-version


check_PROGRAMS = ktartest kgziptest

ktartest_SOURCES = kgzipdev.cpp ktar.cpp ktartest.cpp
ktartest_LDADD = $(LIB_KDECORE)
kgziptest_SOURCES = kgzipdev.cpp kgziptest.cpp
kgziptest_LDADD = $(LIB_KDECORE)


noinst_HEADERS = tar.h

kdelnk_DATA = tar.protocol
kdelnkdir = $(kde_servicesdir)

