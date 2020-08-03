option(BUILD_KDSoapWSDiscoveryClient "Automatically build WSD client if a system one isn't found." ON)
find_package(KDSoapWSDiscoveryClient QUIET)
set(INTERNAL_WSDCLIENT ${BUILD_KDSoapWSDiscoveryClient})
if(KDSoapWSDiscoveryClient_FOUND)
    set(INTERNAL_WSDCLIENT OFF)
endif()
if(INTERNAL_WSDCLIENT)
    # Special internal version, mangled to be a STATIC lib.
    # This is only useful and necessary until the library has
    # its API finalized and gotten a stable release.
    # Until then when using it as external library you should try
    # to use a snapshot that matches the internal copy's git ref.
    # Currently at: 14287e92e80a77aa4c0adee2871e6b87c9c3055e
    add_subdirectory(kdsoap-ws-discovery-client)
endif()
add_feature_info("Internal KDSoapWSDiscoveryClient" INTERNAL_WSDCLIENT
    "Building using internal client because a system-provided version could not be found.")

add_feature_info("SMB DNS-SD Discovery" HAVE_KDNSSD_WITH_SIGNAL_RACE_PROTECTION
    "Discover SMB hosts via DNS-SD/Avahi/Bonjour. KF5DNSSD >= 5.54 is required to support this.")

find_package(Threads REQUIRED)

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_smb\")

include(CheckIncludeFile)
include(CheckSymbolExists)
include(CMakePushCheckState)
set(CMAKE_AUTOMAKE ON)

cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_INCLUDES ${SAMBA_INCLUDE_DIR})
list(APPEND CMAKE_REQUIRED_LIBRARIES ${SAMBA_LIBRARIES})
check_symbol_exists(smbc_readdirplus2 "libsmbclient.h" HAVE_READDIRPLUS2)
cmake_pop_check_state()
check_include_file(utime.h HAVE_UTIME_H)

configure_file(config-smb.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-smb.h)

set(kio_smb_PART_SRCS 
    kio_smb.cpp
    kio_smb_auth.cpp
    kio_smb_browse.cpp
    kio_smb_config.cpp
    kio_smb_dir.cpp
    kio_smb_file.cpp
    smburl.cpp
    kio_smb_mount.cpp
    wsdiscoverer.cpp
    dnssddiscoverer.cpp
    discovery.cpp
    transfer.cpp
    smbcdiscoverer.cpp
    smbcontext.cpp
    smbauthenticator.cpp
)

ecm_qt_declare_logging_category(kio_smb_PART_SRCS
                                HEADER smb-logsettings.h
                                IDENTIFIER KIO_SMB_LOG
                                CATEGORY_NAME log_kio_smb)

include_directories(${SAMBA_INCLUDE_DIR})

# Intermediate static lib target for reuse in testing.
add_library(kio_smb_static STATIC ${kio_smb_PART_SRCS})
target_include_directories(kio_smb_static
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}/..>"
)
target_link_libraries(kio_smb_static
    KF5::KIOCore
    KF5::I18n
    ${SAMBA_LIBRARIES}
    Qt5::Network
    KF5::DNSSD
    KDSoap::WSDiscoveryClient
    Threads::Threads # std::async
)

# Final plugin target.
add_library(kio_smb MODULE main.cpp)
target_link_libraries(kio_smb
    kio_smb_static
)

set_target_properties(kio_smb PROPERTIES OUTPUT_NAME "smb")
set_target_properties(kio_smb PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio")

install(TARGETS kio_smb DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
install(FILES smb-network.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/dirtree/remote)
install(FILES smb-network.desktop DESTINATION ${KDE_INSTALL_DATADIR}/remoteview)

add_subdirectory(autotests)
