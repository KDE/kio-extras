## Makefile.am of kdebase/kioslave/gzip

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_gzip

kio_gzip_SOURCES = gzip.cc
noinst_HEADERS = gzip.h

kioconfig_DATA = gzip.kdelnk
kioconfigdir = $(kde_confdir)/protocols

messages:

