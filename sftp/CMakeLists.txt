########### next target ###############

set(kio_sftp_PART_SRCS
   kio_sftp.cpp
)

include_directories(${LIBSSH_INCLUDE_DIR})

add_library(kio_sftp MODULE ${kio_sftp_PART_SRCS})
target_link_libraries(kio_sftp KF5::KIOCore ${LIBSSH_LIBRARIES})

install(TARGETS kio_sftp  DESTINATION ${PLUGIN_INSTALL_DIR} )

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

