INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

lib_LTLIBRARIES = kio_imap4.la

kio_imap4_la_SOURCES = imap4.cc base64md5.cc response.cc md5.cc
kio_imap4_la_LIBADD  = $(LIB_KIO)
kio_imap4_la_LDFLAGS = $(all_libraries) -module -avoid-version

noinst_HEADERS = imap4.h

kdelnk_DATA = imap4.protocol imaps.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_imap4_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_imap4_la_LIBADD)
