kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kfile_media_PART_SRCS kfilemediaplugin.cpp )

kde4_automoc(${kfile_media_PART_SRCS})

kde4_add_plugin(kfile_media ${kfile_media_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kfile_media )

target_link_libraries(kfile_media  ${QT_AND_KDECORE_LIBS} kio mediacommon )

install_targets(${LIB_INSTALL_DIR}/kde4 kfile_media )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES kfile_media.desktop )
install_files( ${SERVICES_INSTALL_DIR} FILES  )

kde4_footer()



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
