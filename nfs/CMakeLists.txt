add_library(kio_nfs MODULE kio_nfs.cpp mount_xdr.c nfs_prot_xdr.c)
target_link_libraries(kio_nfs KF5::KIOCore KF5::KDE4Support)

install(TARGETS kio_nfs  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES nfs.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
