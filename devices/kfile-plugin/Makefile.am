kde_module_LTLIBRARIES = kfile_device.la

kfile_device_la_SOURCES = kfiledeviceplugin.cpp
kfile_device_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module $(KDE_PLUGIN)
kfile_device_la_LIBADD = $(LIB_KIO)

kdelnkdir = $(kde_servicesdir)
kde_services_DATA = kfile_device.desktop

INCLUDES = $(all_includes)

METASOURCES = AUTO
noinst_HEADERS = kfiledeviceplugin.h
