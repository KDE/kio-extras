## $Id$
## Makefile.am of kdebase/kioslave/gopher

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_gopher

kio_gopher_SOURCES = gopher.cc
noinst_HEADERS = gopher.h

kioconfig_DATA = gopher.desktop
kioconfigdir = $(kde_confdir)/protocols

messages:

