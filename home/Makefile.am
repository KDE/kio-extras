SUBDIRS= . kdedmodule
# wizard

INCLUDES = $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kio_home.la

kio_home_la_SOURCES = dummy.cpp
kio_home_la_LIBADD  = libkiohome.la $(LIB_KIO)
kio_home_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = home.protocol

noinst_LTLIBRARIES = libkiohome.la
libkiohome_la_SOURCES = kio_home.cpp homeimpl.cpp

check_PROGRAMS = testhome
testhome_SOURCES = testhome.cpp
testhome_LDADD = libkiohome.la $(LIB_KIO)
testhome_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testhome
check: testhome
	./testhome


