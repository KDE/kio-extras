project( molletnetwork )

find_package(SLP)
set_package_properties(SLP PROPERTIES DESCRIPTION "SLP (Service Location Protocol) implementation"
                       URL "http://www.openslp.org/"
                       TYPE OPTIONAL
                       PURPOSE "Provides SLP support in the network:/ kioslave."
                      )

macro_bool_to_01( SLP_FOUND  HAVE_SLP )
configure_file( config-slp.h.cmake  ${CMAKE_CURRENT_BINARY_DIR}/config-slp.h )

include_directories(
  builder
)

set( netsystemdriver_SRCS
  builder/simpleitemfactory.cpp
)

# TODO: find best-practice to do such dependant inclusion of files to compile, this is unreadable
if(SLP_FOUND)
set( slpnetworkbuilder_SRCS
  builder/slp/slpservicebrowser.cpp
  builder/slp/slpnetworkbuilder.cpp
)
endif()

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

set( molletnetwork_LIB_SRCS
  ${netsystemdriver_SRCS}
#   ${slpnetworkbuilder_SRCS}
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


set( molletnetwork_LINK_LIBS
    KF5::DNSSD
    Qt5::Network
    Qt5::Gui
    KF5::KDE4Support
)

if(SLP_FOUND)
set( molletnetwork_LINK_LIBS  ${molletnetwork_LINK_LIBS} ${SLP_LIBRARIES} )
endif()

set(MOLLETNETWORK_VERSION_MAJOR 4)
set(MOLLETNETWORK_VERSION_MINOR 95)
set(MOLLETNETWORK_VERSION_PATCH 0)
set(MOLLETNETWORK_VERSION_STRING "${MOLLETNETWORK_VERSION_MAJOR}.${MOLLETNETWORK_VERSION_MINOR}.${MOLLETNETWORK_VERSION_PATCH}" )

add_library( molletnetwork  SHARED ${molletnetwork_LIB_SRCS} )
target_link_libraries( molletnetwork  ${molletnetwork_LINK_LIBS} )
set_target_properties( molletnetwork  PROPERTIES VERSION ${MOLLETNETWORK_VERSION_STRING} SOVERSION 5 )

install( TARGETS molletnetwork  ${INSTALL_TARGETS_DEFAULT_ARGS} )
