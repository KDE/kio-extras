add_definitions(-DTRANSLATION_DOMAIN=\"kio5_thumbnail\")

find_package(OpenEXR 3.0 CONFIG QUIET)
if(NOT OpenEXR_FOUND)
    find_package(OpenEXR)
endif()
set_package_properties(OpenEXR PROPERTIES DESCRIPTION "API for accessing OpenEXR formatted images"
                       URL "https://www.openexr.com"
                       TYPE OPTIONAL
                       PURPOSE "Provides support for OpenEXR formatted images in the thumbnail kioslave"
                      )

find_package(libappimage 0.1.10 CONFIG)
set_package_properties(libappimage PROPERTIES DESCRIPTION "Core library of the AppImage project"
                       URL "https://github.com/AppImage/libappimage"
                       TYPE OPTIONAL
                       PURPOSE "Provides support for AppImage thumbnails"
                       )

find_package(X11)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                        URL "https://www.x.org"
                        TYPE OPTIONAL
                        PURPOSE "Provides support for XCursor thumbnails"
                    )

include_directories(${CMAKE_BINARY_DIR})

########### next target ###############

set(kio_thumbnail_PART_SRCS thumbnail.cpp imagefilter.cpp)

ecm_qt_declare_logging_category(kio_thumbnail_PART_SRCS
                                HEADER thumbnail-logsettings.h
                                IDENTIFIER KIO_THUMBNAIL_LOG
                                CATEGORY_NAME log_kio_thumbnail)

add_library(kio_thumbnail MODULE ${kio_thumbnail_PART_SRCS})

target_link_libraries(kio_thumbnail
	KF5::CoreAddons
	KF5::KIOCore
	KF5::KIOWidgets
 	KF5::I18n
)

set_target_properties(kio_thumbnail PROPERTIES OUTPUT_NAME "thumbnail")

