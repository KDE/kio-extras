## Makefile.am of kdebase/kioslave/info

INCLUDES = $(all_includes)

####### Files

lib_LTLIBRARIES = kio_info.la

kio_info_la_SOURCES = info.cc
kio_info_la_LIBADD = $(LIB_KIO)
kio_info_la_LDFLAGS = $(all_libraries) -module -avoid-version
noinst_HEADERS = info.h

check_PROGRAMS = test
test_SOURCES = $(kio_info_la_SOURCES) test.cpp
test.cpp:
	echo "int main() {};" > test.cpp
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = $(kio_info_la_LIBADD)

kioconfig_DATA = info.desktop
kioconfigdir = $(kde_confdir)/protocols
