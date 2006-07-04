
project(kioslave-info)



########### next target ###############

set(kio_info_PART_SRCS info.cc )

kde4_automoc(${kio_info_PART_SRCS})

kde4_add_plugin(kio_info ${kio_info_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_info )

target_link_libraries(kio_info  ${KDE4_KIO_LIBS} )

install(TARGETS kio_info  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kde-info2html.conf kde-info2html  DESTINATION  ${DATA_INSTALL_DIR}/kio_info )
install( FILES info.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/info
#
#INCLUDES = $(all_includes)
#
#METASOURCES = AUTO
#
######## Files
#
#kde_module_LTLIBRARIES = kio_info.la
#
#kio_info_la_SOURCES = info.cc
#kio_info_la_LIBADD = $(LIB_KIO)
#kio_info_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = info.h
#
#kdelnk_DATA = info.protocol
#kdelnkdir = $(kde_servicesdir)
#
#kio_info_data_DATA = kde-info2html.conf
#kio_info_data_SCRIPTS = kde-info2html
#kio_info_datadir = $(kde_datadir)/kio_info
