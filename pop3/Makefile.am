INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(all_includes)

####### Files

lib_LTLIBRARIES = kio_pop3.la kio_spop3.la

kio_pop3_la_SOURCES = pop3.cc md5.c
kio_pop3_la_LIBADD  = $(LIB_KIO)
kio_pop3_la_LDFLAGS = $(all_libraries) -module -avoid-version

kio_spop3_la_SOURCES = spop3.cc md5.c
kio_spop3_la_LIBADD  = $(LIB_KIO) $(LIBSSL)
kio_spop3_la_LDFLAGS = $(all_libraries) $(SSL_LDFLAGS) -module -avoid-version

noinst_HEADERS = pop3.h

kioconfig_DATA = pop3.desktop spop3.desktop
kioconfigdir = $(kde_confdir)/protocols

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_pop3_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_pop3_la_LIBADD)
linktest_LDADD = $(kio_spop3_la_LIBADD)

# Please do something about this if it's ugliness offends you.
# I'm stumped (and also a newbie at automake/autoconf)
spop3.cc: pop3.cc
	echo "#define SPOP3" >spop3.cc
	cat pop3.cc >>spop3.cc

CLEANFILES = spop3.cc

