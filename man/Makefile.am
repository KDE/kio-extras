## Makefile.am of kdebase/kioslave/man

INCLUDES = $(all_includes)

####### Files

lib_LTLIBRARIES = kio_man.la

kio_man_la_SOURCES = man.cc manslave.cc
kio_man_la_LIBADD = $(LIB_KIO)
kio_man_la_LDFLAGS = $(all_libraries) -module -avoid-version
noinst_HEADERS = man.h manslave.h

check_PROGRAMS = test
test_SOURCES = $(kio_man_la_SOURCES) test.cc
test.cc:
	echo "int main() {};" > test.cc
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = $(kio_man_la_LIBADD)

kioconfig_DATA = man.desktop
kioconfigdir = $(kde_confdir)/protocols
