add_library(kded_networkwatcher MODULE)

set_target_properties(kded_networkwatcher PROPERTIES OUTPUT_NAME networkwatcher)
kcoreaddons_desktop_to_json(kded_networkwatcher networkwatcher.desktop)

set(kded_networkwatcher_dbus_SRCS)
qt5_add_dbus_adaptor(kded_networkwatcher_dbus_SRCS  org.kde.network.kioslavenotifier.xml kioslavenotifier.h Mollet::KioSlaveNotifier)

target_sources(kded_networkwatcher PRIVATE
    networkdbusadaptor.cpp
    kioslavenotifier.cpp
    networkwatcher.cpp
    ${kded_networkwatcher_dbus_SRCS}
)

target_include_directories(kded_networkwatcher PRIVATE
  ../ioslave
)

target_link_libraries(kded_networkwatcher PRIVATE
    molletnetwork5
    KF5::DBusAddons
    KF5::KIOCore
)

install( TARGETS kded_networkwatcher  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded )

install( FILES org.kde.network.kioslavenotifier.xml  DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} RENAME kf5_org.kde.network.kioslavenotifier.xml )
