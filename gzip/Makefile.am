## Makefile.am of kdebase/kioslave/file

AM_CPPFLAGS = -DQT_NO_CAST_ASCII

INCLUDES = $(all_includes)

####### Files

kde_module_LTLIBRARIES = kio_gzip.la

kio_gzip_la_SOURCES = gzip.cc
kio_gzip_la_LIBADD = $(LIB_KIO)
kio_gzip_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = gzip.h

METASOURCES = AUTO

kdelnkdir = $(kde_servicesdir)
kdelnk_DATA = gzip.protocol
