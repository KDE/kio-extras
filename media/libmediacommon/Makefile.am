INCLUDES = $(all_includes)
METASOURCES = AUTO

noinst_LTLIBRARIES = libmediacommon.la
libmediacommon_la_SOURCES = medium.cpp

messages:
	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_media.pot

