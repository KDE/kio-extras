## $Id$
## Makefile.am of kdebase/kioslave/thumbnail

INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kio_thumbnail.la

kio_thumbnail_la_SOURCES = thumbnail.cpp imagecreator.cpp textcreator.cpp htmlcreator.cpp
kio_thumbnail_la_LIBADD = $(LIB_KIO) $(LIB_KHTML)
kio_thumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = thumbnail.h imagecreator.h textcreator.h htmlcreator.h
include_HEADERS = thumbcreator.h

kdelnk_DATA = thumbnail.protocol
kdelnkdir = $(kde_servicesdir)

