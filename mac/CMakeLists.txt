kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kio_mac_PART_SRCS kio_mac.cpp )

kde4_automoc(${kio_mac_PART_SRCS})

kde4_add_plugin(kio_mac ${kio_mac_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_mac )

target_link_libraries(kio_mac  ${LIB_KIO} )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_mac )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES mac.protocol )

kde4_install_icons( ${ICON_INSTALL_DIR} crystalsvg )

kde4_footer()



#original Makefile.am contents follow:

### Makfile.am for kio_mac
#
#INCLUDES= $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_mac.la
#
#kio_mac_la_SOURCES = kio_mac.cpp
#kio_mac_la_LIBADD = -lkio
#kio_mac_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = kio_mac.h
#
#kdelnk_DATA = mac.protocol
#kdelnkdir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#KDE_ICON = AUTO
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_mac.pot
