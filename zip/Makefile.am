## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES = $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kio_zip.la

kio_zip_la_SOURCES = zip.cc
kio_zip_la_LIBADD = $(LIB_KSYCOCA)
kio_zip_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = zip.h

kdelnk_DATA = zip.protocol
kdelnkdir = $(kde_servicesdir)

