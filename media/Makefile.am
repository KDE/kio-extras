SUBDIRS = libmediacommon . kdedmodule mounthelper kfile-plugin mimetypes services

INCLUDES = $(all_includes) -I$(srcdir)/libmediacommon
METASOURCES = AUTO

kde_module_LTLIBRARIES = kio_media.la

kio_media_la_SOURCES = dummy.cpp
kio_media_la_LIBADD  = libkiomedia.la libmediacommon/libmediacommon.la $(LIB_KIO)
kio_media_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined

dummy.cpp:
	echo > dummy.cpp

kde_services_DATA = media.protocol

noinst_LTLIBRARIES = libkiomedia.la
libkiomedia_la_SOURCES = kio_media.cpp mediaimpl.cpp mediaimpl.skel

check_PROGRAMS = testmedia
testmedia_SOURCES = testmedia.cpp
testmedia_LDADD = libkiomedia.la $(LIB_KIO)
testmedia_LDFLAGS = $(all_libraries)

## TODO in unsermake: TESTS = testmedia
check: testmedia
	./testmedia

messages:
	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_media.pot

