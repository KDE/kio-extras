AM_CPPFLAGS = -UQT_NO_ASCII_CAST
INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

lib_LTLIBRARIES = kio_pop3.la

kio_pop3_la_SOURCES = pop3.cc md5.c
kio_pop3_la_LIBADD  = $(LIB_KIO)
kio_pop3_la_LDFLAGS = $(all_libraries) -module -avoid-version

noinst_HEADERS = pop3.h

kdelnk_DATA = pop3.protocol pop3s.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_pop3_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_pop3_la_LIBADD)