install(TARGETS kio_thumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### next target ###############

set(imagethumbnail_PART_SRCS imagecreator.cpp)

add_library(imagethumbnail MODULE ${imagethumbnail_PART_SRCS})

target_link_libraries(imagethumbnail
    KF5::KIOWidgets
)

install(TARGETS imagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### next target ###############

install( FILES directorythumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

set(jpegthumbnail_PART_SRCS jpegcreator.cpp)
kconfig_add_kcfg_files(jpegthumbnail_PART_SRCS jpegcreatorsettings5.kcfgc)
add_library(jpegthumbnail MODULE ${jpegthumbnail_PART_SRCS})

target_link_libraries(jpegthumbnail
	Qt5::Core
	Qt5::Gui
	KF5::KIOCore
	KF5::KIOWidgets
	KF5::I18n
	KF5::ConfigCore
	KF5::ConfigGui
)

install(FILES jpegcreatorsettings5.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(TARGETS jpegthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### next target ###############

set(svgthumbnail_PART_SRCS svgcreator.cpp)

add_library(svgthumbnail MODULE ${svgthumbnail_PART_SRCS})

target_link_libraries(svgthumbnail
	Qt5::Gui
	Qt5::Svg
	KF5::KIOCore
	KF5::KIOWidgets
)

install(TARGETS svgthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### next target ###############

set(textthumbnail_PART_SRCS textcreator.cpp)

add_library(textthumbnail MODULE ${textthumbnail_PART_SRCS})

target_link_libraries(textthumbnail
    Qt5::Gui
    KF5::KIOWidgets
    KF5::SyntaxHighlighting
)

install(TARGETS textthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### next target ###############

if(NOT WIN32)
set(djvuthumbnail_PART_SRCS djvucreator.cpp)

ecm_qt_declare_logging_category(djvuthumbnail_PART_SRCS
                                HEADER thumbnail-djvu-logsettings.h
                                IDENTIFIER KIO_THUMBNAIL_DJVU_LOG
                                CATEGORY_NAME log_kio_thumbnail.djvu)

add_library(djvuthumbnail MODULE ${djvuthumbnail_PART_SRCS})

target_link_libraries(djvuthumbnail KF5::KIOWidgets)

install(TARGETS djvuthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES djvuthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

########### next target ###############

if(OpenEXR_FOUND)

    set(exrthumbnail_PART_SRCS exrcreator.cpp)
    ecm_qt_declare_logging_category(exrthumbnail_PART_SRCS
                                    HEADER thumbnail-exr-logsettings.h
                                    IDENTIFIER KIO_THUMBNAIL_EXR_LOG
                                    CATEGORY_NAME log_kio_thumbnail.exr)

    add_library(exrthumbnail MODULE ${exrthumbnail_PART_SRCS})
    target_link_libraries(exrthumbnail
            KF5::KIOCore
            KF5::KIOWidgets
    )
    if(TARGET OpenEXR::OpenEXR)
        target_link_libraries(exrthumbnail OpenEXR::OpenEXR)
    else()
        target_include_directories(exrthumbnail SYSTEM PRIVATE ${OpenEXR_INCLUDE_DIRS})
        target_link_libraries(exrthumbnail ${OpenEXR_LIBRARIES})
    endif()

    # OpenEXR headers use exceptions; at least clang refuses to build the target
    # when exceptions are not enabled.
    kde_source_files_enable_exceptions(exrcreator.cpp)

    install(TARGETS exrthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install( FILES exrthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

########### next target ###############

if(X11_Xcursor_FOUND)

    set(cursorthumbnail_PART_SRCS cursorcreator.cpp)

    add_library(cursorthumbnail MODULE ${cursorthumbnail_PART_SRCS})

    target_link_libraries(cursorthumbnail
        KF5::KIOCore
        KF5::KIOWidgets
        ${X11_Xcursor_LIB}
    )

    install(TARGETS cursorthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install( FILES cursorthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

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
install ( TARGETS windowsexethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install ( FILES windowsexethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

add_library(windowsimagethumbnail MODULE ${windowsimagethumbnail_SRCS} )
target_link_libraries ( windowsimagethumbnail KF5::KIOWidgets )
install ( TARGETS windowsimagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR} )
install ( FILES windowsimagethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR} )

########### next target ###############

set(comicbookthumbnail_SRCS comiccreator.cpp)
ecm_qt_declare_logging_category(comicbookthumbnail_SRCS
                                HEADER thumbnail-comic-logsettings.h
                                IDENTIFIER KIO_THUMBNAIL_COMIC_LOG
                                CATEGORY_NAME log_kio_thumbnail.comic)

add_library(comicbookthumbnail MODULE ${comicbookthumbnail_SRCS})
target_link_libraries(comicbookthumbnail
    Qt5::Gui
    KF5::Archive
    KF5::KIOWidgets
)
install(TARGETS comicbookthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

# ########### next target ###############

set(kritathumbnail_SRCS kritacreator.cpp)
add_library(kritathumbnail MODULE ${kritathumbnail_SRCS})

target_link_libraries(kritathumbnail
    KF5::KIOWidgets
    KF5::Archive
)

install(TARGETS kritathumbnail  DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### next target #################

find_package(Taglib 1.11)

if(Taglib_FOUND)
    add_library(audiothumbnail MODULE audiocreator.cpp)
    target_link_libraries(audiothumbnail Taglib::Taglib KF5::KIOWidgets)
    install(TARGETS audiothumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    set(audiothumbnail_desktop audiothumbnail.desktop)
endif()

# ########### next target ###############

set(opendocumentthumbnail_SRCS opendocumentcreator.cpp)
add_library(opendocumentthumbnail MODULE ${opendocumentthumbnail_SRCS})

target_link_libraries(opendocumentthumbnail
    Qt5::Gui
    KF5::KIOWidgets
    KF5::Archive
)

install(TARGETS opendocumentthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

# ########### next target ###############

if(libappimage_FOUND)

set(appimagethumbnail_SRCS appimagecreator.cpp)
add_library(appimagethumbnail MODULE ${appimagethumbnail_SRCS})
target_link_libraries(appimagethumbnail
    KF5::KIOWidgets
    Qt5::Gui
    libappimage
)

install(TARGETS appimagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES appimagethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

# ########### next target ###############

set(ebookthumbnail_SRCS ebookcreator.cpp)
add_library(ebookthumbnail MODULE ${ebookthumbnail_SRCS})

target_link_libraries(ebookthumbnail
    Qt5::Gui
    KF5::KIOWidgets
    KF5::Archive
)

install(TARGETS ebookthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### install files ###############

install(FILES thumbcreator.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
install(FILES
    svgthumbnail.desktop
    imagethumbnail.desktop
    jpegthumbnail.desktop
    textthumbnail.desktop
#    desktopthumbnail.desktop
    comicbookthumbnail.desktop
    kraorathumbnail.desktop
    opendocumentthumbnail.desktop
    ebookthumbnail.desktop
    ${audiothumbnail_desktop}
    DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

