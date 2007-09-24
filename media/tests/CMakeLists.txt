set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
set(libmediacommon_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/medium.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/actionlistboxitem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/notifieraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/notifierserviceaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/notifiernothingaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/notifieropenaction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../libmediacommon/notifiersettings.cpp
    )
    
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/libmediacommon )


########### next target ###############
set(testmedia_SRCS testmedia.cpp )



kde4_add_executable(testmedia TEST ${testmedia_SRCS} ${libmediacommon_SRCS} )

target_link_libraries(testmedia  ${KDE4_KIO_LIBS} )


