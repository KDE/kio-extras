INCLUDES = $(all_includes) $(CDPARANOIA_INCS)

lib_LTLIBRARIES = kio_audiocd.la

kio_audiocd_la_SOURCES = audiocd.cpp

kio_audiocd_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(CDPARANOIA_RPATH) \
  -module -avoid-version -no-undefined

kio_audiocd_la_LIBADD = $(LIB_KIO) $(CDPARANOIA_LIBS)

noinst_HEADERS = audiocd.h

protocoldir = $(kde_servicesdir)
protocol_DATA = audiocd.protocol
