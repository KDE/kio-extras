## $Id$
## Makefile.am of kdebase/kioslave/tar

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_tar

kio_tar_SOURCES = tar.cc
noinst_HEADERS = tar.h

kioconfig_DATA = tar.desktop
kioconfigdir = $(kde_confdir)/protocols

messages:

