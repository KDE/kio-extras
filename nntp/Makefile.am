INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(all_includes)

####### Files

lib_LTLIBRARIES = kio_nntp.la

kio_nntp_la_SOURCES = nntp.cpp
kio_nntp_la_LIBADD  = $(LIB_KIO)
kio_nntp_la_LDFLAGS = $(all_libraries) -module -avoid-version

noinst_HEADERS = nntp.h

kdelnk_DATA = nntp.protocol
kdelnkdir = $(kde_servicesdir)

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_nntp_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_nntp_la_LIBADD)

