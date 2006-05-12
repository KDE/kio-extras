kde4_header()

project(kioslave-finger)



########### next target ###############

set(kio_finger_PART_SRCS kio_finger.cpp )

kde4_automoc(${kio_finger_PART_SRCS})

kde4_add_plugin(kio_finger ${kio_finger_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_finger )

target_link_libraries(kio_finger  ${KDE4_KIO_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kio_finger )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kio_finger FILES kio_finger.pl kio_finger.css )
install_files( ${SERVICES_INSTALL_DIR} FILES finger.protocol )

kde4_footer()



#original Makefile.am contents follow:

### Makfile.am for kio_finger
### Edit from Makefile.am of kdebase/kioslave/man
#
#INCLUDES= $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_finger.la
#
#kio_finger_la_SOURCES = kio_finger.cpp 
#kio_finger_la_LIBADD = -lkio
#kio_finger_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = kio_finger.h 
#
#kdelnk_DATA = finger.protocol
#kdelnkdir = $(kde_servicesdir)
#
#kio_finger_data_DATA = kio_finger.pl kio_finger.css
#kio_finger_datadir = $(kde_datadir)/kio_finger
#EXTRA_DIST=$(kio_finger_data_DATA)   
#
#METASOURCES = AUTO
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_finger.pot
