## $Id$
## Makefile.am of kdebase/kioslave/pop3

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_nntp

kio_nntp_SOURCES = nntp.cc
noinst_HEADERS = nntp.h

kioconfig_DATA = nntp.desktop
kioconfigdir = $(kde_confdir)/protocols

