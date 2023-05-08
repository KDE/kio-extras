kcoreaddons_add_plugin(kcm_webshortcuts INSTALL_NAMESPACE "plasma/kcms/systemsettings_qwidgets")

target_sources(kcm_webshortcuts PRIVATE
    ikwsopts.cpp
    searchprovider.cpp
    searchproviderdlg.cpp
    searchproviderregistry.cpp
)

kcmutils_generate_desktop_file(kcm_webshortcuts)
ki18n_wrap_ui(kcm_webshortcuts ikwsopts_ui.ui searchproviderdlg_ui.ui)

target_link_libraries(kcm_webshortcuts
    KF6::KCMUtils # KCModule
    KF6::KIOWidgets    # KUriFilter
    KF6::I18n
    KF6::CoreAddons
    Qt::Core5Compat
)
