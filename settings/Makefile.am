INCLUDES= $(all_includes)

####### Files

kde_module_LTLIBRARIES = kio_settings.la

kio_settings_la_SOURCES = kio_settings.cc
kio_settings_la_LIBADD  = $(LIB_KSYCOCA)
kio_settings_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = kio_settings.h

kdelnk_DATA = settings.protocol
kdelnkdir = $(kde_servicesdir)


SUBDIRS=. 

messages:
	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_settings.pot

