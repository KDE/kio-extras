include_directories( ${CMAKE_SOURCE_DIR}/runtime/kioslave/media/libmediacommon/ )


########### next target ###############

set(kfile_media_PART_SRCS kfilemediaplugin.cpp ${libmediacommon_SRCS})

kde4_automoc(${kfile_media_PART_SRCS})

kde4_add_plugin(kfile_media ${kfile_media_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kfile_media )

target_link_libraries(kfile_media  ${KDE4_KIO_LIBS} )

install(TARGETS kfile_media  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kfile_media.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES   DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#kde_module_LTLIBRARIES = kfile_media.la
#
#kfile_media_la_SOURCES = kfilemediaplugin.cpp
#kfile_media_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module $(KDE_PLUGIN)
#kfile_media_la_LIBADD = $(LIB_KIO) ../libmediacommon/libmediacommon.la
#
#kdelnkdir = $(kde_servicesdir)
#kde_services_DATA = kfile_media.desktop
#
#INCLUDES = -I$(srcdir)/../libmediacommon $(all_includes)
#
#METASOURCES = AUTO
#noinst_HEADERS = kfilemediaplugin.h
