
project(kioslave-tar)



########### next target ###############

set(kio_tar_PART_SRCS tar.cc )

kde4_automoc(kio_tar ${kio_tar_PART_SRCS})

kde4_add_plugin(kio_tar ${kio_tar_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_tar )

target_link_libraries(kio_tar  ${KDE4_KIO_LIBS} )

install(TARGETS kio_tar  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES tar.protocol ar.protocol zip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


