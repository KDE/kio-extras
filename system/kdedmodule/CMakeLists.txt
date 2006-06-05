


########### next target ###############

set(kded_systemdirnotify_PART_SRCS systemdirnotify.cpp systemdirnotifymodule.cpp )

kde4_automoc(${kded_systemdirnotify_PART_SRCS})

kde4_add_plugin(kded_systemdirnotify ${kded_systemdirnotify_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_systemdirnotify )

target_link_libraries(kded_systemdirnotify  ${KDE4_KIO_LIBS})

install_targets(${PLUGIN_INSTALL_DIR} kded_systemdirnotify )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES systemdirnotify.desktop )




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
