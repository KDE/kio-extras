INCLUDES= $(all_includes)

####### Files

kde_module_LTLIBRARIES = kio_devices.la

kio_devices_la_SOURCES = kio_devices.cc
kio_devices_la_LIBADD  = $(LIB_KSYCOCA)
kio_devices_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = kio_devices.h

kdelnk_DATA = devices.protocol system.protocol
kdelnkdir = $(kde_servicesdir)


dataEntry_DATA = devices.desktop
dataEntrydir = $(kde_datadir)/konqsidebartng/entries

globalkickerentriesdir   = $(kde_datadir)/konqsidebartng/kicker_entries
globalkickerentries_DATA = devices.desktop

defaultInclude_DATA=mountwatcher.desktop
defaultIncludedir=$(kde_confdir)


SUBDIRS=. kdedmodule kfile-plugin mounthelper mimetypes

messages:
	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_devices.pot

