kcoreaddons_add_plugin(kcm_netpref INSTALL_NAMESPACE "plasma/kcms/systemsettings_qwidgets")

kcmutils_generate_desktop_file(kcm_netpref)
target_sources(kcm_netpref PRIVATE
    netpref.cpp
    ../ksaveioconfig.cpp
)

target_link_libraries(kcm_netpref
  PUBLIC
    KF6::ConfigCore
    KF6::KCMUtils
    KF6::KIOCore
  PRIVATE
    KF6::I18n
    KF6::CoreAddons
)

if(USE_DBUS)
  target_link_libraries(kcm_netpref PUBLIC Qt6::DBus)
  target_compile_definitions(kcm_netpref PUBLIC -DWITH_DBUS)
endif()
