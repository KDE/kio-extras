
add_definitions (-DQT3_SUPPORT)

add_subdirectory(tests)

########### next target ###############

set(kio_sftp_PART_SRCS 
   process.cpp 
   atomicio.cpp 
   kio_sftp.cpp 
   sftpfileattr.cpp 
   ksshprocess.cpp )


kde4_add_plugin(kio_sftp ${kio_sftp_PART_SRCS})


target_link_libraries(kio_sftp ${KDE4_KIO_LIBS} ${KDE4_KPTY_LIBS})

install(TARGETS kio_sftp  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

