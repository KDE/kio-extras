kde4_header()

project(kioslave-settings)
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kio_settings_PART_SRCS kio_settings.cc )

kde4_automoc(${kio_settings_PART_SRCS})

kde4_add_plugin(kio_settings ${kio_settings_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_settings )

target_link_libraries(kio_settings  ${KDE4_KIO_LIBS} )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_settings )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES settings.protocol programs.protocol applications.protocol )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES= $(all_includes)
#KDE_CXXFLAGS = -UQT3_SUPPORT
######## Files
#
#kde_module_LTLIBRARIES = kio_settings.la
#
#kio_settings_la_SOURCES = kio_settings.cc
#kio_settings_la_LIBADD  = $(LIB_KSYCOCA)
#kio_settings_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = kio_settings.h
#
#kdelnk_DATA = settings.protocol programs.protocol applications.protocol
#kdelnkdir = $(kde_servicesdir)
#
#
#SUBDIRS=. 
#
#messages:
#	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_settings.pot
#
