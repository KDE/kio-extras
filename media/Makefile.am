SUBDIRS = libmediacommon . mediamanager medianotifier mounthelper \
          kfile-plugin kcmodule mimetypes services

INCLUDES = -I$(srcdir)/libmediacommon $(all_includes)
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


