


########### next target ###############

set(kded_homedirnotify_PART_SRCS homedirnotify.cpp homedirnotifymodule.cpp )

kde4_automoc(${kded_homedirnotify_PART_SRCS})

kde4_add_dcop_skels(kded_homedirnotify_PART_SRCS homedirnotify.h homedirnotifymodule.h )

kde4_add_plugin(kded_homedirnotify ${kded_homedirnotify_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_homedirnotify )

target_link_libraries(kded_homedirnotify  ${KDE4_KDECORE_LIBS} DCOP kio)

install_targets(${PLUGIN_INSTALL_DIR} kded_homedirnotify )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR}/kded FILES homedirnotify.desktop )




#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kded_homedirnotify.la
#
#METASOURCES = AUTO
#INCLUDES = $(all_includes)
#
#kded_homedirnotify_la_SOURCES = homedirnotify.cpp homedirnotify.skel homedirnotifymodule.cpp homedirnotifymodule.skel
#kded_homedirnotify_la_LDFLAGS = $(all_libraries) -module -avoid-version
#kded_homedirnotify_la_LIBADD = $(LIB_KSYCOCA)
#
#
#servicesdir = $(kde_servicesdir)/kded
#services_DATA = homedirnotify.desktop
#
