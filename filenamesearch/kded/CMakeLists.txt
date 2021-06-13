add_library(filenamesearchmodule MODULE)

target_sources(filenamesearchmodule PRIVATE
     filenamesearchmodule.cpp
)

kcoreaddons_desktop_to_json(filenamesearchmodule filenamesearchmodule.desktop)

target_link_libraries(filenamesearchmodule
    KF5::DBusAddons
    KF5::KIOCore
)

install(TARGETS filenamesearchmodule DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded)
