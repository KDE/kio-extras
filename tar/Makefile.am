## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
LIBBZ2 = -lbz2
METASOURCES = AUTO

lib_LTLIBRARIES = libkio2.la kgzipfilter.la kbzip2filter.la kio_tar.la

kio_tar_la_SOURCES = tar.cc ktar.cpp
kio_tar_la_LIBADD = $(LIB_KIO) libkio2.la
kio_tar_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

libkio2_la_SOURCES = kfilterbase.cpp kfilterdev.cpp kcomprmanager.cpp
libkio2_la_LIBADD = $(LIB_KSYCOCA)
libkio2_la_LDFLAGS = $(all_libraries) -no-undefined


kgzipfilter_la_SOURCES = kgzipfilter.cpp
kgzipfilter_la_LIBADD = libkio2.la $(LIBZ)
kgzipfilter_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

kbzip2filter_la_SOURCES = kbzip2filter.cpp
kbzip2filter_la_LIBADD = libkio2.la $(LIBBZ2)
kbzip2filter_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

check_PROGRAMS = ktartest kfiltertest

ktartest_SOURCES =  kfilterdev.cpp ktar.cpp ktartest.cpp
ktartest_LDADD = $(LIB_KDECORE) libkio2.la
kfiltertest_SOURCES =  kfilterdev.cpp kfiltertest.cpp
kfiltertest_LDADD = $(LIB_KDECORE) libkio2.la

noinst_HEADERS = tar.h

kdelnk_DATA = tar.protocol
kdelnkdir = $(kde_servicesdir)

kde_servicetypes_DATA = kcomprfilter.desktop
kde_services_DATA = kbzip2filter.desktop kgzipfilter.desktop 

