## Makefile.am of kdebase/kioslave/info

INCLUDES = $(all_includes)

METASOURCES = AUTO

####### Files

lib_LTLIBRARIES = kio_info.la

kio_info_la_SOURCES = info.cc
kio_info_la_LIBADD = $(LIB_KIO)
kio_info_la_LDFLAGS = $(all_libraries) -module -avoid-version
noinst_HEADERS = info.h

check_PROGRAMS = test
test_SOURCES = $(kio_info_la_SOURCES) test.cc
test.cc:
	echo "int main() {};" > test.cc
test_LDFLAGS  = $(all_libraries) -module -avoid-version
test_LDADD = $(kio_info_la_LIBADD)

kdelnk_DATA = info.protocol
kdelnkdir = $(kde_servicesdir)

data_DATA = kde-info2html kde-info2html.conf
datadir = $(kde_datadir)/kio_info
