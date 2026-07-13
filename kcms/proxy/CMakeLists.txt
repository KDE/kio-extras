if(WITH_LIBPROXY)
    add_subdirectory(wpad-detector)
endif()

kcmutils_add_qml_kcm(kcm_proxy
    SOURCES
        proxy.cpp
        proxy.h
        ksaveioconfig.cpp
        ksaveioconfig.h
)

kconfig_add_kcfg_files(kcm_proxy kioslave.kcfgc GENERATE_MOC)

target_link_libraries(kcm_proxy PRIVATE
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::I18n
    KF6::I18nQml
    KF6::KCMUtils
    KF6::KIOCore
)

if(USE_DBUS)
  target_link_libraries(kcm_proxy PRIVATE Qt6::DBus)
  target_compile_definitions(kcm_proxy PUBLIC -DWITH_DBUS)
endif()
