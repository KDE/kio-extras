
project(kioslave-media)

configure_file(config-media.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-media.h)

add_definitions (-DQT3_SUPPORT)

set(libmediacommon_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/medium.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/actionlistboxitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifieraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifierserviceaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifiernothingaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifieropenaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon/notifiersettings.cpp
    )
    
add_subdirectory( libmediacommon ) 
add_subdirectory( mediamanager ) 
add_subdirectory( medianotifier ) 
add_subdirectory( mounthelper ) 
# this plugin is deprecated according to Kévin Ottens
#add_subdirectory( kfile-plugin ) 
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


target_link_libraries(kio_media ${KDE4_KIO_LIBS} )

install(TARGETS kio_media  DESTINATION ${PLUGIN_INSTALL_DIR} )



########### next target ###############
if(KDE4_BUILD_TESTS)
set(testmedia_SRCS testmedia.cpp )

kde4_automoc(${testmedia_SRCS})


kde4_add_executable(testmedia ${testmedia_SRCS} ${libmediacommon_SRCS} )

target_link_libraries(testmedia  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES media.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES org.kde.MediaManager.xml DESTINATION ${DBUS_INTERFACES_DIR} )

