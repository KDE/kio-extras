## $Id$
## Makefile.am of kdebase/kioslave/imap4

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_imap4

kio_imap4_SOURCES = response.cc imap4.cc md5.cc base64md5.cc
noinst_HEADERS = imap4.h md5.h base64md5.h

kioconfig_DATA = imap4.desktop
kioconfigdir = $(kde_confdir)/protocols

