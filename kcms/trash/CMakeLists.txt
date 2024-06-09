add_library(kcm_trash MODULE)
target_sources(kcm_trash PRIVATE
    kcmtrash.cpp
)
target_link_libraries(kcm_trash
   KF6::I18n
   KF6::KCMUtils
   KF6::KIOCore
   KF6::Solid
 )

if(USE_DBUS)
  target_link_libraries(kcm_trash Qt6::DBus)
  target_compile_definitions(kcm_trash PUBLIC -DWITH_DBUS)
endif()

if(APPLE)
    target_link_libraries(kcm_trash "-framework DiskArbitration -framework CoreFoundation")
endif(APPLE)

kcmutils_generate_desktop_file(kcm_trash)
install(TARGETS kcm_trash DESTINATION ${KDE_INSTALL_PLUGINDIR})
