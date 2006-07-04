
project(kioslave-media)

set(libmediacommon_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/medium.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/actionlistboxitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifieraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifierserviceaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifiernothingaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifieropenaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifiersettings.cpp
    )
    
#kde4_add_kcfg_files(libmediacommon_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/mediamanagersettings.kcfgc )

add_subdirectory( libmediacommon ) 
add_subdirectory( mediamanager ) 
add_subdirectory( medianotifier ) 
add_subdirectory( mounthelper ) 
add_subdirectory( kfile-plugin ) 
add_subdirectory( kcmodule ) 
add_subdirectory( mimetypes ) 
add_subdirectory( services ) 

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon )


########### next target ###############
set(kio_media_PART_SRCS ${libmediacommon_SRCS} 
    kio_media.cpp
    mediaimpl.cpp
    mediamanageriface.cpp)


kde4_automoc(${kio_media_PART_SRCS})

kde4_add_plugin(kio_media ${kio_media_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_media )

target_link_libraries(kio_media ${KDE4_KIO_LIBS} )

install(TARGETS kio_media  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############


########### next target ###############

set(testmedia_SRCS testmedia.cpp )

kde4_automoc(${testmedia_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(testmedia ${testmedia_SRCS} ${libmediacommon_SRCS} )

target_link_libraries(testmedia  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES media.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




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
