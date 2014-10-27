project( kio_network )

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
target_link_libraries( kio_network  molletnetwork KF5::KIOCore )
set_target_properties(kio_network PROPERTIES OUTPUT_NAME "network")

install( TARGETS kio_network  DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

install( FILES network.protocol  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES network.desktop  DESTINATION ${DATA_INSTALL_DIR}/remoteview)
install( FILES virtualfolder_network.desktop  DESTINATION ${DATA_INSTALL_DIR}/konqsidebartng/virtual_folders/remote)
