AM_CPPFLAGS = -UQT_NO_ASCII_CAST
INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)

####### Files

if SPOP3
DO_SPOP3 = kio_spop3.la
DO_SPOP3_DESKTOP = spop3.desktop
else
DO_SPOP3 = 
DO_SPOP3_DESKTOP = 
endif

lib_LTLIBRARIES = kio_pop3.la $(DO_SPOP3)

kio_pop3_la_SOURCES = pop3.cc md5.c
kio_pop3_la_LIBADD  = $(LIB_KIO)
kio_pop3_la_LDFLAGS = $(all_libraries) -module -avoid-version

kio_spop3_la_SOURCES = spop3.cc md5.c
kio_spop3_la_LIBADD  = $(LIB_KIO) $(LIBSSL)
kio_spop3_la_LDFLAGS = $(all_libraries) $(SSL_LDFLAGS) -module -avoid-version

noinst_HEADERS = pop3.h

kioconfig_DATA = pop3.desktop $(DO_SPOP3_DESKTOP)
kioconfigdir = $(kde_confdir)/protocols

check_PROGRAMS = linktest
linktest_SOURCES = $(kio_pop3_la_SOURCES) linktest.cpp
linktest.cpp:
	echo "int main() {};" > linktest.cpp
linktest_LDFLAGS  = $(all_libraries) -module -avoid-version
linktest_LDADD = $(kio_pop3_la_LIBADD)
linktest_LDADD = $(kio_spop3_la_LIBADD)

spop3.cc: $(srcdir)/pop3.cc
	echo "#define SPOP3" >spop3.cc
	cat $(srcdir)/pop3.cc >>spop3.cc

CLEANFILES = spop3.cc

