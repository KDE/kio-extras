## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES = $(all_includes)

####### Files

lib_LTLIBRARIES = kio_tar.la

kio_tar_la_SOURCES = tar.cc
kio_tar_la_LIBADD = $(LIB_KIO)
kio_tar_la_LDFLAGS = -module -avoid-version

noinst_HEADERS = tar.h

kdelnk_DATA = tar.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_tar_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_tar_la_LIBADD)

