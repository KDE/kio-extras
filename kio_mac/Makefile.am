## Makfile.am for kio_mac

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

kde_module_LTLIBRARIES = kio_mac.la

kio_mac_la_SOURCES = kio_mac.cpp
kio_mac_la_LIBADD = -lkio
kio_mac_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = kio_mac.h

kdelnk_DATA = mac.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO
KDE_ICON = AUTO
