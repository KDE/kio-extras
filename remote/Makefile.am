SUBDIRS= .
#kdedmodule wizard

INCLUDES = $(all_includes)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kio_remote.la

kio_remote_la_SOURCES = dummy.cpp
kio_remote_la_LIBADD  = libkioremote.la $(LIB_KIO)
kio_remote_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = remote.protocol

noinst_LTLIBRARIES = libkioremote.la
libkioremote_la_SOURCES = kio_remote.cpp

check_PROGRAMS = testremote
testremote_SOURCES = testremote.cpp
testremote_LDADD = libkioremote.la $(LIB_KIO)
testremote_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testremote
check: testremote
	./testremote

messages:
	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_remote.pot

