INCLUDE(CheckIncludeFile)
set(CMAKE_AUTOMAKE ON)

if(NOT WIN32)
CHECK_INCLUDE_FILE(utime.h HAVE_UTIME_H)

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


target_link_libraries(kio_smb KF5::KIOCore KF5::KI18n ${SAMBA_LIBRARIES} )
else(NOT WIN32)
set(kio_smb_PART_SRCS 
   kio_smb_win.cpp)

add_library(kio_smb MODULE ${kio_smb_PART_SRCS})

target_link_libraries(kio_smb KF5::KIOCore KF5::KI18n mpr)    
endif(NOT WIN32)

install(TARGETS kio_smb  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/dirtree/remote )
install( FILES smb.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/remoteview )

