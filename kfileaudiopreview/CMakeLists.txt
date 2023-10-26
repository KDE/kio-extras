add_definitions(-DTRANSLATION_DOMAIN=\"kfileaudiopreview6\")

add_library(kfileaudiopreview MODULE)

target_sources(kfileaudiopreview PRIVATE
   kfileaudiopreview.cpp
   mediacontrols.cpp
)

target_include_directories(kfileaudiopreview SYSTEM PRIVATE ${PHONON_INCLUDE_DIR})

target_link_libraries(kfileaudiopreview
   ${PHONON_LIBRARIES}
   KF6::KIOFileWidgets    # KPreviewWidgetBase
   KF6::I18n
)

install(TARGETS kfileaudiopreview
        DESTINATION ${KDE_INSTALL_PLUGINDIR})
