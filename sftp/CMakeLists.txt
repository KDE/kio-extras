project(kioslave-sftp)

add_definitions (-DQT3_SUPPORT)

add_subdirectory(tests)

########### next target ###############

set(kio_sftp_PART_SRCS 
   process.cpp 
   atomicio.cpp 
   kio_sftp.cpp 
   sftpfileattr.cpp 
   ksshprocess.cpp )

kde4_automoc(${kio_sftp_PART_SRCS})

kde4_add_plugin(kio_sftp ${kio_sftp_PART_SRCS})


target_link_libraries(kio_sftp  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KDESU_LIBS} )

install(TARGETS kio_sftp  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

