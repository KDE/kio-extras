
if(NOT WIN32)
    add_library(kcm_trash MODULE)
    target_sources(kcm_trash PRIVATE
        kcmtrash.cpp
    )
    target_link_libraries(kcm_trash
       trash_common_unix
       Qt6::DBus
       KF6::I18n
       KF6::KCMUtils
       KF6::KIOCore
       KF6::Solid)
    if(APPLE)
        target_link_libraries(kcm_trash "-framework DiskArbitration -framework CoreFoundation")
    endif(APPLE)

    kcmutils_generate_desktop_file(kcm_trash)
    install(TARGETS kcm_trash DESTINATION ${KDE_INSTALL_PLUGINDIR})
endif()
