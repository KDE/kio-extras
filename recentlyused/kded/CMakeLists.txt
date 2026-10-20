add_library(recentlyusedmodule MODULE)

target_sources(recentlyusedmodule PRIVATE
    recentlyusedmodule.cpp
)

target_link_libraries(recentlyusedmodule
    KF6::DBusAddons
    KF6::KIOCore
    Plasma::ActivitiesStats
    Qt6::DBus
)

install(TARGETS recentlyusedmodule DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kded)
