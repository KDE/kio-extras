INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(all_includes)

####### Files

lib_LTLIBRARIES = kio_pop3.la

kio_pop3_la_SOURCES = pop3.cc md5.c
kio_pop3_la_LIBADD  = $(LIB_KIO)
kio_pop3_la_LDFLAGS = $(all_libraries) -module -avoid-version

noinst_HEADERS = pop3.h

kioconfig_DATA = pop3.desktop
kioconfigdir = $(kde_confdir)/protocols

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_pop3_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_pop3_la_LIBADD)

