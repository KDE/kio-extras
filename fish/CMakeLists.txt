kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kio_fish_PART_SRCS fish.cpp )

kde4_automoc(${kio_fish_PART_SRCS})

kde4_add_plugin(kio_fish ${kio_fish_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_fish )

target_link_libraries(kio_fish  ${QT_AND_KDECORE_LIBS} )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_fish )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES fish.protocol )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kio_fish.la
#
#INCLUDES = $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#kio_fish_la_SOURCES = fish.cpp
#kio_fish_la_LIBADD = $(LIB_KSYCOCA) #$(LIBUTIL) 
#kio_fish_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = fishcode.h fish.h
#
#EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO FAQ fish.pl
#
#DISTCLEANFILES = fishcode.h
#
#kdelnk_DATA = fish.protocol
#kdelnkdir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#
#fish.lo: fishcode.h
#
#fishcode.h: fish.pl
#	SUM=`$(MD5SUM) $(srcdir)/fish.pl | cut -d ' ' $(MD5SUM_CUT)`; \
#	echo '#define CHECKSUM "'$$SUM'"' > $@; \
#	echo 'static const char *fishCode(' >> $@; \
#	sed -e 's/\\/\\\\/g;s/"/\\"/g;s/^[ 	]*/"/;/^"# /d;s/[ 	]*$$/\\n"/;/^"\\n"$$/d;s/{CHECKSUM}/'$$SUM'/;' $(srcdir)/fish.pl >> $@; \
#	echo ');' >> $@;
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_fish.pot
#
#
#
