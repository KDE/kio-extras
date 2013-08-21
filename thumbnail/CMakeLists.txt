find_package(OpenEXR)
set_package_properties(OpenEXR PROPERTIES DESCRIPTION "API for accessing OpenEXR formatted images"
                       URL "http://www.openexr.com"
                       TYPE OPTIONAL
                       PURPOSE "Provides support for OpenEXR formatted images in the thumbnail kioslave"
                      )

find_package(JPEG)
set_package_properties(JPEG PROPERTIES DESCRIPTION "JPEG image format support"
                       URL "http://www.ijg.org"
                       TYPE REQUIRED
                      )

find_package(Exiv2)
set_package_properties(Exiv2 PROPERTIES DESCRIPTION "A library to access image metadata"
                       URL "http://www.exiv2.org"
                       TYPE OPTIONAL
                       PURPOSE "Provides support for automatic rotation of JPEGs in the thumbnail kioslave"
                      )

macro_bool_to_01(EXIV2_FOUND HAVE_EXIV2)

include_directories(${JPEG_INCLUDE_DIR})
if(EXIV2_FOUND)
    include_directories(${EXIV2_INCLUDE_DIR})
endif(EXIV2_FOUND)

include_directories(${CMAKE_BINARY_DIR})

########### next target ###############

set(kio_thumbnail_PART_SRCS thumbnail.cpp imagefilter.cpp)

kde4_add_plugin(kio_thumbnail ${kio_thumbnail_PART_SRCS})

#target_link_libraries(kio_thumbnail ${KIO_LIBRARIES})
target_link_libraries(kio_thumbnail
 ${KCore_LIBRARIES}
 ${KDE4_KIO_LIBS}
 ${KWindowSystem_LIBRARIES}
 ${KCoreAddons_LIBRARIES}
 plasma
 ${Qt5Script_LIBRARIES}
 ${KDE4Support_LIBRARIES}
 ${Solid_LIBRARIES}
 ${KIconThemes_LIBRARIES}
 kdeclarative
 KF5::KI18n
)

