## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

lib_LTLIBRARIES = kio_tar.la

kio_tar_la_SOURCES = tar.cc ktar.cpp
kio_tar_la_LIBADD = $(LIB_KSYCOCA)
kio_tar_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

check_PROGRAMS = ktartest

ktartest_SOURCES =  ktar.cpp ktartest.cpp
ktartest_LDADD = $(LIB_KSYCOCA)

noinst_HEADERS = tar.h

kdelnk_DATA = tar.protocol
kdelnkdir = $(kde_servicesdir)

