macro_optional_find_package(OpenEXR)
macro_log_feature(OPENEXR_FOUND "OpenEXR" "API for accessing OpenEXR formatted images" "http://www.openexr.com" FALSE "" "Provides support for OpenEXR formatted images in the thumbnail kioslave")

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})

########### next target ###############

set(kio_thumbnail_PART_SRCS thumbnail.cpp imagefilter.cpp)

kde4_add_plugin(kio_thumbnail ${kio_thumbnail_PART_SRCS})

target_link_libraries(kio_thumbnail ${KDE4_KIO_LIBS})

install(TARGETS kio_thumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(imagethumbnail_PART_SRCS imagecreator.cpp)

kde4_add_plugin(imagethumbnail ${imagethumbnail_PART_SRCS})

target_link_libraries(imagethumbnail ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS imagethumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

install( FILES directorythumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

########### next target ###############

set(jpegthumbnail_PART_SRCS jpegcreator.cpp)

kde4_add_plugin(jpegthumbnail ${jpegthumbnail_PART_SRCS})

target_link_libraries(jpegthumbnail ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${JPEG_LIBRARIES} ${KDE4_KIO_LIBS})

install(TARGETS jpegthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(svgthumbnail_PART_SRCS svgcreator.cpp)

kde4_add_plugin(svgthumbnail ${svgthumbnail_PART_SRCS})

target_link_libraries(svgthumbnail ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS}) # needs kdeui for ksvgrenderer

install(TARGETS svgthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(textthumbnail_PART_SRCS textcreator.cpp)

kde4_add_plugin(textthumbnail ${textthumbnail_PART_SRCS})

target_link_libraries(textthumbnail ${KDE4_KDECORE_LIBS} ${QT_QTGUI_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS textthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(htmlthumbnail_PART_SRCS htmlcreator.cpp)

kde4_add_plugin(htmlthumbnail ${htmlthumbnail_PART_SRCS})

target_link_libraries(htmlthumbnail ${KDE4_KHTML_LIBS} ${QT_QTGUI_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS htmlthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

if(NOT WIN32)
set(djvuthumbnail_PART_SRCS djvucreator.cpp)

kde4_add_plugin(djvuthumbnail ${djvuthumbnail_PART_SRCS})

target_link_libraries(djvuthumbnail ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${KDE4_KIO_LIBS})

install(TARGETS djvuthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
endif(NOT WIN32)

########### next target ###############

if(OPENEXR_FOUND)

   include_directories( ${OPENEXR_INCLUDE_DIR})

   set(exrthumbnail_PART_SRCS exrcreator.cpp)

   kde4_add_plugin(exrthumbnail ${exrthumbnail_PART_SRCS})

   target_link_libraries(exrthumbnail ${KDE4_KIO_LIBS} ${OPENEXR_LIBRARIES})

   install(TARGETS exrthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
   install( FILES exrthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif(OPENEXR_FOUND)

########### next target ###############

if(X11_Xcursor_FOUND)

   set(cursorthumbnail_PART_SRCS cursorcreator.cpp)

   kde4_add_plugin(cursorthumbnail ${cursorthumbnail_PART_SRCS})

   target_link_libraries(cursorthumbnail ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${X11_Xcursor_LIB} ${KDE4_KIO_LIBS})

   install(TARGETS cursorthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
   install( FILES cursorthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif(X11_Xcursor_FOUND)

########### install files ###############

install(FILES thumbcreator.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES
    thumbnail.protocol
    svgthumbnail.desktop
    imagethumbnail.desktop
    jpegthumbnail.desktop
    textthumbnail.desktop
    htmlthumbnail.desktop
    djvuthumbnail.desktop
    desktopthumbnail.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
install(FILES thumbnailfont_7x4.png DESTINATION ${DATA_INSTALL_DIR}/kio_thumbnail/pics)
