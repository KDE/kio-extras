add_library(molletnetwork5 SHARED)

set_target_properties( molletnetwork5 PROPERTIES
    VERSION ${RELEASE_SERVICE_VERSION}
    SOVERSION ${RELEASE_SERVICE_VERSION_MAJOR}
)

generate_export_header(molletnetwork5 BASE_NAME molletnetwork)

target_sources(molletnetwork5 PRIVATE
    builder/simpleitemfactory.cpp

    builder/dnssd/dnssdnetworkbuilder.cpp

    builder/upnp/cagibidevice.cpp
    builder/upnp/cagibidbuscodec.cpp
    builder/upnp/upnpnetworkbuilder.cpp

    builder/abstractnetworkbuilder.cpp
    builder/abstractnetsystemfactory.cpp

    networkdbus.cpp

    network_p.cpp
    network.cpp
    netdevice_p.cpp
    netdevice.cpp
    netservice_p.cpp
    netservice.cpp
)

target_include_directories(molletnetwork5
    PRIVATE
        builder
)

target_link_libraries( molletnetwork5
    PUBLIC
        KF5::DNSSD
        Qt5::Network
        Qt5::Gui
        Qt5::DBus
)

install( TARGETS molletnetwork5 ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP )
