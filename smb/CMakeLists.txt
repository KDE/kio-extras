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
    add_subdirectory(kdsoap-ws-discovery-client)
endif()
add_feature_info("Internal KDSoapWSDiscoveryClient" INTERNAL_WSDCLIENT
    "Building using internal client because a system-provided version could not be found.")

add_feature_info("SMB DNS-SD Discovery" HAVE_KDNSSD_WITH_SIGNAL_RACE_PROTECTION
    "Discover SMB hosts via DNS-SD/Avahi/Bonjour. KF5DNSSD >= 5.54 is required to support this.")

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_smb\")

include(CheckIncludeFile)
set(CMAKE_AUTOMAKE ON)

check_include_file(utime.h HAVE_UTIME_H)

configure_file(config-smb.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-smb.h)

set(kio_smb_PART_SRCS 
   kio_smb.cpp 
   kio_smb_auth.cpp 
   kio_smb_browse.cpp 
   kio_smb_config.cpp 
   kio_smb_dir.cpp 
   kio_smb_file.cpp 
   kio_smb_internal.cpp 
   kio_smb_mount.cpp
   wsdiscoverer.cpp
   dnssddiscoverer.cpp
   discovery.cpp
)

ecm_qt_declare_logging_category(kio_smb_PART_SRCS
                                HEADER smb-logsettings.h
                                IDENTIFIER KIO_SMB_LOG
                                CATEGORY_NAME log_kio_smb)

include_directories(${SAMBA_INCLUDE_DIR})

add_library(kio_smb MODULE ${kio_smb_PART_SRCS})

target_link_libraries(kio_smb
    KF5::KIOCore
    KF5::I18n
    ${SAMBA_LIBRARIES}
    Qt5::Network
    KF5::DNSSD
    KDSoap::WSDiscoveryClient
)

set_target_properties(kio_smb PROPERTIES OUTPUT_NAME "smb")
set_target_properties(kio_smb PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio")

install(TARGETS kio_smb DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### install files ###############

install( FILES smb-network.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/konqueror/dirtree/remote )
install( FILES smb-network.desktop  DESTINATION  ${KDE_INSTALL_DATADIR}/remoteview )

