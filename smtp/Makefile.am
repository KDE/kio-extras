INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

lib_LTLIBRARIES = kio_smtp.la

kio_smtp_la_SOURCES = smtp.cc
kio_smtp_la_LIBADD  = $(LIB_KIO)
kio_smtp_la_LDFLAGS = $(all_libraries) -module -avoid-version

noinst_HEADERS = smtp.h

kdelnk_DATA = smtp.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_smtp_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_smtp_la_LIBADD)
