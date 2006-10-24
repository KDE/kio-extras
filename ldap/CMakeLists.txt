
project(kioslave-ldap)



########### next target ###############

set(kio_ldap_PART_SRCS kio_ldap.cpp )

kde4_automoc(${kio_ldap_PART_SRCS})

kde4_add_plugin(kio_ldap ${kio_ldap_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_ldap )

target_link_libraries(kio_ldap  ${KDE4_KIO_LIBS} kldap )

install(TARGETS kio_ldap  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ldap.protocol ldaps.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
