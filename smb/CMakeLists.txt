add_feature_info("SMB DNS-SD Discovery" HAVE_KDNSSD_WITH_SIGNAL_RACE_PROTECTION
    "Discover SMB hosts via DNS-SD/Avahi/Bonjour. KF5DNSSD >= 5.54 is required to support this.")

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_smb\")

include(CheckIncludeFile)
set(CMAKE_AUTOMAKE ON)

if(NOT WIN32)
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
   kio_smb_mount.cpp )

include_directories(${SAMBA_INCLUDE_DIR})

add_library(kio_smb MODULE ${kio_smb_PART_SRCS})

target_link_libraries(kio_smb
    KF5::KIOCore
    KF5::I18n
    ${SAMBA_LIBRARIES}
    Qt5::Network
    KF5::DNSSD)
else()
set(kio_smb_PART_SRCS 
   kio_smb_win.cpp)

add_library(kio_smb MODULE ${kio_smb_PART_SRCS})

target_link_libraries(kio_smb
    KF5::KIOCore
    KF5::I18n
    mpr
    Qt5::Network
    KF5::DNSSD)
endif()

set_target_properties(kio_smb PROPERTIES OUTPUT_NAME "smb")

install(TARGETS kio_smb DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)


########### install files ###############

install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/dirtree/remote )
install( FILES smb.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/remoteview )

