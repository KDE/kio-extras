INCLUDES= $(all_includes)

LDFLAGS =$(all_libraries)

####### Files

kde_module_LTLIBRARIES = kio_devices.la

kio_devices_la_SOURCES = kio_devices.cc
kio_devices_la_LIBADD  = $(LIB_KSYCOCA)
kio_devices_la_LDFLAGS = -module $(KDE_PLUGIN)

noinst_HEADERS = kio_devices.h

kdelnk_DATA = devices.protocol
kdelnkdir = $(kde_servicesdir)


SUBDIRS=. kdedmodule mimetypes
