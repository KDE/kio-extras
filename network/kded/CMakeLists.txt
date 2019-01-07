include_directories(
  ../network
  ../ioslave
)
set( kded_networkwatcher_SRCS
  networkdbusadaptor.cpp
  kioslavenotifier.cpp
  networkwatcher.cpp
)

qt5_add_dbus_adaptor( kded_networkwatcher_SRCS  org.kde.network.kioslavenotifier.xml kioslavenotifier.h Mollet::KioSlaveNotifier )

add_library(kded_networkwatcher MODULE  ${kded_networkwatcher_SRCS} )
set_target_properties(kded_networkwatcher PROPERTIES OUTPUT_NAME networkwatcher)
kcoreaddons_desktop_to_json(kded_networkwatcher networkwatcher.desktop)

target_link_libraries( kded_networkwatcher KF5::DBusAddons KF5::KIOCore molletnetwork5 )

install( TARGETS kded_networkwatcher  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded )

install( FILES org.kde.network.kioslavenotifier.xml  DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} RENAME kf5_org.kde.network.kioslavenotifier.xml )
