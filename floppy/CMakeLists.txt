kde4_header()

project(kioslave-floppy)
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### next target ###############

set(kio_floppy_PART_SRCS kio_floppy.cpp program.cpp )

kde4_automoc(${kio_floppy_PART_SRCS})

kde4_add_plugin(kio_floppy ${kio_floppy_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_floppy )

target_link_libraries(kio_floppy  ${KDE4_KIO_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kio_floppy )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES floppy.protocol )

kde4_footer()



#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/floppy
#
#INCLUDES= $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_floppy.la
#
#kio_floppy_la_SOURCES = kio_floppy.cpp program.cpp
#kio_floppy_la_LIBADD = $(LIB_KIO)
#kio_floppy_la_LDFLAGS = -module $(KDE_PLUGIN)
#
#kdelnk_DATA = floppy.protocol
#kdelnkdir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_floppy.pot
