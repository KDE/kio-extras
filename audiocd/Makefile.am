INCLUDES = $(all_includes) $(CDPARANOIA_INCS)

kde_module_LTLIBRARIES = kio_audiocd.la

kio_audiocd_la_SOURCES = audiocd.cpp cddb.cpp

kio_audiocd_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(CDPARANOIA_RPATH) \
  -module $(KDE_PLUGIN)

kio_audiocd_la_LIBADD = $(LIB_KIO) $(CDPARANOIA_LIBS)

noinst_HEADERS = audiocd.h cddb.h

protocoldir = $(kde_servicesdir)
protocol_DATA = audiocd.protocol
