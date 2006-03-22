kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kded_systemdirnotify_PART_SRCS systemdirnotify.cpp systemdirnotifymodule.cpp )

kde4_automoc(${kded_systemdirnotify_PART_SRCS})

kde4_add_dcop_skels(kded_systemdirnotify_PART_SRCS systemdirnotify.h systemdirnotifymodule.h )

kde4_add_plugin(kded_systemdirnotify ${kded_systemdirnotify_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_systemdirnotify )

target_link_libraries(kded_systemdirnotify  ${KDE4_KDECORE_LIBS} kio DCOP)

install_targets(${LIB_INSTALL_DIR}/kde4 kded_systemdirnotify )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES systemdirnotify.desktop )

kde4_footer()



#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kded_systemdirnotify.la
#
#METASOURCES = AUTO
#INCLUDES = $(all_includes)
#
#kded_systemdirnotify_la_SOURCES = systemdirnotify.cpp systemdirnotify.skel systemdirnotifymodule.cpp systemdirnotifymodule.skel
#kded_systemdirnotify_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_systemdirnotify_la_LIBADD = $(LIB_KSYCOCA)
#
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = systemdirnotify.desktop
#
