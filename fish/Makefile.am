kde_module_LTLIBRARIES = kio_fish.la

INCLUDES = $(all_includes)

kio_fish_la_SOURCES = fish.cpp
kio_fish_la_LIBADD = $(LIB_KIO) $(LIB_UTIL)
kio_fish_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO FAQ fish.pl

DISTCLEANFILES = fishcode.h

kdelnk_DATA = fish.protocol
kdelnkdir = $(kde_servicesdir)

fish.cpp: fish.h fishcode.h

fishcode.h: fish.pl
	SUM=`md5sum -t $< | cut -d ' ' -f 1`; \
	echo '#define CHECKSUM "'$$SUM'"' > $@; \
	echo 'static const char *fishCode(' >> $@; \
	sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^[ 	]*/"/;/^"# /d;s/[ 	]*$$/\\n"/;/^"\\n"$$/d;s/{CHECKSUM}/'$$SUM'/;' $< >> $@; \
	echo ');' >> $@;



