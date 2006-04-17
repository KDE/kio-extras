kde4_header()

project(kioslave-media)

set(libmediacommon_SRCS
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/medium.cpp
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/actionlistboxitem.cpp
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/notifieraction.cpp
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/notifierserviceaction.cpp
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/notifiernothingaction.cpp
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/notifieropenaction.cpp
    ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/notifiersettings.cpp
    )
    
#kde4_add_kcfg_files(libmediacommon_SRCS ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon/mediamanagersettings.kcfgc )

add_subdirectory( libmediacommon ) 
add_subdirectory( mediamanager ) 
add_subdirectory( medianotifier ) 
add_subdirectory( mounthelper ) 
add_subdirectory( kfile-plugin ) 
add_subdirectory( kcmodule ) 
add_subdirectory( mimetypes ) 
add_subdirectory( services ) 

include_directories( ${CMAKE_SOURCE_DIR}/kioslave/media/libmediacommon ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} )


########### next target ###############
set(kio_media_PART_SRCS ${libmediacommon_SRCS} 
    kio_media.cpp
    mediaimpl.cpp)


kde4_automoc(${kio_media_PART_SRCS})

kde4_add_dcop_skels(kio_media_PART_SRCS mediaimpl.h )

kde4_add_plugin(kio_media ${kio_media_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_media )

target_link_libraries(kio_media ${KDE4_KIO_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kio_media )


########### next target ###############


########### next target ###############

set(testmedia_SRCS testmedia.cpp )

kde4_automoc(${testmedia_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testmedia ${testmedia_SRCS})

target_link_libraries(testmedia  ${KDE4_KIO_LIBS} kiomedia  )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES media.protocol )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = libmediacommon . mediamanager medianotifier mounthelper \
#          kfile-plugin kcmodule mimetypes services
#
#INCLUDES = -I$(srcdir)/libmediacommon $(all_includes)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kio_media.la
#
#kio_media_la_SOURCES = dummy.cpp
#kio_media_la_LIBADD  = libkiomedia.la libmediacommon/libmediacommon.la $(LIB_KIO)
#kio_media_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -no-undefined
#
#dummy.cpp:
#	echo > dummy.cpp
#
#kde_services_DATA = media.protocol
#
#noinst_LTLIBRARIES = libkiomedia.la
#libkiomedia_la_SOURCES = kio_media.cpp mediaimpl.cpp mediaimpl.skel
#
#check_PROGRAMS = testmedia
#testmedia_SOURCES = testmedia.cpp
#testmedia_LDADD = libkiomedia.la $(LIB_KIO)
#testmedia_LDFLAGS = $(all_libraries)
#
### TODO in unsermake: TESTS = testmedia
#check: testmedia
#	./testmedia
#
#messages:
#	$(XGETTEXT) `find . -name "*.cc" -o -name "*.cpp" -o -name "*.h"` -o $(podir)/kio_media.pot
#
