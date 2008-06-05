check_include_files(utime.h HAVE_UTIME_H)

add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE) # for off_t

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

kde4_add_plugin(kio_smb ${kio_smb_PART_SRCS})


target_link_libraries(kio_smb   ${KDE4_KDE3SUPPORT_LIBS} ${SAMBA_LIBRARIES} )

install(TARGETS kio_smb  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/dirtree/remote )
install( FILES smb.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/remoteview )

