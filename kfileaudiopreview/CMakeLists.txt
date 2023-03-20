add_definitions(-DTRANSLATION_DOMAIN=\"kfileaudiopreview5\")

add_library(kfileaudiopreview MODULE)

target_sources(kfileaudiopreview PRIVATE
   kfileaudiopreview.cpp
   mediacontrols.cpp
)

target_include_directories(kfileaudiopreview SYSTEM PRIVATE ${PHONON_INCLUDE_DIR})

target_link_libraries(kfileaudiopreview
   ${PHONON_LIBRARIES}
   KF${QT_MAJOR_VERSION}::KIOFileWidgets    # KPreviewWidgetBase
   KF${QT_MAJOR_VERSION}::I18n
)

install(TARGETS kfileaudiopreview
        DESTINATION ${KDE_INSTALL_PLUGINDIR})
