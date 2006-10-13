
project(kioslave-nfs)



########### next target ###############

set(kio_nfs_PART_SRCS kio_nfs.cpp mount_xdr.c nfs_prot_xdr.c )

kde4_automoc(kio_nfs ${kio_nfs_PART_SRCS})

kde4_add_plugin(kio_nfs ${kio_nfs_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_nfs )

target_link_libraries(kio_nfs  ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS kio_nfs  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES nfs.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


