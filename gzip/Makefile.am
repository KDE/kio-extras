## Makefile.am of kdebase/kioslave/gzip

INCLUDES=	$(all_includes)
LDFLAGS =       $(all_libraries) $(KDE_RPATH)
LDADD	=	$(LIB_KIO)

####### Files

bin_PROGRAMS = kio_gzip

kio_gzip_SOURCES = gzip.cc
noinst_HEADERS = gzip.h

kdelnk_DATA = gzip.protocol bzip.protocol bzip2.protocol
kdelnkdir = $(kde_servicesdir)
