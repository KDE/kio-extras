## $Id$
## Makefile.am of kdebase/kioslave/thumbnail

INCLUDES = $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

kde_module_LTLIBRARIES = kio_thumbnail.la imagethumbnail.la \
	textthumbnail.la htmlthumbnail.la gsthumbnail.la picturethumbnail.la

kio_thumbnail_la_SOURCES = thumbnail.cpp
kio_thumbnail_la_LIBADD = $(LIB_KIO)
kio_thumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

imagethumbnail_la_SOURCES = imagecreator.cpp
imagethumbnail_la_LIBADD = $(LIB_KIO) # for kimageio
imagethumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

textthumbnail_la_SOURCES = textcreator.cpp
textthumbnail_la_LIBADD = $(LIB_KDECORE)
textthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -lkdefx

htmlthumbnail_la_SOURCES = htmlcreator.cpp
htmlthumbnail_la_LIBADD = $(LIB_KHTML)
htmlthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

gsthumbnail_la_SOURCES = gscreator.cpp
gsthumbnail_la_LIBADD = $(LIB_KDECORE)
gsthumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

picturethumbnail_la_SOURCES = picturecreator.cpp
picturethumbnail_la_LIBADD = $(LIB_KDECORE)
picturethumbnail_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

noinst_HEADERS = thumbnail.h imagecreator.h textcreator.h htmlcreator.h \
	gscreator.h picturecreator.h

kdelnk_DATA = thumbnail.protocol
kdelnkdir = $(kde_servicesdir)

servicetypes_DATA = thumbcreator.desktop
servicetypesdir = $(kde_servicetypesdir)

services_DATA = imagethumbnail.desktop textthumbnail.desktop \
                htmlthumbnail.desktop gsthumbnail.desktop picturethumbnail.desktop
servicesdir = $(kde_servicesdir)

