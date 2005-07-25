kde_module_LTLIBRARIES = kfile_media.la

kfile_media_la_SOURCES = kfilemediaplugin.cpp
kfile_media_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module $(KDE_PLUGIN)
kfile_media_la_LIBADD = $(LIB_KIO) ../libmediacommon/libmediacommon.la

kdelnkdir = $(kde_servicesdir)
kde_services_DATA = kfile_media.desktop

INCLUDES = -I$(srcdir)/../libmediacommon $(all_includes)

METASOURCES = AUTO
noinst_HEADERS = kfilemediaplugin.h
