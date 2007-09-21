add_subdirectory(tests)

########### kio_tar ###############

set(kio_tar_SRCS tar.cc )

kde4_add_plugin(kio_tar ${kio_tar_SRCS})

target_link_libraries(kio_tar ${KDE4_KIO_LIBS} )

install(TARGETS kio_tar DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES tar.protocol ar.protocol zip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


