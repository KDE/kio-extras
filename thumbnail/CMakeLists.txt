add_definitions(-DTRANSLATION_DOMAIN=\"kio5_thumbnail\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(OpenEXR 3.0 CONFIG QUIET)
if(NOT OpenEXR_FOUND)
    find_package(OpenEXR)
endif()
set_package_properties(OpenEXR PROPERTIES
    DESCRIPTION "A library for handling OpenEXR high dynamic-range image files"
    URL "https://www.openexr.com/"
    TYPE OPTIONAL
    PURPOSE "Provides support for OpenEXR formatted images in the thumbnail kioslave"
)

find_package(libappimage 0.1.10 CONFIG)
set_package_properties(libappimage PROPERTIES
    DESCRIPTION "Core library of the AppImage project"
    URL "https://github.com/AppImage/libappimage"
    TYPE OPTIONAL
    PURPOSE "Provides support for AppImage thumbnails"
)

find_package(X11)
set_package_properties(X11 PROPERTIES
    DESCRIPTION "X11 libraries"
    URL "https://www.x.org"
    TYPE OPTIONAL
    PURPOSE "Provides support for XCursor thumbnails"
)

find_package(Taglib 1.11)
set_package_properties(Taglib PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Provides support for audio thumbnails"
)

include_directories(${CMAKE_BINARY_DIR})

########### next target ###############

add_library(kio_thumbnail MODULE)
set_target_properties(kio_thumbnail PROPERTIES
    OUTPUT_NAME "thumbnail"
)

target_sources(kio_thumbnail PRIVATE
    thumbnail.cpp
    imagefilter.cpp
)

ecm_qt_declare_logging_category(kio_thumbnail
    HEADER thumbnail-logsettings.h
    IDENTIFIER KIO_THUMBNAIL_LOG
    CATEGORY_NAME kf.kio.slaves.thumbnail
    OLD_CATEGORY_NAMES log_kio_thumbnail
    DESCRIPTION "KIO thumbnail"
    EXPORT KIO_EXTRAS
)

target_compile_definitions(kio_thumbnail
    PRIVATE
        -DKSERVICE_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055100 # KMimeTypeTrader
)

target_link_libraries(kio_thumbnail
    KF5::CoreAddons
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::I18n
)

install(TARGETS kio_thumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### next target ###############

add_library(imagethumbnail MODULE imagecreator.cpp)

target_link_libraries(imagethumbnail
    KF5::KIOWidgets
)

install(TARGETS imagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES imagethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

install(FILES directorythumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

add_library(jpegthumbnail MODULE jpegcreator.cpp)

kconfig_add_kcfg_files(jpegthumbnail jpegcreatorsettings5.kcfgc)

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
install(FILES jpegthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

add_library(svgthumbnail MODULE svgcreator.cpp)

target_link_libraries(svgthumbnail
    Qt5::Gui
    Qt5::Svg
    KF5::KIOCore
    KF5::KIOWidgets
)

install(TARGETS svgthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES svgthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

add_library(textthumbnail MODULE textcreator.cpp)

target_link_libraries(textthumbnail
    Qt5::Gui
    KF5::KIOWidgets
    KF5::SyntaxHighlighting
)

install(TARGETS textthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES textthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

if(NOT WIN32)

    add_library(djvuthumbnail MODULE djvucreator.cpp)

    ecm_qt_declare_logging_category(djvuthumbnail
        HEADER thumbnail-djvu-logsettings.h
        IDENTIFIER KIO_THUMBNAIL_DJVU_LOG
        CATEGORY_NAME kf.kio.slaves.thumbnail.djvu
        OLD_CATEGORY_NAMES log_kio_thumbnail.djvu
        DESCRIPTION "DjVu files thumbnailer"
        EXPORT KIO_EXTRAS
    )

    target_link_libraries(djvuthumbnail KF5::KIOWidgets)

    install(TARGETS djvuthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install(FILES djvuthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

########### next target ###############

if(OpenEXR_FOUND)

    add_library(exrthumbnail MODULE exrcreator.cpp)

    ecm_qt_declare_logging_category(exrthumbnail
        HEADER thumbnail-exr-logsettings.h
        IDENTIFIER KIO_THUMBNAIL_EXR_LOG
        CATEGORY_NAME kf.kio.slaves.thumbnail.exr
        OLD_CATEGORY_NAMES log_kio_thumbnail.exr
        DESCRIPTION "EXR images thumbnailer"
        EXPORT KIO_EXTRAS
    )

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
    install(FILES exrthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

########### next target ###############

if(X11_Xcursor_FOUND)

    add_library(cursorthumbnail MODULE cursorcreator.cpp)

    target_link_libraries(cursorthumbnail
        KF5::KIOCore
        KF5::KIOWidgets
        ${X11_Xcursor_LIB}
    )

    install(TARGETS cursorthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install(FILES cursorthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

########### next target ###############

add_library(windowsexethumbnail MODULE windowsexecreator.cpp icoutils_common.cpp)
add_library(windowsimagethumbnail MODULE windowsimagecreator.cpp icoutils_common.cpp)

if(WIN32)
    target_sources(windowsexethumbnail PRIVATE icoutils_win.cpp)
    target_sources(windowsimagethumbnail PRIVATE icoutils_win.cpp)
else()
    target_sources(windowsexethumbnail PRIVATE icoutils_wrestool.cpp)
    target_sources(windowsimagethumbnail PRIVATE icoutils_wrestool.cpp)
endif()

target_link_libraries(windowsexethumbnail KF5::KIOWidgets)
install(TARGETS windowsexethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES windowsexethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

target_link_libraries(windowsimagethumbnail KF5::KIOWidgets)
install(TARGETS windowsimagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES windowsimagethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target ###############

add_library(comicbookthumbnail MODULE comiccreator.cpp)

ecm_qt_declare_logging_category(comicbookthumbnail
    HEADER thumbnail-comic-logsettings.h
    IDENTIFIER KIO_THUMBNAIL_COMIC_LOG
    CATEGORY_NAME kf.kio.slaves.thumbnail.comic
    OLD_CATEGORY_NAMES log_kio_thumbnail.comic
    DESCRIPTION "Comic books thumbnailer"
    EXPORT KIO_EXTRAS
)

target_link_libraries(comicbookthumbnail
    Qt5::Gui
    KF5::Archive
    KF5::KIOWidgets
)

install(TARGETS comicbookthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES comicbookthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

# ########### next target ###############

add_library(kritathumbnail MODULE kritacreator.cpp)

target_link_libraries(kritathumbnail
    KF5::KIOWidgets
    KF5::Archive
)

install(TARGETS kritathumbnail  DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES kraorathumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### next target #################

if(Taglib_FOUND)

    add_library(audiothumbnail MODULE audiocreator.cpp)

    target_link_libraries(audiothumbnail
        Taglib::Taglib
        KF5::KIOWidgets
    )

    install(TARGETS audiothumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install(FILES audiothumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

# ########### next target ###############

add_library(opendocumentthumbnail MODULE opendocumentcreator.cpp)

target_link_libraries(opendocumentthumbnail
    Qt5::Gui
    KF5::KIOWidgets
    KF5::Archive
)

install(TARGETS opendocumentthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES opendocumentthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

# ########### next target ###############

if(libappimage_FOUND)

    add_library(appimagethumbnail MODULE appimagecreator.cpp)

    target_link_libraries(appimagethumbnail
        KF5::KIOWidgets
        Qt5::Gui
        libappimage
    )

    install(TARGETS appimagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
    install(FILES appimagethumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

endif()

# ########### next target ###############

add_library(ebookthumbnail MODULE ebookcreator.cpp)

target_link_libraries(ebookthumbnail
    Qt5::Gui
    KF5::KIOWidgets
    KF5::Archive
)

install(TARGETS ebookthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR})
install(FILES ebookthumbnail.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

########### install files ###############

install(FILES thumbcreator.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
