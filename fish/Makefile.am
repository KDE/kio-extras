kde_module_LTLIBRARIES = kio_fish.la

INCLUDES = $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kio_fish_la_SOURCES = fish.cpp
kio_fish_la_LIBADD = $(LIB_KSYCOCA) #$(LIBUTIL) 
kio_fish_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = fishcode.h fish.h

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO FAQ fish.pl

DISTCLEANFILES = fishcode.h

kdelnk_DATA = fish.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

fish.lo: fishcode.h

fishcode.h: fish.pl
	SUM=`$(MD5SUM) $(srcdir)/fish.pl | cut -d ' ' $(MD5SUM_CUT)`; \
	echo '#define CHECKSUM "'$$SUM'"' > $@; \
	echo 'static const char *fishCode(' >> $@; \
	sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^[ 	]*/"/;/^"# /d;s/[ 	]*$$/\\n"/;/^"\\n"$$/d;s/{CHECKSUM}/'$$SUM'/;' $(srcdir)/fish.pl >> $@; \
	echo ');' >> $@;