install(TARGETS kio_thumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(imagethumbnail_PART_SRCS imagecreator.cpp)

kde4_add_plugin(imagethumbnail ${imagethumbnail_PART_SRCS})

target_link_libraries(imagethumbnail
 ${KCore_LIBRARIES}
 ${KDE4_KIO_LIBS}
 ${KWindowSystem_LIBRARIES}
 ${KCoreAddons_LIBRARIES}
 plasma
 ${Qt5Script_LIBRARIES}
 ${KDE4Support_LIBRARIES}
 ${Solid_LIBRARIES}
 kdeclarative
 KF5::KI18n
)

install(TARGETS imagethumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

# install( FILES directorythumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

########### next target ###############

# set(jpegthumbnail_PART_SRCS jpegcreator.cpp)
# kde4_add_kcfg_files(jpegthumbnail_PART_SRCS jpegcreatorsettings.kcfgc)
# kde4_add_plugin(jpegthumbnail ${jpegthumbnail_PART_SRCS})
# 
# target_link_libraries(jpegthumbnail ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${JPEG_LIBRARIES} ${KIO_LIBRARIES})
# 
# if (HAVE_EXIV2)
#     set_source_files_properties(jpegcreator.cpp PROPERTIES
#                                 COMPILE_FLAGS "-DHAVE_EXIV2 ${KDE4_ENABLE_EXCEPTIONS}")
#     include_directories(${EXIV1_INCLUDE_DIR})
#     target_link_libraries(jpegthumbnail ${EXIV2_LIBRARIES})
# endif (HAVE_EXIV2)
# 
# install(FILES jpegcreatorsettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
# install(TARGETS jpegthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

# ########### next target ###############
# 
# set(svgthumbnail_PART_SRCS svgcreator.cpp)
# 
# kde4_add_plugin(svgthumbnail ${svgthumbnail_PART_SRCS})
# 
# target_link_libraries(svgthumbnail ${KIO_LIBRARIES})
# 
# install(TARGETS svgthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
# 
# ########### next target ###############
# 
# set(textthumbnail_PART_SRCS textcreator.cpp)
# 
# kde4_add_plugin(textthumbnail ${textthumbnail_PART_SRCS})
# 
# target_link_libraries(textthumbnail ${KIO_LIBRARIES})
# 
# install(TARGETS textthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
# 
# ########### next target ###############
# 
# if(NOT WINCE)
# set(htmlthumbnail_PART_SRCS htmlcreator.cpp)
# 
# kde4_add_plugin(htmlthumbnail ${htmlthumbnail_PART_SRCS})
# 
# target_link_libraries(htmlthumbnail ${KDE4_KDEWEBKIT_LIBS})
# 
# install(TARGETS htmlthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
# endif(NOT WINCE)
# 
# ########### next target ###############
# 
# if(NOT WIN32)
# set(djvuthumbnail_PART_SRCS djvucreator.cpp)
# 
# kde4_add_plugin(djvuthumbnail ${djvuthumbnail_PART_SRCS})
# 
# target_link_libraries(djvuthumbnail ${KIO_LIBRARIES})
# 
# install(TARGETS djvuthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
# endif(NOT WIN32)
# 
# ########### next target ###############
# 
# if(OPENEXR_FOUND)
# 
#    include_directories( ${OPENEXR_INCLUDE_DIR})
# 
#    set(exrthumbnail_PART_SRCS exrcreator.cpp)
# 
#    kde4_add_plugin(exrthumbnail ${exrthumbnail_PART_SRCS})
# 
#    target_link_libraries(exrthumbnail ${KIO_LIBRARIES} ${OPENEXR_LIBRARIES})
# 
#    # OpenEXR headers use exceptions; at least clang refuses to build the target
#    # when exceptions are not enabled.
#    set_target_properties(exrthumbnail PROPERTIES COMPILE_FLAGS "${KDE4_ENABLE_EXCEPTIONS}")
# 
#    install(TARGETS exrthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
#    install( FILES exrthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
# 
# endif(OPENEXR_FOUND)
# 
# ########### next target ###############
# 
# if(X11_Xcursor_FOUND)
# 
#    set(cursorthumbnail_PART_SRCS cursorcreator.cpp)
# 
#    kde4_add_plugin(cursorthumbnail ${cursorthumbnail_PART_SRCS})
# 
#    target_link_libraries(cursorthumbnail ${X11_Xcursor_LIB} ${KIO_LIBRARIES})
# 
#    install(TARGETS cursorthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
#    install( FILES cursorthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
# 
# endif(X11_Xcursor_FOUND)
# 
# ########### next target ###############
# 
# set ( windowsexethumbnail_SRCS windowsexecreator.cpp icoutils.cpp)
# kde4_add_plugin ( windowsexethumbnail ${windowsexethumbnail_SRCS} )
# target_link_libraries ( windowsexethumbnail ${KIO_LIBRARIES} )
# install ( TARGETS windowsexethumbnail DESTINATION ${PLUGIN_INSTALL_DIR} )
# install ( FILES windowsexethumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
# 
# set ( windowsimagethumbnail_SRCS windowsimagecreator.cpp icoutils.cpp)
# kde4_add_plugin ( windowsimagethumbnail ${windowsimagethumbnail_SRCS} )
# target_link_libraries ( windowsimagethumbnail ${KIO_LIBRARIES} )
# install ( TARGETS windowsimagethumbnail DESTINATION ${PLUGIN_INSTALL_DIR} )
# install ( FILES windowsimagethumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR} )
# 
# ########### next target ###############
# 
# set(comicbookthumbnail_SRCS comiccreator.cpp)
# kde4_add_plugin(comicbookthumbnail ${comicbookthumbnail_SRCS})
# target_link_libraries(comicbookthumbnail ${KIO_LIBRARIES})
# if (UNIX)
#    target_link_libraries(comicbookthumbnail ${KDE4_KPTY_LIBRARY})
# endif (UNIX)
# install(TARGETS comicbookthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

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
    comicbookthumbnail.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})
