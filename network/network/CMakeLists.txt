

include_directories(
  builder
)

set( netsystemdriver_SRCS
  builder/simpleitemfactory.cpp
)

set( dnssdnetworkbuilder_SRCS
  builder/dnssd/dnssdnetworkbuilder.cpp
)

set( upnpnetworkbuilder_SRCS
  builder/upnp/cagibidevice.cpp
  builder/upnp/cagibidbuscodec.cpp
  builder/upnp/upnpnetworkbuilder.cpp
)

set( networkbuilder_SRCS
  builder/abstractnetworkbuilder.cpp
  builder/abstractnetsystemfactory.cpp
)

set( networkdbus_LIB_SRCS
  networkdbus.cpp
)

set( molletnetwork5_LIB_SRCS
  ${netsystemdriver_SRCS}
  ${upnpnetworkbuilder_SRCS}
  ${dnssdnetworkbuilder_SRCS}
  ${networkbuilder_SRCS}
  ${networkdbus_LIB_SRCS}
  network_p.cpp
  network.cpp
  netdevice_p.cpp
  netdevice.cpp
  netservice_p.cpp
  netservice.cpp
)

add_library( molletnetwork5  SHARED ${molletnetwork5_LIB_SRCS} )
target_link_libraries( molletnetwork5
    PUBLIC
        KF5::DNSSD
        Qt5::Network
        Qt5::Gui
        Qt5::DBus
)

set_target_properties( molletnetwork5 PROPERTIES VERSION ${RELEASE_SERVICE_VERSION} SOVERSION ${RELEASE_SERVICE_VERSION_MAJOR} )

install( TARGETS molletnetwork5 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP )
