## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

lib_LTLIBRARIES = kgzipfilter.la kio_tar.la

kio_tar_la_SOURCES = tar.cc ktar.cpp
kio_tar_la_LIBADD = $(LIB_KIO) kgzipfilter.la
kio_tar_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

kgzipfilter_la_SOURCES = kgzipfilter.cpp kgzipdev.cpp 
kgzipfilter_la_LIBADD = $(LIB_KDECORE) $(LIBZ)
kgzipfilter_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

check_PROGRAMS = ktartest kgziptest

ktartest_SOURCES = ktar.cpp ktartest.cpp
ktartest_LDADD = $(LIB_KDECORE) kgzipfilter.la
kgziptest_SOURCES =  kgziptest.cpp
kgziptest_LDADD = $(LIB_KDECORE) kgzipfilter.la


noinst_HEADERS = tar.h

kdelnk_DATA = tar.protocol
kdelnkdir = $(kde_servicesdir)

