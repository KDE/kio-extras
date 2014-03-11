add_subdirectory(tests)

########### kio_archive ###############

set(kio_archive_SRCS kio_archive.cpp )

add_library(kio_archive MODULE ${kio_archive_SRCS})

target_link_libraries(kio_archive KF5::KIOCore )

install(TARGETS kio_archive DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES tar.protocol ar.protocol zip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


