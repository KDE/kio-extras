project( networkwatcher )

include_directories(
  ../network
  ../ioslave
)
set( kded_networkwatcher_SRCS
  networkdbusadaptor.cpp
  kioslavenotifier.cpp
  networkwatcher.cpp
  main.cpp
)

qt4_add_dbus_adaptor( kded_networkwatcher_SRCS  org.kde.network.kioslavenotifier.xml kioslavenotifier.h Mollet::KioSlaveNotifier )

kde4_add_plugin( kded_networkwatcher  ${kded_networkwatcher_SRCS} )

target_link_libraries( kded_networkwatcher  ${KDE4_KIO_LIBS} molletnetwork )

install( TARGETS kded_networkwatcher  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES networkwatcher.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kded )
install( FILES org.kde.network.kioslavenotifier.xml  DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )
