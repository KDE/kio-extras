add_definitions(-DTRANSLATION_DOMAIN=\"kio_thumbnail\")

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
endif()

include_directories(${CMAKE_BINARY_DIR})

########### next target ###############

set(kio_thumbnail_PART_SRCS thumbnail.cpp imagefilter.cpp)

add_library(kio_thumbnail MODULE ${kio_thumbnail_PART_SRCS})

target_link_libraries(kio_thumbnail
	KF5::CoreAddons
	KF5::KIOCore
	KF5::KIOWidgets
	KF5::Solid
	KF5::IconThemes
 	KF5::I18n
	KF5::KDELibs4Support
)

set_target_properties(kio_thumbnail PROPERTIES OUTPUT_NAME "thumbnail")

install(TARGETS kio_thumbnail DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

########### next target ###############

set(imagethumbnail_PART_SRCS imagecreator.cpp)

add_library(imagethumbnail MODULE ${imagethumbnail_PART_SRCS})

target_link_libraries(imagethumbnail
    KF5::KIOWidgets
)

install(TARGETS imagethumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

install( FILES directorythumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

########### next target ###############

set(jpegthumbnail_PART_SRCS jpegcreator.cpp)
kconfig_add_kcfg_files(jpegthumbnail_PART_SRCS jpegcreatorsettings5.kcfgc)
add_library(jpegthumbnail MODULE ${jpegthumbnail_PART_SRCS})

target_link_libraries(jpegthumbnail
	Qt5::Core
	Qt5::Gui
	KF5::KIOCore
	KF5::KIOWidgets
	${JPEG_LIBRARIES}
	KF5::I18n
	KF5::ConfigCore
	KF5::ConfigGui
)


if (HAVE_EXIV2)
    set_source_files_properties(jpegcreator.cpp PROPERTIES
                                COMPILE_DEFINITIONS "HAVE_EXIV2")
    kde_source_files_enable_exceptions(jpegcreator.cpp)
    include_directories(${EXIV2_INCLUDE_DIR})
    target_link_libraries(jpegthumbnail ${EXIV2_LIBRARIES})
endif ()

install(FILES jpegcreatorsettings5.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install(TARGETS jpegthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(svgthumbnail_PART_SRCS svgcreator.cpp)

add_library(svgthumbnail MODULE ${svgthumbnail_PART_SRCS})

target_link_libraries(svgthumbnail
	Qt5::Gui
	Qt5::Svg
	KF5::KIOCore
	KF5::KIOWidgets
)

install(TARGETS svgthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

set(textthumbnail_PART_SRCS textcreator.cpp)

add_library(textthumbnail MODULE ${textthumbnail_PART_SRCS})

target_link_libraries(textthumbnail
    Qt5::Gui
    KF5::KIOWidgets
)

install(TARGETS textthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

############ next target ###############

find_package(Qt5WebEngineWidgets 5.7.0 QUIET)

if (Qt5WebEngineWidgets_FOUND)

    set(htmlthumbnail_PART_SRCS htmlcreator.cpp)

    add_library(htmlthumbnail MODULE ${htmlthumbnail_PART_SRCS})

    target_link_libraries(htmlthumbnail Qt5::Core Qt5::WebEngineWidgets KF5::KIOWidgets)

    install(TARGETS htmlthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

endif()
########### next target ###############

if(NOT WIN32)
set(djvuthumbnail_PART_SRCS djvucreator.cpp)

add_library(djvuthumbnail MODULE ${djvuthumbnail_PART_SRCS})

target_link_libraries(djvuthumbnail KF5::KIOWidgets)

install(TARGETS djvuthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES djvuthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif()

########### next target ###############

if(OpenEXR_FOUND)

    include_directories( ${OpenEXR_INCLUDE_DIRS})

    set(exrthumbnail_PART_SRCS exrcreator.cpp)

    add_library(exrthumbnail MODULE ${exrthumbnail_PART_SRCS})

    target_link_libraries(exrthumbnail
            KF5::KIOCore
            KF5::KIOWidgets
            ${OpenEXR_LIBRARIES}
        )

    # OpenEXR headers use exceptions; at least clang refuses to build the target
    # when exceptions are not enabled.
    kde_source_files_enable_exceptions(exrcreator.cpp)

    install(TARGETS exrthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
    install( FILES exrthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})

endif()

########### next target ###############

# if(X11_Xcursor_FOUND)
#
#    set(cursorthumbnail_PART_SRCS cursorcreator.cpp)
#
#    add_library(cursorthumbnail MODULE ${cursorthumbnail_PART_SRCS})
#
#    target_link_libraries(cursorthumbnail ${X11_Xcursor_LIB} ${KIO_LIBRARIES})
#
#    install(TARGETS cursorthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})
#    install( FILES cursorthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
#
# endif()
#

########### next target ###############

set ( windowsexethumbnail_SRCS windowsexecreator.cpp icoutils_common.cpp)
set ( windowsimagethumbnail_SRCS windowsimagecreator.cpp icoutils_common.cpp)

if(WIN32)
    set ( windowsexethumbnail_SRCS ${windowsexethumbnail_SRCS} icoutils_win.cpp )
    set ( windowsimagethumbnail_SRCS ${windowsimagethumbnail_SRCS} icoutils_win.cpp )
else()
    set ( windowsexethumbnail_SRCS ${windowsexethumbnail_SRCS} icoutils_wrestool.cpp )
    set ( windowsimagethumbnail_SRCS ${windowsimagethumbnail_SRCS} icoutils_wrestool.cpp )
endif()

add_library(windowsexethumbnail MODULE ${windowsexethumbnail_SRCS} )
target_link_libraries ( windowsexethumbnail KF5::KIOWidgets )
install ( TARGETS windowsexethumbnail DESTINATION ${PLUGIN_INSTALL_DIR} )
install ( FILES windowsexethumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

add_library(windowsimagethumbnail MODULE ${windowsimagethumbnail_SRCS} )
target_link_libraries ( windowsimagethumbnail KF5::KIOWidgets )
install ( TARGETS windowsimagethumbnail DESTINATION ${PLUGIN_INSTALL_DIR} )
install ( FILES windowsimagethumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR} )

########### next target ###############

set(comicbookthumbnail_SRCS comiccreator.cpp)
add_library(comicbookthumbnail MODULE ${comicbookthumbnail_SRCS})
target_link_libraries(comicbookthumbnail
    Qt5::Gui
    KF5::Archive
    KF5::KIOWidgets
)
if (UNIX)
    target_link_libraries(comicbookthumbnail KF5::Pty)
endif ()
install(TARGETS comicbookthumbnail DESTINATION ${PLUGIN_INSTALL_DIR})

# ########### next target ###############

set(kritathumbnail_SRCS kritacreator.cpp)
add_library(kritathumbnail MODULE ${kritathumbnail_SRCS})

target_link_libraries(kritathumbnail
    KF5::KIOWidgets
    KF5::Archive
    Qt5::Gui
)

install(TARGETS kritathumbnail  DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install(FILES thumbcreator.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES
    thumbnail.protocol
    svgthumbnail.desktop
    imagethumbnail.desktop
    jpegthumbnail.desktop
    textthumbnail.desktop
#    desktopthumbnail.desktop
    comicbookthumbnail.desktop
    kraorathumbnail.desktop
    DESTINATION ${SERVICES_INSTALL_DIR})

if (Qt5WebEngineWidgets_FOUND)
   install(FILES htmlthumbnail.desktop DESTINATION ${SERVICES_INSTALL_DIR})
 
endif()

