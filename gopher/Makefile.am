# $Id$
# Makefile for kdebase/kioslave/gopher

INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(all_includes)

####### Files

lib_LTLIBRARIES = kio_gopher.la

kio_gopher_la_SOURCES = gopher.cc
kio_gopher_la_LIBADD  = $(LIB_KIO)
kio_gopher_la_LDFLAGS = $(all_libraries) -module -avoid-version

noinst_HEADERS = gopher.h

kdelnk_DATA = gopher.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_gopher_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_gopher_la_LIBADD)
