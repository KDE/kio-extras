########### next target ###############

set(kio_sftp_PART_SRCS
   kio_sftp.cpp
)

include_directories(${LIBSSH_INCLUDE_DIRS})

kde4_add_plugin(kio_sftp ${kio_sftp_PART_SRCS})
target_link_libraries(kio_sftp ${KDE4_KIO_LIBS} ${LIBSSH_LIBRARIES})

install(TARGETS kio_sftp  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

