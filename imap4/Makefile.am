## $Id$
## Makefile.am of kdebase/kioslave/imap4

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_imap4

kio_imap4_SOURCES = imap4.cc
noinst_HEADERS = imap4.h

kioconfig_DATA = imap4.kdelnk
kioconfigdir = $(kde_confdir)/protocols

messages:

