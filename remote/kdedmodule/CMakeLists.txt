kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############

set(kded_remotedirnotify_PART_SRCS remotedirnotify.cpp remotedirnotifymodule.cpp )

kde4_automoc(${kded_remotedirnotify_PART_SRCS})

kde4_add_dcop_skels(kded_remotedirnotify_PART_SRCS remotedirnotify.h remotedirnotifymodule.h )

kde4_add_plugin(kded_remotedirnotify ${kded_remotedirnotify_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_remotedirnotify )

target_link_libraries(kded_remotedirnotify  ${KDE4_KDECORE_LIBS} DCOP kio)

install_targets(${LIB_INSTALL_DIR}/kde4 kded_remotedirnotify )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES remotedirnotify.desktop )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kded_remotedirnotify.la
#
#METASOURCES = AUTO
#INCLUDES = $(all_includes)
#
#kded_remotedirnotify_la_SOURCES = remotedirnotify.cpp remotedirnotify.skel remotedirnotifymodule.cpp remotedirnotifymodule.skel
#kded_remotedirnotify_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_remotedirnotify_la_LIBADD = $(LIB_KSYCOCA)
#
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = remotedirnotify.desktop
#
