## $Id$
## Makefile.am of kdebase/kioslave/pop3

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_pop3

kio_pop3_SOURCES = pop3.cc
noinst_HEADERS = pop3.h

kioconfig_DATA = pop3.desktop
kioconfigdir = $(kde_confdir)/protocols

