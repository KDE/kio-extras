


########### next target ###############

set(kshorturifilter_PART_SRCS kshorturifilter.cpp )

kde4_automoc(${kshorturifilter_PART_SRCS})

kde4_add_plugin(kshorturifilter WITH_PREFIX ${kshorturifilter_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kshorturifilter )

target_link_libraries(kshorturifilter  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS kshorturifilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kshorturifilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kshorturifilterrc  DESTINATION  ${CONFIG_INSTALL_DIR} )




#original Makefile.am contents follow:

### Makefile.am of kdebase/kcontrol/ebrowsing/plugins/shorturi
#
#AM_CPPFLAGS   =  $(all_includes) -DQT_NO_CAST_ASCII
#AM_LDFLAGS   = $(all_libraries)
#
#kde_module_LTLIBRARIES = libkshorturifilter.la
#
#libkshorturifilter_la_LDFLAGS = -module $(KDE_PLUGIN)
#libkshorturifilter_la_LIBADD = $(LIB_KSYCOCA) $(LIB_KDEUI)
#
#libkshorturifilter_la_SOURCES = kshorturifilter.cpp kshorturifilter.skel
#
#service_DATA = kshorturifilter.desktop
#servicedir = $(kde_servicesdir)
#
#rc_DATA = kshorturifilterrc
#rcdir = $(kde_confdir)
#
#METASOURCES = AUTO
