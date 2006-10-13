
project(kioslave-smb)



########### next target ###############

set(kio_smb_PART_SRCS 
   kio_smb.cpp 
   kio_smb_auth.cpp 
   kio_smb_browse.cpp 
   kio_smb_config.cpp 
   kio_smb_dir.cpp 
   kio_smb_file.cpp 
   kio_smb_internal.cpp 
   kio_smb_mount.cpp )

kde4_automoc(kio_smb ${kio_smb_PART_SRCS})

kde4_add_plugin(kio_smb ${kio_smb_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_smb )

target_link_libraries(kio_smb  ${KDE4_KDE3SUPPORT_LIBS} ${SAMBA_LIBRARIES} )

install(TARGETS kio_smb  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/konqueror/dirtree/remote )
install( FILES x-smb-workgroup.desktop x-smb-server.desktop  DESTINATION  ${MIME_INSTALL_DIR}/application )
install( FILES smb.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES smb-network.desktop  DESTINATION  ${DATA_INSTALL_DIR}/remoteview )

