include_directories(
  ../network
)

set( kio_network_SRCS
  networkdbusinterface.cpp
  networkinitwatcher.cpp
  networkthread.cpp
  networkuri.cpp
  mimetypes.cpp
  networkslave.cpp
  main.cpp
)

# qt5_add_dbus_interface( kio_network_SRCS  ../kded/org.kde.network.xml networkdbusproxy )

add_library(kio_network MODULE  ${kio_network_SRCS} )
target_link_libraries( kio_network  molletnetwork5 KF5::KIOCore )
set_target_properties(kio_network PROPERTIES OUTPUT_NAME "network")

install( TARGETS kio_network  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

install( FILES network.protocol  DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install( FILES network.desktop  DESTINATION ${KDE_INSTALL_DATADIR}/remoteview)
install( FILES virtualfolder_network.desktop  DESTINATION ${KDE_INSTALL_DATADIR}/konqsidebartng/virtual_folders/remote)
