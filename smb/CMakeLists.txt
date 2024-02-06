remove_definitions(
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
)

find_package(smbclient 0.7.0 REQUIRED)
set_package_properties(smbclient PROPERTIES
    DESCRIPTION "the SMB client library"
    URL "https://www.samba.org/"
    PURPOSE "Needed to build the SMB KIO worker"
)

find_package(KDSoapWSDiscoveryClient REQUIRED)

find_package(Threads REQUIRED)

add_definitions(-DTRANSLATION_DOMAIN=\"kio6_smb\")

include(CheckIncludeFile)
include(CheckSymbolExists)
set(CMAKE_AUTOMAKE ON)

cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_LIBRARIES smbclient::smbclient)
check_symbol_exists(smbc_readdirplus2 "libsmbclient.h" HAVE_READDIRPLUS2)
cmake_pop_check_state()
check_include_file(utime.h HAVE_UTIME_H)

configure_file(config-smb.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-smb.h)

# Intermediate static lib target for reuse in testing.
add_library(kio_smb_static STATIC)

target_sources(kio_smb_static PRIVATE
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

ecm_qt_declare_logging_category(kio_smb_static
    HEADER smb-logsettings.h
    IDENTIFIER KIO_SMB_LOG
    CATEGORY_NAME kf.kio.workers.smb
    OLD_CATEGORY_NAMES log_kio_smb kf.kio.slaves.smb
    DESCRIPTION "KIO smb"
    EXPORT KIO_EXTRAS
)

set_property(TARGET kio_smb_static PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(kio_smb_static
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}/..>"
)
target_link_libraries(kio_smb_static
    KF6::KIOCore
    KF6::I18n
    KF6::ConfigCore
    smbclient::smbclient
    Qt::Network
    KF6::DNSSD
    KDSoap::WSDiscoveryClient
    Threads::Threads # std::async
)

# Final plugin target.
add_library(kio_smb MODULE main.cpp)
target_link_libraries(kio_smb
    kio_smb_static
)

set_target_properties(kio_smb PROPERTIES OUTPUT_NAME "smb")
set_target_properties(kio_smb PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf6/kio")

install(TARGETS kio_smb DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kio)
install(FILES smb-network.desktop DESTINATION ${KDE_INSTALL_DATADIR}/konqueror/dirtree/remote)
install(FILES smb-network.desktop DESTINATION ${KDE_INSTALL_DATADIR}/remoteview)

option(WITH_SHAREDMIMEINFO "Build with shared shared-mime-info and update mimetype database on install" ON)
if(WITH_SHAREDMIMEINFO)
    install(FILES org.kde.kio.smb.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
    find_package(SharedMimeInfo 1.3)
    set_package_properties(SharedMimeInfo PROPERTIES
        TYPE REQUIRED
        PURPOSE "Used to update the mimetype database")
    update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})
endif()

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()
add_subdirectory(kded)
