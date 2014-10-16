add_definitions(-DTRANSLATION_DOMAIN=\"kio_nfs\")

add_library(kio_nfs MODULE kio_nfs.cpp nfsv2.cpp nfsv3.cpp rpc_mnt3_xdr.c rpc_nfs3_prot_xdr.c rpc_mnt2_xdr.c rpc_nfs2_prot_xdr.c)
target_link_libraries(kio_nfs KF5::KIOCore KF5::I18n Qt5::Network)

install(TARGETS kio_nfs  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES nfs.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
