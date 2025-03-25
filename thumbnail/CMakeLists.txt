if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

add_definitions(-DTRANSLATION_DOMAIN=\"kio6_thumbnail\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(OpenEXR 3.0 CONFIG QUIET)
if(NOT OpenEXR_FOUND)
    find_package(OpenEXR)
endif()
set_package_properties(OpenEXR PROPERTIES
    DESCRIPTION "A library for handling OpenEXR high dynamic-range image files"
    URL "https://www.openexr.com/"
    TYPE OPTIONAL
    PURPOSE "Provides support for OpenEXR formatted images in the thumbnail KIO worker"
)

find_package(libappimage 0.1.10 CONFIG)
set_package_properties(libappimage PROPERTIES
    DESCRIPTION "Core library of the AppImage project"
    URL "https://github.com/AppImage/libappimage"
    TYPE OPTIONAL
    PURPOSE "Provides support for AppImage thumbnails"
)

option(WITHOUT_X11 "Build without support for XCursor thumbnails (disables finding X11)" OFF)
if (NOT WITHOUT_X11)
    find_package(X11)
    set_package_properties(X11 PROPERTIES
        DESCRIPTION "X11 libraries"
        URL "https://www.x.org"
        TYPE OPTIONAL
        PURPOSE "Provides support for XCursor thumbnails"
    )
endif()

find_package(Taglib 1.11)
set_package_properties(Taglib PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Provides support for audio thumbnails"
)

find_package(KExiv2Qt6)
set_package_properties(KExiv2Qt6 PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Provides support for extracting JPEG thumbnails")
if (KExiv2Qt6_FOUND)
    set(HAVE_KEXIV2 TRUE)
endif()

include_directories(${CMAKE_BINARY_DIR})

include(ECMSetupQtPluginMacroNames)
ecm_setup_qtplugin_macro_names(
    JSON_ARG2
        "EXPORT_THUMBNAILER_WITH_JSON"
    CONFIG_CODE_VARIABLE
        PACKAGE_SETUP_AUTOMOC_VARIABLES
)

configure_file(config-thumbnail.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-thumbnail.h)

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
    CATEGORY_NAME kf.kio.workers.thumbnail
    OLD_CATEGORY_NAMES log_kio_thumbnail kf.kio.slaves.thumbnail
    DESCRIPTION "KIO thumbnail"
    EXPORT KIO_EXTRAS
)

target_link_libraries(kio_thumbnail
    PUBLIC
        KF6::CoreAddons
        KF6::KIOCore
        KF6::KIOWidgets
        KF6::I18n
    PRIVATE
        Qt6::GuiPrivate
)

install(TARGETS kio_thumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kio)

########### next target ###############

add_library(imagethumbnail MODULE imagecreator.cpp)

target_link_libraries(imagethumbnail
    KF6::CoreAddons
    KF6::KIOGui
)

install(TARGETS imagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

########### next target ###############

add_library(jpegthumbnail MODULE jpegcreator.cpp)

kconfig_add_kcfg_files(jpegthumbnail jpegcreatorsettings5.kcfgc)

target_link_libraries(jpegthumbnail
    Qt::Core
    Qt::Gui
    KF6::KIOCore
    KF6::KIOGui
    KF6::I18n
    KF6::ConfigCore
    KF6::ConfigGui
)
if (TARGET KExiv2Qt6)
    target_link_libraries(jpegthumbnail KExiv2Qt6)
endif()

install(FILES jpegcreatorsettings5.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
install(TARGETS jpegthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

########### next target ###############

add_library(svgthumbnail MODULE svgcreator.cpp)

target_link_libraries(svgthumbnail
    Qt::Gui
    Qt::Svg
    KF6::KIOCore
    KF6::KIOGui
)

install(TARGETS svgthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

########### next target ###############

add_library(textthumbnail MODULE textcreator.cpp)

target_link_libraries(textthumbnail
    Qt::Gui
    KF6::KIOGui
    KF6::SyntaxHighlighting
)

install(TARGETS textthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

########### next target ###############

if(NOT WIN32)

    add_library(djvuthumbnail MODULE djvucreator.cpp)

    ecm_qt_declare_logging_category(djvuthumbnail
        HEADER thumbnail-djvu-logsettings.h
        IDENTIFIER KIO_THUMBNAIL_DJVU_LOG
        CATEGORY_NAME kf.kio.workers.thumbnail.djvu
        OLD_CATEGORY_NAMES log_kio_thumbnail.djvu kf.kio.slaves.thumbnail.djvu
        DESCRIPTION "DjVu files thumbnailer"
        EXPORT KIO_EXTRAS
    )

    target_link_libraries(djvuthumbnail KF6::KIOGui)

    install(TARGETS djvuthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

endif()

########### next target ###############

if(OpenEXR_FOUND)

    add_library(exrthumbnail MODULE exrcreator.cpp)

    ecm_qt_declare_logging_category(exrthumbnail
        HEADER thumbnail-exr-logsettings.h
        IDENTIFIER KIO_THUMBNAIL_EXR_LOG
        CATEGORY_NAME kf.kio.workers.thumbnail.exr
        OLD_CATEGORY_NAMES log_kio_thumbnail.exr kf.kio.slaves.thumbnail.exr
        DESCRIPTION "EXR images thumbnailer"
        EXPORT KIO_EXTRAS
    )

    target_link_libraries(exrthumbnail
        KF6::KIOCore
        KF6::KIOGui
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

    install(TARGETS exrthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

endif()

########### next target ###############

if(X11_Xcursor_FOUND AND NOT WITHOUT_X11)

    add_library(cursorthumbnail MODULE cursorcreator.cpp)

    target_link_libraries(cursorthumbnail
        KF6::KIOCore
        KF6::KIOGui
        KF6::CoreAddons
        ${X11_Xcursor_LIB}
    )

    install(TARGETS cursorthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

endif()

########### next target ###############

add_library(windowsexethumbnail MODULE windowsexecreator.cpp icoutils.cpp exeutils.cpp)
add_library(windowsimagethumbnail MODULE windowsimagecreator.cpp icoutils.cpp exeutils.cpp)

target_link_libraries(windowsexethumbnail KF6::KIOGui)
install(TARGETS windowsexethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

target_link_libraries(windowsimagethumbnail KF6::KIOGui)
install(TARGETS windowsimagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

########### next target ###############

add_library(comicbookthumbnail MODULE comiccreator.cpp)

ecm_qt_declare_logging_category(comicbookthumbnail
    HEADER thumbnail-comic-logsettings.h
    IDENTIFIER KIO_THUMBNAIL_COMIC_LOG
    CATEGORY_NAME kf.kio.workers.thumbnail.comic
    OLD_CATEGORY_NAMES log_kio_thumbnail.comic kf.kio.slaves.thumbnail.comic
    DESCRIPTION "Comic books thumbnailer"
    EXPORT KIO_EXTRAS
)

target_link_libraries(comicbookthumbnail
    Qt::Gui
    KF6::Archive
    KF6::KIOGui
)

install(TARGETS comicbookthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

# ########### next target ###############

add_library(kraorathumbnail MODULE kritacreator.cpp)

target_link_libraries(kraorathumbnail
    KF6::KIOGui
    KF6::Archive
)

install(TARGETS kraorathumbnail  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

########### next target #################

if(Taglib_FOUND)

    add_library(audiothumbnail MODULE audiocreator.cpp)

    target_link_libraries(audiothumbnail
        Taglib::Taglib
        KF6::KIOGui
    )

    install(TARGETS audiothumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

endif()

# ########### next target ###############

add_library(opendocumentthumbnail MODULE opendocumentcreator.cpp)

target_link_libraries(opendocumentthumbnail
    Qt::Gui
    KF6::KIOGui
    KF6::Archive
)

install(TARGETS opendocumentthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

# ########### next target ###############

if(libappimage_FOUND)

    add_library(appimagethumbnail MODULE appimagecreator.cpp)

    target_link_libraries(appimagethumbnail
        KF6::KIOGui
        Qt::Gui
        libappimage
    )

    install(TARGETS appimagethumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

endif()

# ########### next target ###############

add_library(ebookthumbnail MODULE ebookcreator.cpp)

target_link_libraries(ebookthumbnail
    Qt::Gui
    KF6::KIOGui
    KF6::Archive
)

install(TARGETS ebookthumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)

# ########### next target ###############

add_library(directorythumbnail MODULE directorycreator.cpp)

target_link_libraries(directorythumbnail
    Qt::Gui
    KF6::CoreAddons
    KF6::KIOGui
)

install(TARGETS directorythumbnail DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/thumbcreator)
