project( molletnetwork )


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
  builder/slpservicebrowser.cpp
  builder/slpnetworkbuilder.cpp
)
elseif(SLP_FOUND)
set( slpnetworkbuilder_SRCS
)
endif(SLP_FOUND)

set( dnssdnetworkbuilder_SRCS
  builder/dnssdnetworkbuilder.cpp
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


set( molletnetwork_LINK_LIBS  ${KDE4_KDECORE_LIBS}  ${KDE4_KDNSSD_LIBS} )

if(SLP_FOUND)
set( molletnetwork_LINK_LIBS  ${molletnetwork_LINK_LIBS} ${SLP_LIBRARIES} )
endif(SLP_FOUND)

kde4_add_library( molletnetwork  SHARED ${molletnetwork_LIB_SRCS} )
target_link_libraries( molletnetwork  ${molletnetwork_LINK_LIBS} )
set_target_properties( molletnetwork  PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )

install( TARGETS molletnetwork  ${INSTALL_TARGETS_DEFAULT_ARGS} )
